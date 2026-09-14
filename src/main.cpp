#include "axm/render/reference_renderer.hpp"
#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace axm {

static render::SceneState demo_scene() {
    return {{
        {{{{{-0.82f, -0.62f, 0.45f}}, {{0.72f, -0.55f, 0.35f}}, {{-0.05f, 0.78f, 0.40f}}}}, {226, 68, 92}},
        {{{{{-0.45f, -0.25f, 0.20f}}, {{0.82f, -0.12f, 0.18f}}, {{0.38f, 0.66f, 0.16f}}}}, {74, 182, 255}}
    }};
}

static render::RenderCapabilities native_capabilities() {
    return {
        render::native_reference_backend,
        render::native_reference_renderer_version,
        render::native_reference_backend,
        render::scene_contract_version,
        render::render_request_contract_version,
        8192,
        8192,
        {"ppm-rgb8"}
    };
}

struct Args {
    int width = 320;
    int height = 180;
    std::string output = "frame.ppm";
    std::string scene_path;
    std::string request_path;
    std::string receipt_path;
    std::string capabilities_path;
    bool self_test = false;
    bool width_set = false;
    bool height_set = false;
    bool output_set = false;
    bool scene_set = false;
    bool receipt_set = false;
    bool capabilities_set = false;
};

static Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--self-test") {
            args.self_test = true;
        } else if (a == "--request" && i + 1 < argc) {
            args.request_path = argv[++i];
        } else if (a == "--receipt" && i + 1 < argc) {
            args.receipt_path = argv[++i];
            args.receipt_set = true;
        } else if (a == "--capabilities" && i + 1 < argc) {
            args.capabilities_path = argv[++i];
            args.capabilities_set = true;
        } else if (a == "--scene" && i + 1 < argc) {
            args.scene_path = argv[++i];
            args.scene_set = true;
        } else if (a == "--out" && i + 1 < argc) {
            args.output = argv[++i];
            args.output_set = true;
        } else if (a == "--width" && i + 1 < argc) {
            args.width = std::stoi(argv[++i]);
            args.width_set = true;
        } else if (a == "--height" && i + 1 < argc) {
            args.height = std::stoi(argv[++i]);
            args.height_set = true;
        } else if (a == "--help") {
            std::cout << "AXM Render Fabric reference renderer\n"
                      << "  --request PATH        load AXM_RENDER_REQUEST v1 (scene/backend/dimensions/format/output)\n"
                      << "  --receipt PATH        write AXM_RENDER_RECEIPT v1 for a request render\n"
                      << "  --capabilities PATH   write AXM_RENDER_CAPABILITIES v1 and exit\n"
                      << "  --scene PATH          load AXM_SCENE v1 state from disk\n"
                      << "  --self-test           render twice and verify identical frame hashes\n"
                      << "  --out PATH            output PPM path (default frame.ppm)\n"
                      << "  --width N             output width (default 320)\n"
                      << "  --height N            output height (default 180)\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + a);
        }
    }
    if (!args.request_path.empty() && (args.scene_set || args.output_set || args.width_set || args.height_set)) {
        throw std::invalid_argument("--request cannot be combined with --scene, --out, --width, or --height");
    }
    if (args.receipt_set && args.request_path.empty()) {
        throw std::invalid_argument("--receipt currently requires --request so source evidence is explicit");
    }
    if (args.receipt_set && args.self_test) {
        throw std::invalid_argument("--receipt cannot be combined with --self-test because self-test does not write output");
    }
    if (args.capabilities_set &&
        (!args.request_path.empty() || args.receipt_set || args.scene_set || args.output_set ||
         args.width_set || args.height_set || args.self_test)) {
        throw std::invalid_argument("--capabilities is a discovery-only operation and cannot be combined with rendering arguments");
    }
    if (args.width <= 0 || args.height <= 0 || args.width > 8192 || args.height > 8192) {
        throw std::invalid_argument("width/height must be in the range 1..8192");
    }
    return args;
}

static std::filesystem::path normalized_absolute(const std::string& path) {
    return std::filesystem::absolute(std::filesystem::path(path)).lexically_normal();
}

} // namespace axm

