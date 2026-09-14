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
// collision-resistance primitive. Renderer bodies should use this helper for
// AXM_RENDER_RECEIPT 1 frame_pixels_digest64 so the frozen digest semantics do
// not drift between implementations.
inline std::uint64_t continuity_digest64_rgb8(const std::vector<Color>& pixels) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    for (const auto& pixel : pixels) {
        const std::uint8_t bytes[3] = {pixel.r, pixel.g, pixel.b};
        for (const std::uint8_t byte : bytes) {
            hash ^= byte;
            hash *= prime;
        }
    }
    return hash;
}

std::uint64_t continuity_digest64_file(const std::string& path);
std::string digest64_hex(std::uint64_t value);

RenderReceipt load_render_receipt_file(const std::string& path);
void write_render_receipt_file(const std::string& path, const RenderReceipt& receipt);

} // namespace axm::render
