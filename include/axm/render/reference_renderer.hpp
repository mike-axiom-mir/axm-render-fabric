#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace axm::render {

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

class Image {
public:
    Image(int width, int height, Color clear);

    int width() const noexcept;
    int height() const noexcept;
    void set(int x, int y, Color c) noexcept;
    const std::vector<Color>& pixels() const noexcept;
    void write_ppm(const std::string& path) const;

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

class ReferenceRenderer {
public:
    ReferenceRenderer(int width, int height);

    Image render(const std::vector<Triangle>& triangles) const;

private:
    int width_;
    int height_;
};

std::uint64_t fnv1a(const Image& image) noexcept;

} // namespace axm::render
