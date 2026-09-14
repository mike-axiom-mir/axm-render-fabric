#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace axm::research {

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Transform {
    Vec3 translation;
    float scale{1.0f};
};

struct InstanceState {
    std::uint32_t mesh_id{};
    std::uint32_t material_id{};
    Transform transform;
};

struct ExpandedObject {
    std::uint32_t material_id{};
    std::vector<Vec3> vertices;
};

static std::vector<Vec3> base_mesh() {
    // 12 triangles / 36 vertices. The exact shape is irrelevant to this first
    // residency experiment; what matters is that both strategies expand the
    // same canonical geometry and produce the same visible-geometry digest.
    const Vec3 p[8] = {
        {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
    };
    const int t[12][3] = {
        {0, 1, 2}, {0, 2, 3}, {4, 6, 5}, {4, 7, 6},
        {0, 4, 5}, {0, 5, 1}, {1, 5, 6}, {1, 6, 2},
        {2, 6, 7}, {2, 7, 3}, {3, 7, 4}, {3, 4, 0}
    };

    std::vector<Vec3> out;
    out.reserve(36);
    for (const auto& tri : t) {
        for (int idx : tri) out.push_back(p[idx]);
    }
    return out;
}

static Transform transform_for(std::size_t i) {
    const std::uint32_t n = static_cast<std::uint32_t>(i);
    const float x = static_cast<float>(n % 100U) * 1.25f;
    const float y = static_cast<float>((n / 100U) % 100U) * 1.25f;
    const float z = static_cast<float>((n * 17U) % 23U) * 0.05f;
    const float scale = 0.75f + static_cast<float>((n * 13U) % 7U) * 0.05f;
    return {{x, y, z}, scale};
}

static Vec3 apply(const Vec3& v, const Transform& t) {
    return {
        v.x * t.scale + t.translation.x,
        v.y * t.scale + t.translation.y,
        v.z * t.scale + t.translation.z
    };
}

static bool visible(std::size_t i, std::uint32_t percent) {
    const std::uint32_t n = static_cast<std::uint32_t>(i);
    return ((n * 2654435761U) % 100U) < percent;
}

static std::uint64_t mix(std::uint64_t h, std::uint64_t v) {
    constexpr std::uint64_t prime = 1099511628211ULL;
    for (int i = 0; i < 8; ++i) {
        h ^= static_cast<std::uint8_t>((v >> (i * 8)) & 0xffU);
        h *= prime;
    }
    return h;
}

static std::uint64_t q(float v) {
    const auto scaled = static_cast<std::int64_t>(std::llround(static_cast<double>(v) * 1000.0));
    return static_cast<std::uint64_t>(scaled);
}

static std::uint64_t digest_vertex(std::uint64_t h, const Vec3& v) {
    h = mix(h, q(v.x));
    h = mix(h, q(v.y));
    return mix(h, q(v.z));
}

struct Metrics {
    std::size_t resident_bytes{};
    std::size_t state_canonical_bytes{};
    std::size_t state_peak_modeled_bytes{};
    std::size_t visible_objects{};
    std::uint64_t resident_digest{};
    std::uint64_t state_digest{};
};

static Metrics run(std::size_t object_count, std::uint32_t visible_percent) {
    if (object_count == 0) throw std::invalid_argument("object count must be positive");
    if (visible_percent == 0 || visible_percent > 100) {
        throw std::invalid_argument("visible percent must be in 1..100");
    }

    const auto mesh = base_mesh();

    std::vector<ExpandedObject> resident;
    resident.reserve(object_count);

    std::vector<InstanceState> state;
    state.reserve(object_count);

    for (std::size_t i = 0; i < object_count; ++i) {
        const Transform tr = transform_for(i);
        const auto material = static_cast<std::uint32_t>((i * 7U) % 11U);

        ExpandedObject expanded;
        expanded.material_id = material;
        expanded.vertices.reserve(mesh.size());
        for (const auto& v : mesh) expanded.vertices.push_back(apply(v, tr));
        resident.push_back(std::move(expanded));

        state.push_back({0U, material, tr});
    }

    Metrics m{};
    m.resident_bytes = resident.capacity() * sizeof(ExpandedObject);
    for (const auto& obj : resident) {
        m.resident_bytes += obj.vertices.capacity() * sizeof(Vec3);
    }

    m.state_canonical_bytes = state.capacity() * sizeof(InstanceState)
        + mesh.capacity() * sizeof(Vec3);
    // The state path below expands one base mesh at a time and does not retain
    // those expanded vertices. Count one mesh-sized scratch allocation as the
    // modeled peak working set beyond canonical state.
    m.state_peak_modeled_bytes = m.state_canonical_bytes + mesh.size() * sizeof(Vec3);

    constexpr std::uint64_t offset = 1469598103934665603ULL;
    m.resident_digest = offset;
    m.state_digest = offset;

    for (std::size_t i = 0; i < object_count; ++i) {
        if (!visible(i, visible_percent)) continue;
        ++m.visible_objects;

        const auto& obj = resident[i];
        m.resident_digest = mix(m.resident_digest, obj.material_id);
        for (const auto& v : obj.vertices) {
            m.resident_digest = digest_vertex(m.resident_digest, v);
        }

        const auto& inst = state[i];
        m.state_digest = mix(m.state_digest, inst.material_id);
        for (const auto& v : mesh) {
            m.state_digest = digest_vertex(m.state_digest, apply(v, inst.transform));
        }
    }

    return m;
}

struct Args {
    std::size_t objects = 10000;
    std::uint32_t visible_percent = 10;
    bool self_test = false;
};

static Args parse_args(int argc, char** argv) {
    Args args;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--self-test") {
            args.self_test = true;
        } else if (a == "--objects" && i + 1 < argc) {
            args.objects = static_cast<std::size_t>(std::stoull(argv[++i]));
        } else if (a == "--visible-percent" && i + 1 < argc) {
            args.visible_percent = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if (a == "--help") {
            std::cout << "AXM state-native rendering residency experiment\n"
                      << "  --objects N            synthetic object count (default 10000)\n"
                      << "  --visible-percent N    synthetic visible working set 1..100 (default 10)\n"
                      << "  --self-test            require equivalent visible-geometry digest and lower modeled state residency\n";
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown or incomplete argument: " + a);
        }
    }
    if (args.objects == 0 || args.objects > 1000000) {
        throw std::invalid_argument("objects must be in 1..1000000");
    }
    if (args.visible_percent == 0 || args.visible_percent > 100) {
        throw std::invalid_argument("visible-percent must be in 1..100");
    }
    return args;
}

} // namespace axm::research

