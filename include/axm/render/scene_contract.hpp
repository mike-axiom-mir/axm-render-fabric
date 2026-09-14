#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace axm::render {

inline constexpr std::uint32_t scene_contract_version = 1;

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

struct Vertex {
    Vec3 position;
};

struct Triangle {
    std::array<Vertex, 3> v;
    Color albedo;
};

struct SceneState {
    std::vector<Triangle> triangles;
};

SceneState load_scene_file(const std::string& path);

} // namespace axm::render
