#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/render_verification.hpp"
#include "axm/render/scene_contract.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
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
    std::string expected_capabilities_path;
};

struct RendererProcessEvidence {
    std::filesystem::path invocation_path;
    std::filesystem::path canonical_path;
    std::uint64_t digest64 = 0;
    std::string resolution;
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
        } else if (arg == "--expect-capabilities" && i + 1 < argc) {
            args.expected_capabilities_path = argv[++i];
        } else if (arg == "--help") {
            std::cout
                << "AXM external renderer process adapter harness\n"
                << "  --renderer PATH             external renderer/adapter executable\n"
                << "  --request PATH              AXM_RENDER_REQUEST v1 input\n"
                << "  --receipt PATH              AXM_RENDER_RECEIPT v1 output path\n"
                << "  --capabilities PATH         generated AXM_RENDER_CAPABILITIES v1 evidence path\n"
                << "  --expect-capabilities PATH  optional previously inspected capability manifest\n\n"
                << "Current experimental process convention:\n"
                << "  RENDERER --capabilities CAPS\n"
                << "  RENDERER REQUEST RECEIPT\n\n"
                << "When --expect-capabilities is provided, the freshly discovered manifest\n"
                << "must semantically match that caller-selected v1 manifest before rendering.\n"
                << "Explicit renderer paths and bare renderer names resolved through PATH are\n"
                << "pinned to one invocation path; its canonical target and byte digest are\n"
                << "continuity-checked across both child-process phases.\n"
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

char executable_path_list_separator() {
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

std::vector<std::filesystem::path> executable_name_candidates(
    const std::filesystem::path& directory,
    const std::filesystem::path& supplied_name) {
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(directory / supplied_name);
#ifdef _WIN32
    if (supplied_name.extension().empty()) {
        const char* raw_pathext = std::getenv("PATHEXT");
        const std::string pathext = raw_pathext ? raw_pathext : ".COM;.EXE;.BAT;.CMD";
        std::size_t start = 0;
        while (start <= pathext.size()) {
            const std::size_t end = pathext.find(';', start);
            const std::string extension = pathext.substr(
                start,
                end == std::string::npos ? std::string::npos : end - start);
            if (!extension.empty()) {
                candidates.push_back(directory / (supplied_name.string() + extension));
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
#endif
    return candidates;
}

std::optional<std::filesystem::path> resolve_renderer_from_path(
    const std::string& executable) {
    const char* raw_path = std::getenv("PATH");
    if (raw_path == nullptr) return std::nullopt;

    const std::string path_list(raw_path);
    const char separator = executable_path_list_separator();
    const std::filesystem::path supplied_name(executable);
    std::size_t start = 0;
    while (start <= path_list.size()) {
        const std::size_t end = path_list.find(separator, start);
        const std::string entry = path_list.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        const std::filesystem::path directory = entry.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(entry);

        for (const auto& candidate : executable_name_candidates(directory, supplied_name)) {
            std::error_code ec;
            const std::filesystem::file_status status = std::filesystem::status(candidate, ec);
            if (ec || !std::filesystem::is_regular_file(status)) continue;
#ifndef _WIN32
            if (::access(candidate.c_str(), X_OK) != 0) continue;
#endif
            return std::filesystem::absolute(candidate).lexically_normal();
        }

        if (end == std::string::npos) break;
        start = end + 1;
    }
    return std::nullopt;
}

std::optional<RendererProcessEvidence> inspect_renderer_process_evidence(
    const std::string& executable) {
    const std::filesystem::path supplied(executable);
    const bool explicit_path = supplied.is_absolute() || supplied.has_parent_path();

    std::filesystem::path invocation_path;
    std::string resolution;
    if (explicit_path) {
        invocation_path = normalized_absolute(executable);
        resolution = "EXPLICIT";
    } else {
        const auto path_match = resolve_renderer_from_path(executable);
        if (!path_match.has_value()) return std::nullopt;
        invocation_path = *path_match;
        resolution = "PATH";
    }

    std::error_code ec;
    const std::filesystem::path canonical_path =
        std::filesystem::canonical(invocation_path, ec);
    if (ec) {
        throw std::runtime_error(
            "cannot resolve renderer executable path for continuity evidence: " +
            ec.message());
    }

    const std::filesystem::file_status status = std::filesystem::status(canonical_path, ec);
    if (ec || !std::filesystem::is_regular_file(status)) {
        throw std::runtime_error(
            "renderer executable path must resolve to a regular file for dispatch");
    }

    RendererProcessEvidence evidence;
    evidence.invocation_path = invocation_path;
    evidence.canonical_path = canonical_path;
    evidence.digest64 = axm::render::continuity_digest64_file(canonical_path.string());
    evidence.resolution = resolution;
    return evidence;
}

void reject_mutable_artifact_collision_with_renderer(
    const std::filesystem::path& artifact_path,
    const char* artifact_name,
    const RendererProcessEvidence& renderer) {
    reject_path_collision(
        artifact_path,
        artifact_name,
        renderer.invocation_path,
        "renderer executable");
    if (renderer.canonical_path != renderer.invocation_path) {
        reject_path_collision(
            artifact_path,
            artifact_name,
            renderer.canonical_path,
            "renderer executable target");
    }
}

void require_renderer_process_unchanged(const RendererProcessEvidence& expected) {
    std::error_code ec;
    const std::filesystem::path observed_canonical =
        std::filesystem::canonical(expected.invocation_path, ec);
    if (ec) {
        throw std::runtime_error(
            "renderer executable path became unavailable during external dispatch: " +
            ec.message());
    }
    if (observed_canonical != expected.canonical_path) {
        throw std::runtime_error(
            "renderer executable target changed during external dispatch");
    }
    if (axm::render::continuity_digest64_file(observed_canonical.string()) != expected.digest64) {
        throw std::runtime_error(
            "renderer executable bytes changed during external dispatch");
    }
}

void clear_declared_artifact_path(
    const std::filesystem::path& path,
    const char* label) {
    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, ec);
    if (ec == std::errc::no_such_file_or_directory) {
        return;
    }
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
    if (ec == std::errc::no_such_file_or_directory) {
        throw std::runtime_error(
            std::string("external renderer did not produce ") + label);
    }
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

std::string capability_manifest_mismatch(
    const axm::render::RenderCapabilities& expected,
    const axm::render::RenderCapabilities& observed) {
    if (expected.renderer != observed.renderer) {
        return "renderer mismatch: expected=" + expected.renderer +
            " observed=" + observed.renderer;
    }
    if (expected.renderer_version != observed.renderer_version) {
        return "renderer_version mismatch: expected=" + expected.renderer_version +
            " observed=" + observed.renderer_version;
    }
    if (expected.backend != observed.backend) {
        return "backend mismatch: expected=" + expected.backend +
            " observed=" + observed.backend;
    }
    if (expected.scene_contract != observed.scene_contract) {
        return "scene_contract mismatch: expected=" +
            std::to_string(expected.scene_contract) + " observed=" +
            std::to_string(observed.scene_contract);
    }
    if (expected.render_request_contract != observed.render_request_contract) {
        return "render_request_contract mismatch: expected=" +
            std::to_string(expected.render_request_contract) + " observed=" +
            std::to_string(observed.render_request_contract);
    }
    if (expected.max_width != observed.max_width || expected.max_height != observed.max_height) {
        return "max dimensions mismatch: expected=" +
            std::to_string(expected.max_width) + "x" + std::to_string(expected.max_height) +
            " observed=" + std::to_string(observed.max_width) + "x" +
            std::to_string(observed.max_height);
    }

    auto expected_formats = expected.output_formats;
    auto observed_formats = observed.output_formats;
    std::sort(expected_formats.begin(), expected_formats.end());
    std::sort(observed_formats.begin(), observed_formats.end());
    if (expected_formats != observed_formats) {
        return "output format set mismatch";
    }
    return {};
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

        const auto renderer_process_evidence =
            inspect_renderer_process_evidence(args.renderer_executable);
        const std::string renderer_launch_executable = renderer_process_evidence.has_value()
            ? renderer_process_evidence->invocation_path.string()
            : args.renderer_executable;
        const auto request_path = normalized_absolute(args.request_path);
        const auto scene_path = normalized_absolute(request.scene_path);
        const auto output_path = normalized_absolute(request.output_path);
        const auto receipt_path = normalized_absolute(args.receipt_path);
        const auto capabilities_path = normalized_absolute(args.capabilities_path);
        const bool has_expected_capabilities = !args.expected_capabilities_path.empty();
        const auto expected_capabilities_path = has_expected_capabilities
            ? normalized_absolute(args.expected_capabilities_path)
            : std::filesystem::path{};

        reject_path_collision(output_path, "render output", request_path, "request source");
        reject_path_collision(output_path, "render output", scene_path, "scene source");
        reject_path_collision(receipt_path, "receipt", request_path, "request source");
        reject_path_collision(receipt_path, "receipt", scene_path, "scene source");
        reject_path_collision(receipt_path, "receipt", output_path, "render output");
        reject_path_collision(capabilities_path, "capabilities", request_path, "request source");
        reject_path_collision(capabilities_path, "capabilities", scene_path, "scene source");
        reject_path_collision(capabilities_path, "capabilities", output_path, "render output");
        reject_path_collision(capabilities_path, "capabilities", receipt_path, "receipt");

        if (renderer_process_evidence.has_value()) {
            reject_mutable_artifact_collision_with_renderer(
                output_path,
                "render output",
                *renderer_process_evidence);
            reject_mutable_artifact_collision_with_renderer(
                receipt_path,
                "receipt",
                *renderer_process_evidence);
            reject_mutable_artifact_collision_with_renderer(
                capabilities_path,
                "capabilities",
                *renderer_process_evidence);
            require_renderer_process_unchanged(*renderer_process_evidence);
        }

        std::uint64_t expected_capabilities_digest = 0;
        axm::render::RenderCapabilities expected_capabilities;
        if (has_expected_capabilities) {
            reject_path_collision(
                expected_capabilities_path, "expected capabilities", request_path, "request source");
            reject_path_collision(
                expected_capabilities_path, "expected capabilities", scene_path, "scene source");
            reject_path_collision(
                expected_capabilities_path, "expected capabilities", output_path, "render output");
            reject_path_collision(
                expected_capabilities_path, "expected capabilities", receipt_path, "receipt");
            reject_path_collision(
                expected_capabilities_path, "expected capabilities", capabilities_path, "capabilities");

            expected_capabilities_digest = axm::render::continuity_digest64_file(
                args.expected_capabilities_path);
            expected_capabilities = axm::render::load_render_capabilities_file(
                args.expected_capabilities_path);
            require_digest_unchanged(
                args.expected_capabilities_path,
                expected_capabilities_digest,
                "expected capability manifest");

            const std::string expected_incompatibility =
                axm::render::render_request_incompatibility(request, expected_capabilities);
            if (!expected_incompatibility.empty()) {
                throw std::runtime_error(
                    "expected capability manifest rejects request: " + expected_incompatibility);
            }
        }

        clear_declared_artifact_path(capabilities_path, "capability manifest");
        run_required(
            renderer_launch_executable,
            {"--capabilities", args.capabilities_path},
            "capability-discovery");
        require_fresh_regular_artifact(capabilities_path, "capability manifest");
        if (renderer_process_evidence.has_value()) {
            require_renderer_process_unchanged(*renderer_process_evidence);
        }

        require_digest_unchanged(args.request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");
        if (has_expected_capabilities) {
            require_digest_unchanged(
                args.expected_capabilities_path,
                expected_capabilities_digest,
                "expected capability manifest");
        }

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

        if (has_expected_capabilities) {
            const std::string mismatch =
                capability_manifest_mismatch(expected_capabilities, capabilities);
            if (!mismatch.empty()) {
                throw std::runtime_error(
                    "fresh capability manifest does not match expected manifest: " + mismatch);
            }
        }

        clear_declared_artifact_path(output_path, "render output");
        clear_declared_artifact_path(receipt_path, "receipt");
        if (renderer_process_evidence.has_value()) {
            require_renderer_process_unchanged(*renderer_process_evidence);
        }
        run_required(
            renderer_launch_executable,
            {args.request_path, args.receipt_path},
            "render");
        require_fresh_regular_artifact(output_path, "render output");
        require_fresh_regular_artifact(receipt_path, "receipt");
        if (renderer_process_evidence.has_value()) {
            require_renderer_process_unchanged(*renderer_process_evidence);
        }

        require_digest_unchanged(args.request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");
        require_fresh_regular_artifact(capabilities_path, "capability manifest");
        require_digest_unchanged(
            args.capabilities_path, capabilities_digest, "capability manifest");
        if (has_expected_capabilities) {
            require_digest_unchanged(
                args.expected_capabilities_path,
                expected_capabilities_digest,
                "expected capability manifest");
        }

        const axm::render::RenderReceiptVerification verification =
            axm::render::verify_render_receipt_files_with_capabilities(
                args.request_path,
                args.receipt_path,
                args.capabilities_path);
        const axm::render::RenderReceipt receipt =
            axm::render::load_render_receipt_file(args.receipt_path);
        if (renderer_process_evidence.has_value()) {
            require_renderer_process_unchanged(*renderer_process_evidence);
        }

        std::cout << "renderer_process=" << args.renderer_executable << "\n";
        if (renderer_process_evidence.has_value()) {
            std::cout << "renderer_process_resolution="
                      << renderer_process_evidence->resolution << "\n";
            std::cout << "renderer_process_invocation_path="
                      << renderer_process_evidence->invocation_path.string() << "\n";
            std::cout << "renderer_process_canonical_path="
                      << renderer_process_evidence->canonical_path.string() << "\n";
            std::cout << "renderer_process_digest64="
                      << axm::render::digest64_hex(renderer_process_evidence->digest64) << "\n";
            std::cout << "renderer_process_continuity=PASS\n";
        } else {
            std::cout << "renderer_process_resolution=UNRESOLVED\n";
            std::cout << "renderer_process_continuity=UNAVAILABLE\n";
        }
        std::cout << "renderer=" << receipt.renderer << "\n";
        std::cout << "renderer_version=" << receipt.renderer_version << "\n";
        std::cout << "backend=" << receipt.backend << "\n";
        if (has_expected_capabilities) {
            std::cout << "expected_capabilities_digest64="
                      << axm::render::digest64_hex(expected_capabilities_digest) << "\n";
            std::cout << "expected_capabilities_match=PASS\n";
        }
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
