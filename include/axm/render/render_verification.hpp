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

// Perform the same replay verification, then corroborate the receipt's declared
// renderer identity/version/backend and frozen v1 support against one current
// AXM_RENDER_CAPABILITIES 1 manifest. The capability manifest is external
// corroboration: receipt v1 does not embed or authenticate its bytes.
RenderReceiptVerification verify_render_receipt_files_with_capabilities(
    const std::string& request_path,
    const std::string& receipt_path,
    const std::string& capabilities_path);

} // namespace axm::render
