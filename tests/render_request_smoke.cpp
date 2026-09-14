#include "axm/render/reference_renderer.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: render_request_smoke PATH\n";
        return 2;
    }

    try {
        const auto request = axm::render::load_render_request_file(argv[1]);
        const auto scene = axm::render::load_scene_file(request.scene_path);
        const axm::render::ReferenceRenderer renderer(request.width, request.height);
        const auto image = renderer.render(scene.triangles);
        const std::uint64_t hash = axm::render::fnv1a(image);
        constexpr std::uint64_t expected_reference_hash = 0x456f404dd94c91daULL;

        const bool backend_ok = request.backend == axm::render::native_reference_backend;
        const bool dimensions_ok = request.width == 320 && request.height == 180;
        const bool format_ok = request.output_format == "ppm-rgb8";
        const bool scene_ok = scene.triangles.size() == 2;
        const bool hash_ok = hash == expected_reference_hash;

        std::cout << "render_request_contract_version=" << axm::render::render_request_contract_version << "\n";
        std::cout << "render_request_backend=" << request.backend << "\n";
        std::cout << "render_request_backend_expected=" << (backend_ok ? "PASS" : "FAIL") << "\n";
        std::cout << "render_request_dimensions_expected=" << (dimensions_ok ? "PASS" : "FAIL") << "\n";
        std::cout << "render_request_format_expected=" << (format_ok ? "PASS" : "FAIL") << "\n";
        std::cout << "render_request_scene_expected=" << (scene_ok ? "PASS" : "FAIL") << "\n";
        std::cout << std::hex << "render_request_frame_hash=0x" << hash << "\n";
        std::cout << "render_request_reference_hash_expected=0x" << expected_reference_hash << "\n";
        std::cout << std::dec << "render_request_reference_hash_match=" << (hash_ok ? "PASS" : "FAIL") << "\n";

        return backend_ok && dimensions_ok && format_ok && scene_ok && hash_ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
