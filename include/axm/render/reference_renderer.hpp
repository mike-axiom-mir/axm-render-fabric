#pragma once

#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace axm::render {

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
