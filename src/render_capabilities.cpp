#include "axm/render/render_capabilities.hpp"

#include "axm/render/scene_contract.hpp"

#include <algorithm>
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
        "render capabilities parse error in " + path + ":" +
        std::to_string(line_number) + ": " + message);
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

void require_capability_dimension(
    int value,
    const std::string& path,
    std::size_t line_number,
    const char* name) {
    if (value <= 0 || value > 8192) {
        throw parse_error(
            path,
            line_number,
            std::string(name) + " must be in the range 1..8192 for request contract v1");
    }
}

} // namespace

RenderCapabilities load_render_capabilities_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open render capabilities file: " + path);
    }

    RenderCapabilities capabilities;
    bool saw_header = false;
    bool saw_renderer = false;
    bool saw_renderer_version = false;
    bool saw_backend = false;
    bool saw_scene_contract = false;
    bool saw_request_contract = false;
    bool saw_max_width = false;
    bool saw_max_height = false;

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
            if (!(tokens >> magic >> version) || magic != "AXM_RENDER_CAPABILITIES") {
                throw parse_error(path, line_number, "expected header: AXM_RENDER_CAPABILITIES 1");
            }
            require_no_extra_tokens(tokens, path, line_number);
            if (version != render_capabilities_contract_version) {
                throw parse_error(
                    path,
                    line_number,
                    "unsupported render capabilities contract version " +
                        std::to_string(version));
            }
            saw_header = true;
            continue;
        }

        std::string directive;
        tokens >> directive;
        if (directive == "renderer") {
            if (saw_renderer) throw parse_error(path, line_number, "duplicate renderer directive");
            if (!(tokens >> capabilities.renderer)) {
                throw parse_error(path, line_number, "renderer requires one identifier");
            }
            require_no_extra_tokens(tokens, path, line_number);
            saw_renderer = true;
        } else if (directive == "renderer_version") {
            if (saw_renderer_version) {
                throw parse_error(path, line_number, "duplicate renderer_version directive");
            }
            if (!(tokens >> capabilities.renderer_version)) {
                throw parse_error(path, line_number, "renderer_version requires one token");
            }
            require_no_extra_tokens(tokens, path, line_number);
            saw_renderer_version = true;
        } else if (directive == "backend") {
            if (saw_backend) throw parse_error(path, line_number, "duplicate backend directive");
            if (!(tokens >> capabilities.backend)) {
                throw parse_error(path, line_number, "backend requires one identifier");
            }
            require_no_extra_tokens(tokens, path, line_number);
            saw_backend = true;
        } else if (directive == "scene_contract") {
            if (saw_scene_contract) {
                throw parse_error(path, line_number, "duplicate scene_contract directive");
            }
            if (!(tokens >> capabilities.scene_contract)) {
                throw parse_error(path, line_number, "scene_contract requires an integer version");
            }
            require_no_extra_tokens(tokens, path, line_number);
            saw_scene_contract = true;
        } else if (directive == "render_request_contract") {
            if (saw_request_contract) {
                throw parse_error(path, line_number, "duplicate render_request_contract directive");
            }
            if (!(tokens >> capabilities.render_request_contract)) {
                throw parse_error(
                    path, line_number, "render_request_contract requires an integer version");
            }
            require_no_extra_tokens(tokens, path, line_number);
            saw_request_contract = true;
        } else if (directive == "max_width") {
            if (saw_max_width) throw parse_error(path, line_number, "duplicate max_width directive");
            if (!(tokens >> capabilities.max_width)) {
                throw parse_error(path, line_number, "max_width requires an integer");
            }
            require_no_extra_tokens(tokens, path, line_number);
            require_capability_dimension(capabilities.max_width, path, line_number, "max_width");
            saw_max_width = true;
        } else if (directive == "max_height") {
            if (saw_max_height) throw parse_error(path, line_number, "duplicate max_height directive");
            if (!(tokens >> capabilities.max_height)) {
                throw parse_error(path, line_number, "max_height requires an integer");
            }
            require_no_extra_tokens(tokens, path, line_number);
            require_capability_dimension(capabilities.max_height, path, line_number, "max_height");
            saw_max_height = true;
        } else if (directive == "format") {
            std::string format;
            if (!(tokens >> format)) {
                throw parse_error(path, line_number, "format requires one identifier");
            }
            require_no_extra_tokens(tokens, path, line_number);
            if (std::find(
                    capabilities.output_formats.begin(),
                    capabilities.output_formats.end(),
                    format) != capabilities.output_formats.end()) {
                throw parse_error(path, line_number, "duplicate format directive: " + format);
            }
            capabilities.output_formats.push_back(format);
        } else {
            throw parse_error(path, line_number, "unsupported directive: " + directive);
        }
    }

    if (!saw_header) {
        throw std::runtime_error(
            "render capabilities parse error in " + path +
            ": missing AXM_RENDER_CAPABILITIES header");
    }
    if (!input.eof()) {
        throw std::runtime_error("failed while reading render capabilities file: " + path);
    }

    if (!saw_renderer || !saw_renderer_version || !saw_backend ||
        !saw_scene_contract || !saw_request_contract || !saw_max_width ||
        !saw_max_height || capabilities.output_formats.empty()) {
        throw std::runtime_error(
            "render capabilities parse error in " + path +
            ": v1 requires renderer, renderer_version, backend, scene_contract, "
            "render_request_contract, max_width, max_height, and at least one format");
    }

    return capabilities;
}

