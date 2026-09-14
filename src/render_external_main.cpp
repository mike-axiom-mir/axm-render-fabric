#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/render_verification.hpp"
#include "axm/render/scene_contract.hpp"

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

struct Args {
    std::string renderer_executable;
    std::string request_path;
    std::string receipt_path;
    std::string capabilities_path;
};

Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--renderer" && i + 1 < argc) {
            args.renderer_executable = argv[++i];
        } else if (arg == "--request" && i + 1 < argc) {
            args.request_path = argv[++i];
        } else if (arg == "--receipt" && i + 1 < argc) {
            args.receipt_path = argv[++i];
        } else if (arg == "--capabilities" && i + 1 < argc) {
            args.capabilities_path = argv[++i];
        } else if (arg == "--help") {
            std::cout
                << "AXM external renderer process adapter harness\n"
                << "  --renderer PATH       external renderer/adapter executable\n"
                << "  --request PATH        AXM_RENDER_REQUEST v1 input\n"
                << "  --receipt PATH        AXM_RENDER_RECEIPT v1 output path\n"
                << "  --capabilities PATH   generated AXM_RENDER_CAPABILITIES v1 evidence path\n\n"
                << "Current experimental process convention:\n"
                << "  RENDERER --capabilities CAPS\n"
                << "  RENDERER REQUEST RECEIPT\n\n"
                << "Declared output/receipt/capability paths are cleared before their phase,\n"
                << "then must be recreated as regular files by that child invocation.\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + arg);
        }
    }

    if (args.renderer_executable.empty() || args.request_path.empty() ||
        args.receipt_path.empty() || args.capabilities_path.empty()) {
        throw std::invalid_argument(
            "--renderer, --request, --receipt, and --capabilities are all required");
    }
    return args;
}

std::filesystem::path normalized_absolute(const std::string& path) {
    return std::filesystem::absolute(std::filesystem::path(path)).lexically_normal();
}

void reject_path_collision(
    const std::filesystem::path& a,
    const char* a_name,
    const std::filesystem::path& b,
    const char* b_name) {
    if (a == b) {
        throw std::invalid_argument(
            std::string(a_name) + " path must differ from " + b_name + " path");
    }
}

void clear_declared_artifact_path(
    const std::filesystem::path& path,
    const char* label) {
    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, ec);
    if (ec) {
        throw std::runtime_error(
            std::string("cannot inspect ") + label + " path before dispatch: " +
            ec.message());
    }
    if (!std::filesystem::exists(status)) {
        return;
    }
    if (std::filesystem::is_directory(status)) {
        throw std::invalid_argument(
            std::string(label) + " path must not be an existing directory");
    }

    if (!std::filesystem::remove(path, ec) || ec) {
        throw std::runtime_error(
            std::string("cannot clear pre-existing ") + label + " path: " +
            (ec ? ec.message() : "remove returned false"));
    }
}

void require_fresh_regular_artifact(
    const std::filesystem::path& path,
    const char* label) {
    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, ec);
    if (ec) {
        throw std::runtime_error(
            std::string("cannot inspect produced ") + label + ": " + ec.message());
    }
    if (!std::filesystem::exists(status)) {
        throw std::runtime_error(
            std::string("external renderer did not produce ") + label);
    }
    if (std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status)) {
        throw std::runtime_error(
            std::string("external renderer ") + label +
            " must be a newly produced regular file");
    }
}

