#include "axm/render/render_contract.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace axm::render {
namespace {

std::runtime_error parse_error(
    const std::string& path,
    std::size_t line_number,
    const std::string& message) {
    return std::runtime_error(
        "render request parse error in " + path + ":" + std::to_string(line_number) + ": " + message);
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

std::string resolve_relative_path(const std::string& request_path, const std::string& value) {
    const std::filesystem::path candidate(value);
    if (candidate.is_absolute()) return candidate.lexically_normal().string();
    const std::filesystem::path parent = std::filesystem::path(request_path).parent_path();
    return (parent / candidate).lexically_normal().string();
}

} // namespace

RenderRequest load_render_request_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open render request file: " + path);
    }

    RenderRequest request;
    bool saw_header = false;
    bool saw_scene = false;
    bool saw_backend = false;
    bool saw_width = false;
    bool saw_height = false;
    bool saw_format = false;
    bool saw_output = false;

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
            if (!(tokens >> magic >> version) || magic != "AXM_RENDER_REQUEST") {
                throw parse_error(path, line_number, "expected header: AXM_RENDER_REQUEST 1");
            }
            require_no_extra_tokens(tokens, path, line_number);
            if (version != render_request_contract_version) {
                throw parse_error(
                    path,
                    line_number,
                    "unsupported render request contract version " + std::to_string(version));
            }
            saw_header = true;
            continue;
        }

        std::string directive;
        tokens >> directive;
        if (directive == "scene") {
            if (saw_scene) throw parse_error(path, line_number, "duplicate scene directive");
            std::string value;
            if (!(tokens >> value)) throw parse_error(path, line_number, "scene requires one path token");
            require_no_extra_tokens(tokens, path, line_number);
            request.scene_path = resolve_relative_path(path, value);
            saw_scene = true;
        } else if (directive == "backend") {
            if (saw_backend) throw parse_error(path, line_number, "duplicate backend directive");
            if (!(tokens >> request.backend)) throw parse_error(path, line_number, "backend requires one identifier");
            require_no_extra_tokens(tokens, path, line_number);
            saw_backend = true;
        } else if (directive == "width") {
            if (saw_width) throw parse_error(path, line_number, "duplicate width directive");
            if (!(tokens >> request.width)) throw parse_error(path, line_number, "width requires an integer");
            require_no_extra_tokens(tokens, path, line_number);
            if (request.width <= 0 || request.width > 8192) {
                throw parse_error(path, line_number, "width must be in the range 1..8192");
            }
            saw_width = true;
        } else if (directive == "height") {
            if (saw_height) throw parse_error(path, line_number, "duplicate height directive");
            if (!(tokens >> request.height)) throw parse_error(path, line_number, "height requires an integer");
            require_no_extra_tokens(tokens, path, line_number);
            if (request.height <= 0 || request.height > 8192) {
                throw parse_error(path, line_number, "height must be in the range 1..8192");
            }
            saw_height = true;
        } else if (directive == "format") {
            if (saw_format) throw parse_error(path, line_number, "duplicate format directive");
            if (!(tokens >> request.output_format)) throw parse_error(path, line_number, "format requires one identifier");
            require_no_extra_tokens(tokens, path, line_number);
            if (request.output_format != "ppm-rgb8") {
                throw parse_error(path, line_number, "unsupported v1 output format: " + request.output_format);
            }
            saw_format = true;
        } else if (directive == "output") {
            if (saw_output) throw parse_error(path, line_number, "duplicate output directive");
            std::string value;
            if (!(tokens >> value)) throw parse_error(path, line_number, "output requires one path token");
            require_no_extra_tokens(tokens, path, line_number);
            request.output_path = resolve_relative_path(path, value);
            saw_output = true;
        } else {
            throw parse_error(path, line_number, "unsupported directive: " + directive);
        }
    }

    if (!saw_header) throw std::runtime_error("render request parse error in " + path + ": missing AXM_RENDER_REQUEST header");
    if (!input.eof()) throw std::runtime_error("failed while reading render request file: " + path);

    if (!saw_scene || !saw_backend || !saw_width || !saw_height || !saw_format || !saw_output) {
        throw std::runtime_error(
            "render request parse error in " + path +
            ": v1 requires scene, backend, width, height, format, and output exactly once");
    }

    return request;
}

} // namespace axm::render
