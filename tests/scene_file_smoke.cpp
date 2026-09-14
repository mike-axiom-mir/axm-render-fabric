#include "axm/render/reference_renderer.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: scene_file_smoke PATH\n";
        return 2;
    }

    try {
        const auto scene = axm::render::load_scene_file(argv[1]);
        const bool triangle_count_ok = scene.triangles.size() == 2;

        const axm::render::ReferenceRenderer renderer(320, 180);
        const auto image = renderer.render(scene.triangles);
        const std::uint64_t hash = axm::render::fnv1a(image);
        constexpr std::uint64_t expected_reference_hash = 0x456f404dd94c91daULL;
        const bool reference_hash_ok = hash == expected_reference_hash;

        std::cout << "scene_contract_version=" << axm::render::scene_contract_version << "\n";
        std::cout << "scene_triangle_count=" << scene.triangles.size() << "\n";
        std::cout << "scene_triangle_count_expected=" << (triangle_count_ok ? "PASS" : "FAIL") << "\n";
        std::cout << std::hex << "scene_frame_hash=0x" << hash << "\n";
        std::cout << "scene_reference_hash_expected=0x" << expected_reference_hash << "\n";
        std::cout << std::dec << "scene_reference_hash_match=" << (reference_hash_ok ? "PASS" : "FAIL") << "\n";

        return triangle_count_ok && reference_hash_ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
