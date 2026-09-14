#include "axm/render/reference_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    const std::vector<axm::render::Triangle> scene = {
        {{{{{-0.75f, -0.65f, 0.30f}}, {{0.70f, -0.55f, 0.25f}}, {{0.05f, 0.72f, 0.20f}}}}, {190, 120, 45}}
    };

    const axm::render::ReferenceRenderer renderer(96, 64);
    const auto first = renderer.render(scene);
    const auto second = renderer.render(scene);

    const std::uint64_t first_hash = axm::render::fnv1a(first);
    const std::uint64_t second_hash = axm::render::fnv1a(second);

    const axm::render::Color background{12, 15, 23};
    std::size_t changed_pixels = 0;
    for (const auto& pixel : first.pixels()) {
        if (pixel.r != background.r || pixel.g != background.g || pixel.b != background.b) {
            ++changed_pixels;
        }
    }

    const bool dimensions_ok = first.width() == 96 && first.height() == 64;
    const bool size_ok = first.pixels().size() == 96U * 64U;
    const bool same_run_hash_ok = first_hash == second_hash;
    const bool rasterized_pixels_ok = changed_pixels > 0;

    std::cout << "library_dimensions=" << (dimensions_ok ? "PASS" : "FAIL") << "\n";
    std::cout << "library_pixel_buffer_size=" << (size_ok ? "PASS" : "FAIL") << "\n";
    std::cout << "library_same_run_hash=" << (same_run_hash_ok ? "PASS" : "FAIL") << "\n";
    std::cout << "library_rasterized_pixels=" << (rasterized_pixels_ok ? "PASS" : "FAIL") << "\n";
    std::cout << "library_changed_pixels=" << changed_pixels << "\n";
    std::cout << std::hex << "library_frame_hash=0x" << first_hash << "\n";

    return dimensions_ok && size_ok && same_run_hash_ok && rasterized_pixels_ok ? 0 : 2;
}
