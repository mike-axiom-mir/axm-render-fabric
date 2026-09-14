#pragma once

#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace axm::render {

inline constexpr std::uint32_t render_receipt_contract_version = 1;

struct RenderReceipt {
    std::string renderer;
    std::string renderer_version;
    std::string backend;
    std::uint32_t scene_contract{};
    std::uint32_t render_request_contract{};
    std::uint64_t scene_source_digest64{};
    std::uint64_t request_source_digest64{};
    int width{};
    int height{};
    std::string output_format;
    std::uint64_t frame_pixels_digest64{};
    std::uint64_t output_file_digest64{};
};

// AXM continuity digest64 v1 deliberately matches the repository's existing
// frame-hash parameters. It is non-cryptographic evidence, not an integrity or
// collision-resistance primitive.
std::uint64_t continuity_digest64_rgb8(const std::vector<Color>& pixels) noexcept;
std::uint64_t continuity_digest64_file(const std::string& path);
std::string digest64_hex(std::uint64_t value);

RenderReceipt load_render_receipt_file(const std::string& path);
void write_render_receipt_file(const std::string& path, const RenderReceipt& receipt);

} // namespace axm::render
