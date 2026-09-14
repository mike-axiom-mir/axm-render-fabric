#pragma once

#include "axm/render/render_contract.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace axm::render {

inline constexpr std::uint32_t render_capabilities_contract_version = 1;

struct RenderCapabilities {
    std::string renderer;
    std::string renderer_version;
    std::string backend;
    std::uint32_t scene_contract{};
    std::uint32_t render_request_contract{};
    int max_width{};
    int max_height{};
    std::vector<std::string> output_formats;
};

RenderCapabilities load_render_capabilities_file(const std::string& path);
void write_render_capabilities_file(const std::string& path, const RenderCapabilities& capabilities);

// Returns an empty string when the request is compatible with the declared
// capability envelope, otherwise a human-readable incompatibility reason.
std::string render_request_incompatibility(
    const RenderRequest& request,
    const RenderCapabilities& capabilities);

} // namespace axm::render
