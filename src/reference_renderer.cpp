#include "axm/render/reference_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace axm::render {
namespace {

struct ScreenVertex {
    float x;
    float y;
    float z;
};

Vec3 sub(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 normalize(Vec3 v) {
    const float len = std::sqrt(dot(v, v));
    if (len <= 1e-8f) return {0.0f, 0.0f, 1.0f};
    return {v.x / len, v.y / len, v.z / len};
}

float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

Color shade(Color base, float intensity) {
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    auto channel = [intensity](std::uint8_t c) {
        return static_cast<std::uint8_t>(
            std::clamp(std::lround(static_cast<float>(c) * intensity), 0L, 255L));
    };
    return {channel(base.r), channel(base.g), channel(base.b)};
}

} // namespace

Image::Image(int width, int height, Color clear)
    : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("image dimensions must be positive");
    }
    pixels_.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), clear);
}

int Image::width() const noexcept {
    return width_;
}

int Image::height() const noexcept {
    return height_;
}

void Image::set(int x, int y, Color c) noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
    pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = c;
}

const std::vector<Color>& Image::pixels() const noexcept {
    return pixels_;
}

void Image::write_ppm(const std::string& path) const {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("failed to open output file: " + path);
    out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (const auto& p : pixels_) {
        const char rgb[3] = {
            static_cast<char>(p.r),
            static_cast<char>(p.g),
            static_cast<char>(p.b)
        };
        out.write(rgb, 3);
    }
    if (!out) throw std::runtime_error("failed while writing output file: " + path);
}

ReferenceRenderer::ReferenceRenderer(int width, int height)
    : width_(width), height_(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("renderer dimensions must be positive");
    }
}

Image ReferenceRenderer::render(const std::vector<Triangle>& triangles) const {
    Image image(width_, height_, {12, 15, 23});
    std::vector<float> depth(
        static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_),
        std::numeric_limits<float>::infinity());

    const Vec3 light_dir = normalize({-0.35f, 0.5f, 1.0f});

    for (const auto& tri : triangles) {
        const Vec3 e1 = sub(tri.v[1].position, tri.v[0].position);
        const Vec3 e2 = sub(tri.v[2].position, tri.v[0].position);
        const Vec3 normal = normalize(cross(e1, e2));
        const float lambert = std::max(0.0f, dot(normal, light_dir));
        const float lighting = 0.22f + 0.78f * lambert;
        const Color lit = shade(tri.albedo, lighting);

        std::array<ScreenVertex, 3> sv{};
        for (int i = 0; i < 3; ++i) {
            const Vec3 p = tri.v[static_cast<std::size_t>(i)].position;
            sv[static_cast<std::size_t>(i)].x =
                (p.x * 0.5f + 0.5f) * static_cast<float>(width_ - 1);
            sv[static_cast<std::size_t>(i)].y =
                (1.0f - (p.y * 0.5f + 0.5f)) * static_cast<float>(height_ - 1);
            sv[static_cast<std::size_t>(i)].z = p.z;
        }

        const float area = edge(
            sv[0].x, sv[0].y, sv[1].x, sv[1].y, sv[2].x, sv[2].y);
        if (std::abs(area) <= 1e-8f) continue;

        const int min_x = std::max(
            0,
            static_cast<int>(std::floor(std::min({sv[0].x, sv[1].x, sv[2].x}))));
        const int max_x = std::min(
            width_ - 1,
            static_cast<int>(std::ceil(std::max({sv[0].x, sv[1].x, sv[2].x}))));
        const int min_y = std::max(
            0,
            static_cast<int>(std::floor(std::min({sv[0].y, sv[1].y, sv[2].y}))));
        const int max_y = std::min(
            height_ - 1,
            static_cast<int>(std::ceil(std::max({sv[0].y, sv[1].y, sv[2].y}))));

        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const float px = static_cast<float>(x) + 0.5f;
                const float py = static_cast<float>(y) + 0.5f;
                const float w0 = edge(
                    sv[1].x, sv[1].y, sv[2].x, sv[2].y, px, py) / area;
                const float w1 = edge(
                    sv[2].x, sv[2].y, sv[0].x, sv[0].y, px, py) / area;
                const float w2 = 1.0f - w0 - w1;
                constexpr float eps = -1e-6f;
                if (w0 < eps || w1 < eps || w2 < eps) continue;

                const float z = w0 * sv[0].z + w1 * sv[1].z + w2 * sv[2].z;
                const std::size_t idx =
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(width_)
                    + static_cast<std::size_t>(x);
                if (z < depth[idx]) {
                    depth[idx] = z;
                    image.set(x, y, lit);
                }
            }
        }
    }

    return image;
}

std::uint64_t fnv1a(const Image& image) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    for (const auto& p : image.pixels()) {
        const std::uint8_t bytes[3] = {p.r, p.g, p.b};
        for (std::uint8_t b : bytes) {
            hash ^= b;
            hash *= prime;
        }
    }
    return hash;
}

} // namespace axm::render
