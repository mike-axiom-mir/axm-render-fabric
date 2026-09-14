#pragma once

#include <cstdint>
#include <string>

namespace axm::render {

inline constexpr std::uint32_t render_request_contract_version = 1;
inline constexpr const char* native_reference_backend = "axm.native.cpu.reference";

struct RenderRequest {
    std::string scene_path;
    std::string backend;
    int width{};
    int height{};
    std::string output_format;
    std::string output_path;
};

RenderRequest load_render_request_file(const std::string& path);

} // namespace axm::render