int run_process(const std::string& executable, const std::vector<std::string>& args) {
#ifdef _WIN32
    std::vector<const char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(executable.c_str());
    for (const auto& arg : args) argv.push_back(arg.c_str());
    argv.push_back(nullptr);

    const intptr_t status = _spawnvp(_P_WAIT, executable.c_str(), argv.data());
    if (status == -1) {
        throw std::runtime_error("failed to launch external renderer process: " + executable);
    }
    return static_cast<int>(status);
#else
    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("failed to fork external renderer process");
    }

    if (pid == 0) {
        std::vector<std::string> storage;
        storage.reserve(args.size() + 1);
        storage.push_back(executable);
        storage.insert(storage.end(), args.begin(), args.end());

        std::vector<char*> argv;
        argv.reserve(storage.size() + 1);
        for (auto& arg : storage) argv.push_back(arg.data());
        argv.push_back(nullptr);

        execvp(executable.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error("failed while waiting for external renderer process");
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 125;
#endif
}

void run_required(
    const std::string& executable,
    const std::vector<std::string>& args,
    const char* phase) {
    const int status = run_process(executable, args);
    if (status != 0) {
        throw std::runtime_error(
            std::string("external renderer ") + phase +
            " phase exited with status " + std::to_string(status));
    }
}

void require_digest_unchanged(
    const std::string& path,
    std::uint64_t expected,
    const char* label) {
    if (axm::render::continuity_digest64_file(path) != expected) {
        throw std::runtime_error(std::string(label) + " changed during external dispatch");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse_args(argc, argv);

        const std::uint64_t request_digest =
            axm::render::continuity_digest64_file(args.request_path);
        const axm::render::RenderRequest request =
            axm::render::load_render_request_file(args.request_path);
        require_digest_unchanged(args.request_path, request_digest, "render request source");

        const std::uint64_t scene_digest =
            axm::render::continuity_digest64_file(request.scene_path);
        axm::render::load_scene_file(request.scene_path);
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        const auto request_path = normalized_absolute(args.request_path);
        const auto scene_path = normalized_absolute(request.scene_path);
        const auto output_path = normalized_absolute(request.output_path);
        const auto receipt_path = normalized_absolute(args.receipt_path);
        const auto capabilities_path = normalized_absolute(args.capabilities_path);

        reject_path_collision(output_path, "render output", request_path, "request source");
        reject_path_collision(output_path, "render output", scene_path, "scene source");
        reject_path_collision(receipt_path, "receipt", request_path, "request source");
        reject_path_collision(receipt_path, "receipt", scene_path, "scene source");
        reject_path_collision(receipt_path, "receipt", output_path, "render output");
        reject_path_collision(capabilities_path, "capabilities", request_path, "request source");
        reject_path_collision(capabilities_path, "capabilities", scene_path, "scene source");
        reject_path_collision(capabilities_path, "capabilities", output_path, "render output");
        reject_path_collision(capabilities_path, "capabilities", receipt_path, "receipt");

        clear_declared_artifact_path(capabilities_path, "capability manifest");
        run_required(
            args.renderer_executable,
            {"--capabilities", args.capabilities_path},
            "capability-discovery");
        require_fresh_regular_artifact(capabilities_path, "capability manifest");

        require_digest_unchanged(args.request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        const std::uint64_t capabilities_digest =
            axm::render::continuity_digest64_file(args.capabilities_path);
        const axm::render::RenderCapabilities capabilities =
            axm::render::load_render_capabilities_file(args.capabilities_path);
        require_digest_unchanged(
            args.capabilities_path, capabilities_digest, "capability manifest");

        const std::string incompatibility =
            axm::render::render_request_incompatibility(request, capabilities);
        if (!incompatibility.empty()) {
            throw std::runtime_error(
                "external renderer capabilities reject request: " + incompatibility);
        }

        clear_declared_artifact_path(output_path, "render output");
        clear_declared_artifact_path(receipt_path, "receipt");
        run_required(
            args.renderer_executable,
            {args.request_path, args.receipt_path},
            "render");
        require_fresh_regular_artifact(output_path, "render output");
        require_fresh_regular_artifact(receipt_path, "receipt");

        require_digest_unchanged(args.request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");
        require_fresh_regular_artifact(capabilities_path, "capability manifest");
        require_digest_unchanged(
            args.capabilities_path, capabilities_digest, "capability manifest");

        const axm::render::RenderReceiptVerification verification =
            axm::render::verify_render_receipt_files_with_capabilities(
                args.request_path,
                args.receipt_path,
                args.capabilities_path);
        const axm::render::RenderReceipt receipt =
            axm::render::load_render_receipt_file(args.receipt_path);

        std::cout << "renderer_process=" << args.renderer_executable << "\n";
        std::cout << "renderer=" << receipt.renderer << "\n";
        std::cout << "renderer_version=" << receipt.renderer_version << "\n";
        std::cout << "backend=" << receipt.backend << "\n";
        std::cout << "capabilities_digest64="
                  << axm::render::digest64_hex(capabilities_digest) << "\n";
        std::cout << "scene_source_digest64="
                  << axm::render::digest64_hex(verification.scene_source_digest64) << "\n";
        std::cout << "request_source_digest64="
                  << axm::render::digest64_hex(verification.request_source_digest64) << "\n";
        std::cout << "frame_pixels_digest64="
                  << axm::render::digest64_hex(verification.frame_pixels_digest64) << "\n";
        std::cout << "output_file_digest64="
                  << axm::render::digest64_hex(verification.output_file_digest64) << "\n";
        std::cout << "external_process_dispatch=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
