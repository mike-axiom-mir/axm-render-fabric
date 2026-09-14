#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

constexpr const char* adapter_backend = "external.imagemagick.svg-raster";
constexpr const char* adapter_renderer = "axm.adapter.imagemagick-svg";
constexpr const char* adapter_version_base = "0.2.0";

struct DelegatedRendererIdentity {
    std::filesystem::path resolved_executable;
    std::uint64_t executable_digest64 = 0;
    std::string bound_adapter_version;
};

std::vector<std::string> executable_suffixes(const std::filesystem::path& executable) {
#ifdef _WIN32
    if (executable.has_extension()) return {""};

    std::vector<std::string> suffixes;
    const char* raw_pathext = std::getenv("PATHEXT");
    const std::string pathext = raw_pathext && *raw_pathext != '\0'
        ? raw_pathext
        : ".COM;.EXE;.BAT;.CMD";
    std::size_t begin = 0;
    while (begin <= pathext.size()) {
        const std::size_t end = pathext.find(';', begin);
        std::string suffix = pathext.substr(begin, end - begin);
        if (!suffix.empty()) suffixes.push_back(suffix);
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    if (suffixes.empty()) suffixes.push_back("");
    return suffixes;
#else
    (void)executable;
    return {""};
#endif
}

std::filesystem::path canonical_regular_file(const std::filesystem::path& candidate) {
    std::error_code ec;
    const auto status = std::filesystem::status(candidate, ec);
    if (ec || !std::filesystem::is_regular_file(status)) return {};

    const auto resolved = std::filesystem::canonical(candidate, ec);
    if (ec || resolved.empty()) return {};
    return resolved;
}

std::filesystem::path resolve_executable_path(const std::string& executable) {
    const std::filesystem::path command(executable);
    const auto suffixes = executable_suffixes(command);

    auto try_base = [&](const std::filesystem::path& base) -> std::filesystem::path {
        for (const auto& suffix : suffixes) {
            std::filesystem::path candidate = base;
            candidate += suffix;
            const auto resolved = canonical_regular_file(candidate);
            if (!resolved.empty()) return resolved;
        }
        return {};
    };

    if (command.is_absolute() || command.has_parent_path()) {
        const auto resolved = try_base(command);
        if (!resolved.empty()) return resolved;
        throw std::runtime_error(
            "cannot resolve delegated ImageMagick executable to a regular file: " + executable);
    }

    const char* raw_path = std::getenv("PATH");
    if (raw_path == nullptr || *raw_path == '\0') {
        throw std::runtime_error(
            "cannot resolve delegated ImageMagick executable because PATH is empty: " + executable);
    }

    const std::string path_list(raw_path);
#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif
    std::size_t begin = 0;
    while (begin <= path_list.size()) {
        const std::size_t end = path_list.find(separator, begin);
        const std::string entry = path_list.substr(begin, end - begin);
        const std::filesystem::path directory = entry.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(entry);
        const auto resolved = try_base(directory / command);
        if (!resolved.empty()) return resolved;
        if (end == std::string::npos) break;
        begin = end + 1;
    }

    throw std::runtime_error(
        "cannot resolve delegated ImageMagick executable on PATH: " + executable);
}

DelegatedRendererIdentity delegated_renderer_identity(const std::string& executable) {
    const auto resolved = resolve_executable_path(executable);
    const std::uint64_t digest = axm::render::continuity_digest64_file(resolved.string());
    const std::string digest_hex = axm::render::digest64_hex(digest);
    return {
        resolved,
        digest,
        std::string(adapter_version_base) + "+delegated-fnv64-" + digest_hex.substr(2)
    };
}

axm::render::RenderCapabilities adapter_capabilities(
    const DelegatedRendererIdentity& delegated) {
    return {
        adapter_renderer,
        delegated.bound_adapter_version,
        adapter_backend,
        axm::render::scene_contract_version,
        axm::render::render_request_contract_version,
        8192,
        8192,
        {"ppm-rgb8"}
    };
}

std::string imagemagick_executable() {
    if (const char* configured = std::getenv("AXM_IMAGEMAGICK_CONVERT")) {
        if (*configured != '\0') return configured;
    }
    return "convert";
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
        throw std::runtime_error("failed to launch delegated ImageMagick process: " + executable);
    }
    return static_cast<int>(status);
#else
    const pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("failed to fork delegated ImageMagick process");
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
        throw std::runtime_error("failed while waiting for delegated ImageMagick process");
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return 125;
#endif
}