void write_render_capabilities_file(
    const std::string& path,
    const RenderCapabilities& capabilities) {
    if (capabilities.renderer.empty() || capabilities.renderer_version.empty() ||
        capabilities.backend.empty()) {
        throw std::invalid_argument("render capabilities identity fields must not be empty");
    }
    if (capabilities.scene_contract == 0 || capabilities.render_request_contract == 0) {
        throw std::invalid_argument("render capabilities contract versions must be positive");
    }
    if (capabilities.max_width <= 0 || capabilities.max_width > 8192 ||
        capabilities.max_height <= 0 || capabilities.max_height > 8192) {
        throw std::invalid_argument(
            "render capabilities max dimensions must be in the range 1..8192 for request contract v1");
    }
    if (capabilities.output_formats.empty()) {
        throw std::invalid_argument("render capabilities require at least one output format");
    }

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("failed to open render capabilities output: " + path);
    }

    output << "AXM_RENDER_CAPABILITIES " << render_capabilities_contract_version << '\n';
    output << "renderer " << capabilities.renderer << '\n';
    output << "renderer_version " << capabilities.renderer_version << '\n';
    output << "backend " << capabilities.backend << '\n';
    output << "scene_contract " << capabilities.scene_contract << '\n';
    output << "render_request_contract " << capabilities.render_request_contract << '\n';
    output << "max_width " << capabilities.max_width << '\n';
    output << "max_height " << capabilities.max_height << '\n';
    for (const auto& format : capabilities.output_formats) {
        if (format.empty()) {
            throw std::invalid_argument("render capabilities format identifiers must not be empty");
        }
        output << "format " << format << '\n';
    }
    if (!output) {
        throw std::runtime_error("failed while writing render capabilities output: " + path);
    }
}

std::string render_request_incompatibility(
    const RenderRequest& request,
    const RenderCapabilities& capabilities) {
    if (capabilities.render_request_contract != render_request_contract_version) {
        return "renderer does not declare AXM_RENDER_REQUEST 1 support";
    }
    if (capabilities.scene_contract != scene_contract_version) {
        return "renderer does not declare AXM_SCENE 1 support";
    }
    if (request.backend != capabilities.backend) {
        return "backend mismatch: request=" + request.backend +
            " capabilities=" + capabilities.backend;
    }
    if (request.width > capabilities.max_width || request.height > capabilities.max_height) {
        return "requested dimensions " + std::to_string(request.width) + "x" +
            std::to_string(request.height) + " exceed declared maximum " +
            std::to_string(capabilities.max_width) + "x" +
            std::to_string(capabilities.max_height);
    }
    if (std::find(
            capabilities.output_formats.begin(),
            capabilities.output_formats.end(),
            request.output_format) == capabilities.output_formats.end()) {
        return "output format not declared by renderer: " + request.output_format;
    }
    return {};
}

} // namespace axm::render
