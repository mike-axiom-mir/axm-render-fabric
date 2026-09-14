#pragma once

#include <cstdint>
#include <string>

namespace axm::render {

struct RenderReceiptVerification {
    std::uint64_t scene_source_digest64{};
    std::uint64_t request_source_digest64{};
    std::uint64_t frame_pixels_digest64{};
    std::uint64_t output_file_digest64{};
};

// Re-check an AXM_RENDER_RECEIPT 1 against the request, scene, and ppm-rgb8
// output files that currently exist on disk. This is continuity verification,
// not cryptographic authentication or provenance proof.
RenderReceiptVerification verify_render_receipt_files(
    const std::string& request_path,
    const std::string& receipt_path);

} // namespace axm::render
