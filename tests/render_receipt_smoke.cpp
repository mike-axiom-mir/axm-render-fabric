#include "axm/render/reference_renderer.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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
    if (argc != 5) {
        std::cerr << "usage: render_receipt_smoke REQUEST SCENE OUTPUT RECEIPT\n";
        return 2;
    }

    try {
        const std::string request_path = argv[1];
        const std::string scene_path = argv[2];
        const std::string output_path = argv[3];
        const std::string receipt_path = argv[4];

        const auto request = axm::render::load_render_request_file(request_path);
        const auto scene = axm::render::load_scene_file(scene_path);
        const axm::render::ReferenceRenderer renderer(request.width, request.height);
        const auto image = renderer.render(scene.triangles);
        image.write_ppm(output_path);

        const std::uint64_t frame_digest64 = axm::render::fnv1a(image);
        constexpr std::uint64_t expected_reference_frame_digest64 = 0x456f404dd94c91daULL;
        constexpr std::uint64_t expected_reference_output_digest64 = 0x7e33cf9168a97ab0ULL;

        const axm::render::RenderReceipt expected{
            axm::render::native_reference_backend,
            axm::render::native_reference_renderer_version,
            request.backend,
            axm::render::scene_contract_version,
            axm::render::render_request_contract_version,
            axm::render::continuity_digest64_file(scene_path),
            axm::render::continuity_digest64_file(request_path),
            request.width,
            request.height,
            request.output_format,
            frame_digest64,
            axm::render::continuity_digest64_file(output_path)
        };

        axm::render::write_render_receipt_file(receipt_path, expected);
        const auto loaded = axm::render::load_render_receipt_file(receipt_path);

        const bool round_trip_ok = same_receipt(expected, loaded);
        const bool frame_continuity_ok = frame_digest64 == expected_reference_frame_digest64;
        const bool output_continuity_ok =
            expected.output_file_digest64 == expected_reference_output_digest64;

        const std::string unsupported_path = receipt_path + ".unsupported-v2";
        {
            std::ofstream unsupported(unsupported_path, std::ios::binary);
            unsupported << "AXM_RENDER_RECEIPT 2\n";
        }
        bool unsupported_version_rejected = false;
        try {
            static_cast<void>(axm::render::load_render_receipt_file(unsupported_path));
        } catch (const std::exception&) {
            unsupported_version_rejected = true;
        }

        bool comment_marker_token_rejected = false;
        try {
            auto invalid = expected;
            invalid.renderer = "invalid#renderer";
            axm::render::write_render_receipt_file(receipt_path + ".invalid-token", invalid);
        } catch (const std::exception&) {
            comment_marker_token_rejected = true;
        }

        std::cout << "render_receipt_contract_version="
                  << axm::render::render_receipt_contract_version << "\n";
        std::cout << "receipt_round_trip=" << (round_trip_ok ? "PASS" : "FAIL") << "\n";
        std::cout << "receipt_unsupported_version_rejected="
                  << (unsupported_version_rejected ? "PASS" : "FAIL") << "\n";
        std::cout << "receipt_comment_marker_token_rejected="
                  << (comment_marker_token_rejected ? "PASS" : "FAIL") << "\n";
        std::cout << "receipt_scene_source_digest64="
                  << axm::render::digest64_hex(expected.scene_source_digest64) << "\n";
        std::cout << "receipt_request_source_digest64="
                  << axm::render::digest64_hex(expected.request_source_digest64) << "\n";
        std::cout << "receipt_frame_pixels_digest64="
                  << axm::render::digest64_hex(expected.frame_pixels_digest64) << "\n";
        std::cout << "receipt_output_file_digest64="
                  << axm::render::digest64_hex(expected.output_file_digest64) << "\n";
        std::cout << "receipt_reference_frame_continuity="
                  << (frame_continuity_ok ? "PASS" : "FAIL") << "\n";
        std::cout << "receipt_reference_output_continuity="
                  << (output_continuity_ok ? "PASS" : "FAIL") << "\n";

        return round_trip_ok && unsupported_version_rejected &&
            comment_marker_token_rejected && frame_continuity_ok &&
            output_continuity_ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
