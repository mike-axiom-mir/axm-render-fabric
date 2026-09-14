#include "axm/render/render_receipt.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

const char* yes_no(bool value) noexcept {
    return value ? "YES" : "NO";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            throw std::invalid_argument(
                "usage: axm-render-compare LEFT.axmreceipt RIGHT.axmreceipt");
        }

        const auto left = axm::render::load_render_receipt_file(argv[1]);
        const auto right = axm::render::load_render_receipt_file(argv[2]);

        const bool same_scene_contract = left.scene_contract == right.scene_contract;
        const bool same_request_contract =
            left.render_request_contract == right.render_request_contract;
        const bool same_scene_source =
            left.scene_source_digest64 == right.scene_source_digest64;
        const bool same_dimensions =
            left.width == right.width && left.height == right.height;
        const bool same_output_format = left.output_format == right.output_format;

        // Under frozen AXM_RENDER_REQUEST 1, backend and output path are the
        // renderer-specific fields. The remaining declared render inputs that
        // survive into receipt v1 are scene identity, contract versions,
        // dimensions, and output format. Matching those makes the two receipts
        // comparable as executions of the same v1 declared frame intent.
        const bool comparable_v1 =
            same_scene_contract &&
            same_request_contract &&
            same_scene_source &&
            same_dimensions &&
            same_output_format;

        const bool same_pixels =
            left.frame_pixels_digest64 == right.frame_pixels_digest64;
        const bool same_output_file =
            left.output_file_digest64 == right.output_file_digest64;

        std::cout << "left_renderer=" << left.renderer << "\n";
        std::cout << "left_backend=" << left.backend << "\n";
        std::cout << "right_renderer=" << right.renderer << "\n";
        std::cout << "right_backend=" << right.backend << "\n";
        std::cout << "same_scene_contract=" << yes_no(same_scene_contract) << "\n";
        std::cout << "same_render_request_contract=" << yes_no(same_request_contract) << "\n";
        std::cout << "same_scene_source=" << yes_no(same_scene_source) << "\n";
        std::cout << "same_dimensions=" << yes_no(same_dimensions) << "\n";
        std::cout << "same_output_format=" << yes_no(same_output_format) << "\n";
        std::cout << "comparable_v1=" << yes_no(comparable_v1) << "\n";
        std::cout << "same_frame_pixels_digest64=" << yes_no(same_pixels) << "\n";
        std::cout << "same_output_file_digest64=" << yes_no(same_output_file) << "\n";

        if (!comparable_v1) {
            std::cerr
                << "error: receipts do not describe comparable AXM_RENDER_REQUEST 1 frame intent\n";
            return 2;
        }

        std::cout
            << "comparison_boundary=PIXEL_DIGEST_RELATION_REPORTED_NOT_REQUIRED\n";
        std::cout << "render_receipt_compare=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