int main(int argc, char** argv) {
    try {
        const auto args = axm::research::parse_args(argc, argv);
        const auto m = axm::research::run(args.objects, args.visible_percent);
        const bool equivalent = m.resident_digest == m.state_digest;
        const bool lower_modeled_residency = m.state_peak_modeled_bytes < m.resident_bytes;
        const double ratio = static_cast<double>(m.resident_bytes)
            / static_cast<double>(m.state_peak_modeled_bytes);

        std::cout << "objects=" << args.objects << "\n";
        std::cout << "visible_objects=" << m.visible_objects << "\n";
        std::cout << "resident_modeled_owned_bytes=" << m.resident_bytes << "\n";
        std::cout << "state_canonical_modeled_owned_bytes=" << m.state_canonical_bytes << "\n";
        std::cout << "state_peak_modeled_owned_bytes=" << m.state_peak_modeled_bytes << "\n";
        std::cout << std::fixed << std::setprecision(3)
                  << "modeled_residency_ratio_resident_over_state=" << ratio << "\n";
        std::cout << std::hex;
        std::cout << "resident_visible_digest=0x" << m.resident_digest << "\n";
        std::cout << "state_visible_digest=0x" << m.state_digest << "\n";
        std::cout << std::dec;
        std::cout << "equivalent_visible_expansion=" << (equivalent ? "PASS" : "FAIL") << "\n";
        std::cout << "lower_modeled_state_residency=" << (lower_modeled_residency ? "PASS" : "FAIL") << "\n";
        std::cout << "truth_boundary=synthetic_modeled_owned_bytes_not_process_RSS_or_GPU_VRAM\n";

        if (args.self_test) return equivalent && lower_modeled_residency ? 0 : 2;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
