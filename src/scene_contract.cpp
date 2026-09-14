#include "axm/render/scene_contract.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace axm::render {
namespace {

constexpr std::size_t max_triangles = 1'000'000;

std::runtime_error parse_error(
    const std::string& path,
    std::size_t line_number,
    const std::string& message) {
    return std::runtime_error(
        "scene parse error in " + path + ":" + std::to_string(line_number) + ": " + message);
}

bool line_is_blank(const std::string& line) {
    return line.find_first_not_of(" \t\r\n") == std::string::npos;
}

void require_no_extra_tokens(
    std::istringstream& input,
    const std::string& path,
    std::size_t line_number) {
    std::string extra;
    if (input >> extra) {
        throw parse_error(path, line_number, "unexpected trailing token: " + extra);
    }
}

} // namespace

SceneState load_scene_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open scene file: " + path);
    }

    SceneState scene;
    bool saw_header = false;
    std::string raw_line;
    std::size_t line_number = 0;

    while (std::getline(input, raw_line)) {
        ++line_number;
        const auto comment = raw_line.find('#');
        const std::string line = raw_line.substr(0, comment);
        if (line_is_blank(line)) continue;

        std::istringstream tokens(line);
        if (!saw_header) {
            std::string magic;
            std::uint32_t version = 0;
            if (!(tokens >> magic >> version) || magic != "AXM_SCENE") {
                throw parse_error(path, line_number, "expected header: AXM_SCENE 1");
            }
            require_no_extra_tokens(tokens, path, line_number);
            if (version != scene_contract_version) {
                throw parse_error(
                    path,
                    line_number,
                    "unsupported scene contract version " + std::to_string(version));
            }
            saw_header = true;
            continue;
        }

        std::string directive;
        tokens >> directive;
        if (directive != "triangle") {
            throw parse_error(path, line_number, "unsupported directive: " + directive);
        }

        float coordinates[9]{};
        int colors[3]{};
        for (float& coordinate : coordinates) {
            if (!(tokens >> coordinate) || !std::isfinite(coordinate)) {
                throw parse_error(path, line_number, "triangle requires nine finite coordinates");
            }
        }
        for (int& channel : colors) {
            if (!(tokens >> channel) || channel < 0 || channel > 255) {
                throw parse_error(path, line_number, "triangle RGB channels must be integers in 0..255");
            }
        }
        require_no_extra_tokens(tokens, path, line_number);

        if (scene.triangles.size() >= max_triangles) {
            throw parse_error(path, line_number, "triangle limit exceeded");
        }

        Triangle triangle{};
        for (std::size_t vertex = 0; vertex < 3; ++vertex) {
            triangle.v[vertex].position = {
                coordinates[vertex * 3],
                coordinates[vertex * 3 + 1],
                coordinates[vertex * 3 + 2]
            };
        }
        triangle.albedo = {
            static_cast<std::uint8_t>(colors[0]),
            static_cast<std::uint8_t>(colors[1]),
            static_cast<std::uint8_t>(colors[2])
        };
        scene.triangles.push_back(triangle);
    }

    if (!saw_header) {
        throw std::runtime_error("scene parse error in " + path + ": missing AXM_SCENE header");
    }
    if (!input.eof()) {
        throw std::runtime_error("failed while reading scene file: " + path);
    }

    return scene;
}

} // namespace axm::render
