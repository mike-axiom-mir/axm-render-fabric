#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr const char* flat_backend = "axm.contract.cpu.flat";
constexpr const char* flat_renderer = "axm.contract.flat-demo";
constexpr const char* flat_renderer_version = "0.1.0";

axm::render::RenderCapabilities flat_capabilities() {
    return {
        flat_renderer,
        flat_renderer_version,
        flat_backend,
        axm::render::scene_contract_version,
        axm::render::render_request_contract_version,
        8192,
        8192,
        {"ppm-rgb8"}
    };
}

struct Point2 {
    double x{};
    double y{};
};

Point2 project_xy(const axm::render::Vec3& p, int width, int height) {
    return {
        (static_cast<double>(p.x) * 0.5 + 0.5) * static_cast<double>(width - 1),
        (1.0 - (static_cast<double>(p.y) * 0.5 + 0.5)) * static_cast<double>(height - 1)
    };
}

double edge(const Point2& a, const Point2& b, const Point2& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

void rasterize_triangle(
    std::vector<axm::render::Color>& pixels,
    int width,
    int height,
    const axm::render::Triangle& triangle) {

    const Point2 a = project_xy(triangle.v[0].position, width, height);
    const Point2 b = project_xy(triangle.v[1].position, width, height);
    const Point2 c = project_xy(triangle.v[2].position, width, height);
    const double area = edge(a, b, c);
    if (std::abs(area) < 1.0e-12) {
        return;
    }

    const int min_x = std::clamp(
        static_cast<int>(std::floor(std::min({a.x, b.x, c.x}))), 0, width - 1);
    const int max_x = std::clamp(
        static_cast<int>(std::ceil(std::max({a.x, b.x, c.x}))), 0, width - 1);
    const int min_y = std::clamp(
        static_cast<int>(std::floor(std::min({a.y, b.y, c.y}))), 0, height - 1);
    const int max_y = std::clamp(
        static_cast<int>(std::ceil(std::max({a.y, b.y, c.y}))), 0, height - 1);

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            const Point2 p{static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5};
            const double w0 = edge(b, c, p);
            const double w1 = edge(c, a, p);
            const double w2 = edge(a, b, p);
            const bool inside = area > 0.0
                ? (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0)
                : (w0 <= 0.0 && w1 <= 0.0 && w2 <= 0.0);
            if (inside) {
                pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                       static_cast<std::size_t>(x)] = triangle.albedo;
            }
        }
    }
}

std::uint64_t digest_pixels(const std::vector<axm::render::Color>& pixels) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    for (const auto& pixel : pixels) {
        hash ^= pixel.r;
        hash *= prime;
        hash ^= pixel.g;
        hash *= prime;
        hash ^= pixel.b;
        hash *= prime;
    }
    return hash;
}

void write_ppm(
    const std::string& path,
    int width,
    int height,
    const std::vector<axm::render::Color>& pixels) {

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot open output file: " + path);
    }
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (const auto& pixel : pixels) {
        out.put(static_cast<char>(pixel.r));
        out.put(static_cast<char>(pixel.g));
        out.put(static_cast<char>(pixel.b));
    }
    if (!out) {
        throw std::runtime_error("failed while writing output file: " + path);
    }
}

bool same_receipt(const axm::render::RenderReceipt& a, const axm::render::RenderReceipt& b) {
    return a.renderer == b.renderer &&
        a.renderer_version == b.renderer_version &&
        a.backend == b.backend &&
        a.scene_contract == b.scene_contract &&
        a.render_request_contract == b.render_request_contract &&
        a.scene_source_digest64 == b.scene_source_digest64 &&
        a.request_source_digest64 == b.request_source_digest64 &&
        a.width == b.width &&
        a.height == b.height &&
        a.output_format == b.output_format &&
        a.frame_pixels_digest64 == b.frame_pixels_digest64 &&
        a.output_file_digest64 == b.output_file_digest64;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--capabilities") {
            const auto capabilities = flat_capabilities();
            axm::render::write_render_capabilities_file(argv[2], capabilities);
            std::cout << "capabilities=" << argv[2] << "\n";
            std::cout << "backend=" << capabilities.backend << "\n";
            std::cout << "scene_contract=" << capabilities.scene_contract << "\n";
            std::cout << "render_request_contract=" << capabilities.render_request_contract << "\n";
            std::cout << "max_dimensions=" << capabilities.max_width << "x" << capabilities.max_height << "\n";
            std::cout << "format=ppm-rgb8\n";
            return 0;
        }

        if (argc != 3) {
            throw std::invalid_argument(
                "usage: contract_flat_renderer REQUEST.axmrender RECEIPT.axmreceipt\n"
                "   or: contract_flat_renderer --capabilities OUTPUT.axmcaps");
        }

        const std::string request_path = argv[1];
        const std::string receipt_path = argv[2];

        const std::uint64_t request_digest_before =
            axm::render::continuity_digest64_file(request_path);
        const auto request = axm::render::load_render_request_file(request_path);
        const std::uint64_t request_digest_after =
            axm::render::continuity_digest64_file(request_path);
        if (request_digest_before != request_digest_after) {
            throw std::runtime_error("render request source changed while flat renderer loaded it");
        }
        if (request.backend != flat_backend) {
            throw std::runtime_error(
                "unsupported backend for flat renderer: " + request.backend +
                " (supported: " + flat_backend + ")");
        }
        if (request.output_format != "ppm-rgb8") {
            throw std::runtime_error(
                "unsupported output format for flat renderer: " + request.output_format);
        }

        const std::uint64_t scene_digest_before =
            axm::render::continuity_digest64_file(request.scene_path);
        const auto scene = axm::render::load_scene_file(request.scene_path);
        const std::uint64_t scene_digest_after =
            axm::render::continuity_digest64_file(request.scene_path);
        if (scene_digest_before != scene_digest_after) {
            throw std::runtime_error("scene source changed while flat renderer loaded it");
        }

        std::vector<axm::render::Color> pixels(
            static_cast<std::size_t>(request.width) * static_cast<std::size_t>(request.height),
            axm::render::Color{0, 0, 0});
        for (const auto& triangle : scene.triangles) {
            rasterize_triangle(pixels, request.width, request.height, triangle);
        }

        write_ppm(request.output_path, request.width, request.height, pixels);
        const std::uint64_t frame_digest = digest_pixels(pixels);
        const std::uint64_t output_digest =
            axm::render::continuity_digest64_file(request.output_path);

        const axm::render::RenderReceipt receipt{
            flat_renderer,
            flat_renderer_version,
            request.backend,
            axm::render::scene_contract_version,
            axm::render::render_request_contract_version,
            scene_digest_after,
            request_digest_after,
            request.width,
            request.height,
            request.output_format,
            frame_digest,
            output_digest
        };
        axm::render::write_render_receipt_file(receipt_path, receipt);
        const auto loaded = axm::render::load_render_receipt_file(receipt_path);
        if (!same_receipt(receipt, loaded)) {
            throw std::runtime_error("flat renderer receipt failed round-trip verification");
        }

        std::cout << "renderer=" << flat_renderer << "\n";
        std::cout << "renderer_version=" << flat_renderer_version << "\n";
        std::cout << "backend=" << request.backend << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "projection=xy-no-depth\n";
        std::cout << "frame_pixels_digest64=" << axm::render::digest64_hex(frame_digest) << "\n";
        std::cout << "output_file_digest64=" << axm::render::digest64_hex(output_digest) << "\n";
        std::cout << "writes_pixels=YES\n";
        std::cout << "writes_receipt=YES\n";
        std::cout << "contract_flat_renderer=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
