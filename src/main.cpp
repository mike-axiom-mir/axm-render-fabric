#include "axm/render/reference_renderer.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <cstdlib>
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

struct Args {
    int width = 320;
    int height = 180;
    std::string output = "frame.ppm";
    std::string scene_path;
    bool self_test = false;
};

static Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--self-test") {
            args.self_test = true;
        } else if (a == "--scene" && i + 1 < argc) {
            args.scene_path = argv[++i];
        } else if (a == "--out" && i + 1 < argc) {
            args.output = argv[++i];
        } else if (a == "--width" && i + 1 < argc) {
            args.width = std::stoi(argv[++i]);
        } else if (a == "--height" && i + 1 < argc) {
            args.height = std::stoi(argv[++i]);
        } else if (a == "--help") {
            std::cout << "AXM Render Fabric reference renderer\n"
                      << "  --scene PATH          load AXM_SCENE v1 state from disk\n"
                      << "  --self-test          render twice and verify identical frame hashes\n"
                      << "  --out PATH           output PPM path (default frame.ppm)\n"
                      << "  --width N            output width (default 320)\n"
                      << "  --height N           output height (default 180)\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + a);
        }
    }
    if (args.width <= 0 || args.height <= 0 || args.width > 8192 || args.height > 8192) {
        throw std::invalid_argument("width/height must be in the range 1..8192");
    }
    return args;
}

} // namespace axm

int main(int argc, char** argv) {
    try {
        const axm::Args args = axm::parse_args(argc, argv);
        const axm::render::ReferenceRenderer renderer(args.width, args.height);
        const auto scene = args.scene_path.empty()
            ? axm::demo_scene()
            : axm::render::load_scene_file(args.scene_path);

        if (args.self_test) {
            const auto a = renderer.render(scene.triangles);
            const auto b = renderer.render(scene.triangles);
            const std::uint64_t hash_a = axm::render::fnv1a(a);
            const std::uint64_t hash_b = axm::render::fnv1a(b);
            std::cout << "scene_source=" << (args.scene_path.empty() ? "builtin" : args.scene_path) << "\n";
            std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
            std::cout << "frame_hash_a=0x" << std::hex << hash_a << "\n";
            std::cout << "frame_hash_b=0x" << std::hex << hash_b << "\n";
            std::cout << "deterministic_same_run=" << (hash_a == hash_b ? "PASS" : "FAIL") << "\n";
            return hash_a == hash_b ? 0 : 2;
        }

        const auto image = renderer.render(scene.triangles);
        image.write_ppm(args.output);
        std::cout << "scene_source=" << (args.scene_path.empty() ? "builtin" : args.scene_path) << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "wrote=" << args.output << "\n";
        std::cout << "frame_hash=0x" << std::hex << axm::render::fnv1a(image) << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