void require_process_success(
    const std::string& executable,
    const std::vector<std::string>& args,
    const char* phase) {
    const int status = run_process(executable, args);
    if (status != 0) {
        throw std::runtime_error(
            std::string("delegated ImageMagick ") + phase +
            " exited with status " + std::to_string(status));
    }
}

std::uint64_t process_id_value() noexcept {
#ifdef _WIN32
    return static_cast<std::uint64_t>(_getpid());
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}

struct ScratchDirectory {
    std::filesystem::path path;

    ~ScratchDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

ScratchDirectory make_scratch_directory() {
    const auto root = std::filesystem::temp_directory_path();
    const auto tick = static_cast<unsigned long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    for (unsigned int attempt = 0; attempt < 16; ++attempt) {
        const auto candidate = root /
            ("axm-imagemagick-svg-" + std::to_string(process_id_value()) + "-" +
             std::to_string(tick) + "-" + std::to_string(attempt));
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec)) {
            return ScratchDirectory{candidate};
        }
        if (ec && ec != std::errc::file_exists) {
            throw std::runtime_error("cannot create ImageMagick adapter scratch directory: " + ec.message());
        }
    }
    throw std::runtime_error("cannot allocate unique ImageMagick adapter scratch directory");
}

double project_x(float x, int width) {
    return (static_cast<double>(x) * 0.5 + 0.5) * static_cast<double>(width - 1);
}

double project_y(float y, int height) {
    return (1.0 - (static_cast<double>(y) * 0.5 + 0.5)) * static_cast<double>(height - 1);
}

void write_svg(
    const std::filesystem::path& path,
    const axm::render::SceneState& scene,
    int width,
    int height) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot open SVG scratch file: " + path.string());
    }

    out << std::setprecision(17);
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
        << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << ' ' << height
        << "\">\n";
    out << "  <rect x=\"0\" y=\"0\" width=\"" << width << "\" height=\"" << height
        << "\" fill=\"rgb(0,0,0)\"/>\n";

    for (const auto& triangle : scene.triangles) {
        out << "  <polygon points=\"";
        for (std::size_t i = 0; i < 3; ++i) {
            if (i != 0) out << ' ';
            out << project_x(triangle.v[i].position.x, width) << ','
                << project_y(triangle.v[i].position.y, height);
        }
        out << "\" fill=\"rgb("
            << static_cast<int>(triangle.albedo.r) << ','
            << static_cast<int>(triangle.albedo.g) << ','
            << static_cast<int>(triangle.albedo.b) << ")\"/>\n";
    }
    out << "</svg>\n";

    if (!out) {
        throw std::runtime_error("failed while writing SVG scratch file: " + path.string());
    }
}

std::vector<axm::render::Color> load_raw_rgb8(
    const std::filesystem::path& path,
    int width,
    int height) {
    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const std::size_t expected_bytes = pixel_count * 3U;

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("delegated ImageMagick did not create RGB8 output");
    }
    const auto end = in.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) != expected_bytes) {
        throw std::runtime_error(
            "delegated ImageMagick RGB8 byte count does not match requested dimensions");
    }
    in.seekg(0, std::ios::beg);

    std::vector<unsigned char> bytes(expected_bytes);
    in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!in || in.gcount() != static_cast<std::streamsize>(bytes.size())) {
        throw std::runtime_error("failed while reading delegated ImageMagick RGB8 output");
    }

    std::vector<axm::render::Color> pixels(pixel_count);
    for (std::size_t i = 0; i < pixel_count; ++i) {
        pixels[i] = axm::render::Color{
            static_cast<std::uint8_t>(bytes[i * 3U]),
            static_cast<std::uint8_t>(bytes[i * 3U + 1U]),
            static_cast<std::uint8_t>(bytes[i * 3U + 2U])
        };
    }
    return pixels;
}

void write_ppm(
    const std::string& path,
    int width,
    int height,
    const std::vector<axm::render::Color>& pixels) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot open requested output file: " + path);
    }
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto& pixel : pixels) {
        out.put(static_cast<char>(pixel.r));
        out.put(static_cast<char>(pixel.g));
        out.put(static_cast<char>(pixel.b));
    }
    if (!out) {
        throw std::runtime_error("failed while writing requested PPM output: " + path);
    }
}

