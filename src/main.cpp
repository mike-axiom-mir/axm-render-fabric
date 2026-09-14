#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace axm {

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
    Image(int width, int height, Color clear)
        : width_(width), height_(height), pixels_(static_cast<std::size_t>(width) * height, clear) {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("image dimensions must be positive");
        }
    }

    int width() const { return width_; }
    int height() const { return height_; }

    void set(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
        pixels_[static_cast<std::size_t>(y) * width_ + x] = c;
    }

    const std::vector<Color>& pixels() const { return pixels_; }

    void write_ppm(const std::string& path) const {
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

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

struct ScreenVertex {
    float x;
    float y;
    float z;
};

static Vec3 sub(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static Vec3 cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 normalize(Vec3 v) {
    const float len = std::sqrt(dot(v, v));
    if (len <= 1e-8f) return {0.0f, 0.0f, 1.0f};
    return {v.x / len, v.y / len, v.z / len};
}

static float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

static Color shade(Color base, float intensity) {
    intensity = std::clamp(intensity, 0.0f, 1.0f);
    auto channel = [intensity](std::uint8_t c) {
        return static_cast<std::uint8_t>(std::clamp(std::lround(static_cast<float>(c) * intensity), 0L, 255L));
    };
    return {channel(base.r), channel(base.g), channel(base.b)};
}

class ReferenceRenderer {
public:
    ReferenceRenderer(int width, int height)
        : width_(width), height_(height) {}

    Image render(const std::vector<Triangle>& triangles) const {
        Image image(width_, height_, {12, 15, 23});
        std::vector<float> depth(static_cast<std::size_t>(width_) * height_, std::numeric_limits<float>::infinity());

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
                const Vec3 p = tri.v[i].position;
                sv[i].x = (p.x * 0.5f + 0.5f) * static_cast<float>(width_ - 1);
                sv[i].y = (1.0f - (p.y * 0.5f + 0.5f)) * static_cast<float>(height_ - 1);
                sv[i].z = p.z;
            }

            const float area = edge(sv[0].x, sv[0].y, sv[1].x, sv[1].y, sv[2].x, sv[2].y);
            if (std::abs(area) <= 1e-8f) continue;

            const int min_x = std::max(0, static_cast<int>(std::floor(std::min({sv[0].x, sv[1].x, sv[2].x}))));
            const int max_x = std::min(width_ - 1, static_cast<int>(std::ceil(std::max({sv[0].x, sv[1].x, sv[2].x}))));
            const int min_y = std::max(0, static_cast<int>(std::floor(std::min({sv[0].y, sv[1].y, sv[2].y}))));
            const int max_y = std::min(height_ - 1, static_cast<int>(std::ceil(std::max({sv[0].y, sv[1].y, sv[2].y}))));

            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {
                    const float px = static_cast<float>(x) + 0.5f;
                    const float py = static_cast<float>(y) + 0.5f;
                    const float w0 = edge(sv[1].x, sv[1].y, sv[2].x, sv[2].y, px, py) / area;
                    const float w1 = edge(sv[2].x, sv[2].y, sv[0].x, sv[0].y, px, py) / area;
                    const float w2 = 1.0f - w0 - w1;
                    constexpr float eps = -1e-6f;
                    if (w0 < eps || w1 < eps || w2 < eps) continue;

                    const float z = w0 * sv[0].z + w1 * sv[1].z + w2 * sv[2].z;
                    const std::size_t idx = static_cast<std::size_t>(y) * width_ + x;
                    if (z < depth[idx]) {
                        depth[idx] = z;
                        image.set(x, y, lit);
                    }
                }
            }
        }

        return image;
    }

private:
    int width_;
    int height_;
};

static std::uint64_t fnv1a(const Image& image) {
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

static std::vector<Triangle> demo_scene() {
    return {
        {{{{{-0.82f, -0.62f, 0.45f}}, {{0.72f, -0.55f, 0.35f}}, {{-0.05f, 0.78f, 0.40f}}}}, {226, 68, 92}},
        {{{{{-0.45f, -0.25f, 0.20f}}, {{0.82f, -0.12f, 0.18f}}, {{0.38f, 0.66f, 0.16f}}}}, {74, 182, 255}}
    };
}

struct Args {
    int width = 320;
    int height = 180;
    std::string output = "frame.ppm";
    bool self_test = false;
};

static Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--self-test") {
            args.self_test = true;
        } else if (a == "--out" && i + 1 < argc) {
            args.output = argv[++i];
        } else if (a == "--width" && i + 1 < argc) {
            args.width = std::stoi(argv[++i]);
        } else if (a == "--height" && i + 1 < argc) {
            args.height = std::stoi(argv[++i]);
        } else if (a == "--help") {
            std::cout << "AXM Render Fabric reference renderer\n"
                      << "  --self-test          render twice and verify identical frame hashes\n"
                      << "  --out PATH           output PPM path (default frame.ppm)\n"
                      << "  --width N            output width (default 320)\n"
                      << "  --height N           output height (default 180)\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + a);
        }
    }
    if (args.width <= 0 || args.height <= 0 || args.width > 8192 || args.height > 8192) {
        throw std::invalid_argument("width/height must be in the range 1..8192");
    }
    return args;
}

} // namespace axm

int main(int argc, char** argv) {
    try {
        const axm::Args args = axm::parse_args(argc, argv);
        const axm::ReferenceRenderer renderer(args.width, args.height);
        const auto scene = axm::demo_scene();

        if (args.self_test) {
            const auto a = renderer.render(scene);
            const auto b = renderer.render(scene);
            const std::uint64_t hash_a = axm::fnv1a(a);
            const std::uint64_t hash_b = axm::fnv1a(b);
            std::cout << "frame_hash_a=0x" << std::hex << hash_a << "\n";
            std::cout << "frame_hash_b=0x" << std::hex << hash_b << "\n";
            std::cout << "deterministic_same_run=" << (hash_a == hash_b ? "PASS" : "FAIL") << "\n";
            return hash_a == hash_b ? 0 : 2;
        }

        const auto image = renderer.render(scene);
        image.write_ppm(args.output);
        std::cout << "wrote=" << args.output << "\n";
        std::cout << "frame_hash=0x" << std::hex << axm::fnv1a(image) << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