int main(int argc, char** argv) {
    try {
        const axm::Args args = axm::parse_args(argc, argv);

        if (args.capabilities_set) {
            const auto capabilities = axm::native_capabilities();
            axm::render::write_render_capabilities_file(args.capabilities_path, capabilities);
            std::cout << "capabilities=" << args.capabilities_path << "\n";
            std::cout << "backend=" << capabilities.backend << "\n";
            std::cout << "scene_contract=" << capabilities.scene_contract << "\n";
            std::cout << "render_request_contract=" << capabilities.render_request_contract << "\n";
            std::cout << "max_dimensions=" << capabilities.max_width << "x" << capabilities.max_height << "\n";
            std::cout << "format=ppm-rgb8\n";
            return 0;
        }

        int width = args.width;
        int height = args.height;
        std::string output = args.output;
        std::string scene_path = args.scene_path;
        std::string backend = axm::render::native_reference_backend;
        std::string output_format = "ppm-rgb8";
        std::uint64_t request_source_digest64 = 0;
        std::uint64_t scene_source_digest64 = 0;

        if (!args.request_path.empty()) {
            if (args.receipt_set) {
                request_source_digest64 = axm::render::continuity_digest64_file(args.request_path);
            }
            const auto request = axm::render::load_render_request_file(args.request_path);
            if (args.receipt_set &&
                axm::render::continuity_digest64_file(args.request_path) != request_source_digest64) {
                throw std::runtime_error("render request source changed while it was being loaded");
            }
            if (request.backend != axm::render::native_reference_backend) {
                throw std::runtime_error(
                    "unsupported backend for native executable: " + request.backend +
                    " (supported: " + axm::render::native_reference_backend + ")");
            }
            width = request.width;
            height = request.height;
            output = request.output_path;
            scene_path = request.scene_path;
            backend = request.backend;
            output_format = request.output_format;
        }

        if (args.receipt_set && axm::normalized_absolute(args.receipt_path) == axm::normalized_absolute(output)) {
            throw std::invalid_argument("receipt path must differ from rendered output path");
        }

        const axm::render::ReferenceRenderer renderer(width, height);
        if (args.receipt_set) {
            scene_source_digest64 = axm::render::continuity_digest64_file(scene_path);
        }
        const auto scene = scene_path.empty()
            ? axm::demo_scene()
            : axm::render::load_scene_file(scene_path);
        if (args.receipt_set &&
            axm::render::continuity_digest64_file(scene_path) != scene_source_digest64) {
            throw std::runtime_error("scene source changed while it was being loaded");
        }

        if (args.self_test) {
            const auto a = renderer.render(scene.triangles);
            const auto b = renderer.render(scene.triangles);
            const std::uint64_t hash_a = axm::render::fnv1a(a);
            const std::uint64_t hash_b = axm::render::fnv1a(b);
            std::cout << "render_request_source=" << (args.request_path.empty() ? "cli" : args.request_path) << "\n";
            std::cout << "backend=" << backend << "\n";
            std::cout << "output_format=" << output_format << "\n";
            std::cout << "scene_source=" << (scene_path.empty() ? "builtin" : scene_path) << "\n";
            std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
            std::cout << "frame_hash_a=0x" << std::hex << hash_a << "\n";
            std::cout << "frame_hash_b=0x" << std::hex << hash_b << "\n";
            std::cout << "deterministic_same_run=" << (hash_a == hash_b ? "PASS" : "FAIL") << "\n";
            return hash_a == hash_b ? 0 : 2;
        }

        const auto image = renderer.render(scene.triangles);
        const std::uint64_t frame_digest64 = axm::render::fnv1a(image);
        image.write_ppm(output);

        if (args.receipt_set) {
            if (axm::render::continuity_digest64_file(args.request_path) != request_source_digest64) {
                throw std::runtime_error("render request source changed before receipt emission");
            }
            if (axm::render::continuity_digest64_file(scene_path) != scene_source_digest64) {
                throw std::runtime_error("scene source changed before receipt emission");
            }

            const axm::render::RenderReceipt receipt{
                axm::render::native_reference_backend,
                axm::render::native_reference_renderer_version,
                backend,
                axm::render::scene_contract_version,
                axm::render::render_request_contract_version,
                scene_source_digest64,
                request_source_digest64,
                width,
                height,
                output_format,
                frame_digest64,
                axm::render::continuity_digest64_file(output)
            };
            axm::render::write_render_receipt_file(args.receipt_path, receipt);
        }

        std::cout << "render_request_source=" << (args.request_path.empty() ? "cli" : args.request_path) << "\n";
        std::cout << "backend=" << backend << "\n";
        std::cout << "output_format=" << output_format << "\n";
        std::cout << "scene_source=" << (scene_path.empty() ? "builtin" : scene_path) << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "wrote=" << output << "\n";
        std::cout << "frame_hash=0x" << std::hex << frame_digest64 << "\n";
        if (args.receipt_set) {
            std::cout << "receipt=" << args.receipt_path << "\n";
            std::cout << "scene_source_digest64=" << axm::render::digest64_hex(scene_source_digest64) << "\n";
            std::cout << "request_source_digest64=" << axm::render::digest64_hex(request_source_digest64) << "\n";
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