void require_digest_unchanged(
    const std::string& path,
    std::uint64_t expected,
    const char* label) {
    if (axm::render::continuity_digest64_file(path) != expected) {
        throw std::runtime_error(std::string(label) + " changed while external adapter rendered");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::string convert = imagemagick_executable();
        const DelegatedRendererIdentity delegated = delegated_renderer_identity(convert);

        if (argc == 3 && std::string(argv[1]) == "--capabilities") {
            require_process_success(convert, {"-version"}, "availability check");
            require_digest_unchanged(
                delegated.resolved_executable.string(),
                delegated.executable_digest64,
                "delegated ImageMagick executable");
            const auto capabilities = adapter_capabilities(delegated);
            axm::render::write_render_capabilities_file(argv[2], capabilities);
            std::cout << "renderer=" << capabilities.renderer << "\n";
            std::cout << "renderer_version=" << capabilities.renderer_version << "\n";
            std::cout << "backend=" << capabilities.backend << "\n";
            std::cout << "delegated_renderer=ImageMagick-convert\n";
            std::cout << "delegated_executable=" << delegated.resolved_executable.string() << "\n";
            std::cout << "delegated_executable_digest64="
                      << axm::render::digest64_hex(delegated.executable_digest64) << "\n";
            std::cout << "delegated_renderer_identity_bound_in_receipt=YES_NON_CRYPTOGRAPHIC\n";
            std::cout << "capabilities=" << argv[2] << "\n";
            return 0;
        }

        if (argc != 3) {
            throw std::invalid_argument(
                "usage: imagemagick_svg_renderer REQUEST.axmrender RECEIPT.axmreceipt\n"
                "   or: imagemagick_svg_renderer --capabilities OUTPUT.axmcaps");
        }

        const std::string request_path = argv[1];
        const std::string receipt_path = argv[2];

        const std::uint64_t request_digest =
            axm::render::continuity_digest64_file(request_path);
        const auto request = axm::render::load_render_request_file(request_path);
        require_digest_unchanged(request_path, request_digest, "render request source");

        if (request.backend != adapter_backend) {
            throw std::runtime_error(
                "unsupported backend for ImageMagick SVG adapter: " + request.backend +
                " (supported: " + adapter_backend + ")");
        }
        if (request.output_format != "ppm-rgb8") {
            throw std::runtime_error(
                "unsupported output format for ImageMagick SVG adapter: " + request.output_format);
        }

        const std::uint64_t scene_digest =
            axm::render::continuity_digest64_file(request.scene_path);
        const auto scene = axm::render::load_scene_file(request.scene_path);
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        ScratchDirectory scratch = make_scratch_directory();
        const auto svg_path = scratch.path / "scene.svg";
        const auto raw_path = scratch.path / "pixels.rgb";
        write_svg(svg_path, scene, request.width, request.height);

        require_process_success(
            convert,
            {
                svg_path.string(),
                "-alpha", "off",
                "-depth", "8",
                "rgb:" + raw_path.string()
            },
            "SVG rasterization");
        require_digest_unchanged(
            delegated.resolved_executable.string(),
            delegated.executable_digest64,
            "delegated ImageMagick executable");

        const auto pixels = load_raw_rgb8(raw_path, request.width, request.height);
        require_digest_unchanged(request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        write_ppm(request.output_path, request.width, request.height, pixels);
        const std::uint64_t frame_digest = axm::render::continuity_digest64_rgb8(pixels);
        const std::uint64_t output_digest =
            axm::render::continuity_digest64_file(request.output_path);

        const axm::render::RenderReceipt receipt{
            adapter_renderer,
            delegated.bound_adapter_version,
            request.backend,
            axm::render::scene_contract_version,
            axm::render::render_request_contract_version,
            scene_digest,
            request_digest,
            request.width,
            request.height,
            request.output_format,
            frame_digest,
            output_digest
        };
        axm::render::write_render_receipt_file(receipt_path, receipt);

        std::cout << "renderer=" << adapter_renderer << "\n";
        std::cout << "renderer_version=" << delegated.bound_adapter_version << "\n";
        std::cout << "backend=" << request.backend << "\n";
        std::cout << "delegated_renderer=ImageMagick-convert\n";
        std::cout << "delegated_executable=" << delegated.resolved_executable.string() << "\n";
        std::cout << "delegated_executable_digest64="
                  << axm::render::digest64_hex(delegated.executable_digest64) << "\n";
        std::cout << "delegated_renderer_identity_bound_in_receipt=YES_NON_CRYPTOGRAPHIC\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "translation=axm-scene-v1-to-svg-xy-flat-albedo\n";
        std::cout << "frame_pixels_digest64=" << axm::render::digest64_hex(frame_digest) << "\n";
        std::cout << "output_file_digest64=" << axm::render::digest64_hex(output_digest) << "\n";
        std::cout << "writes_pixels=YES\n";
        std::cout << "writes_receipt=YES\n";
        std::cout << "external_renderer_delegation=YES\n";
        std::cout << "imagemagick_svg_renderer=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
