#include "axm/render/render_receipt.hpp"

#include <charconv>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace axm::render {
namespace {

constexpr std::uint64_t digest_offset = 1469598103934665603ULL;
constexpr std::uint64_t digest_prime = 1099511628211ULL;

std::runtime_error parse_error(
    const std::string& path,
    std::size_t line_number,
    const std::string& message) {
    return std::runtime_error(
        "render receipt parse error in " + path + ":" + std::to_string(line_number) + ": " + message);
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

std::string require_token(
    std::istringstream& input,
    const std::string& path,
    std::size_t line_number,
    const std::string& field) {
    std::string value;
    if (!(input >> value)) {
        throw parse_error(path, line_number, field + " requires one token");
    }
    require_no_extra_tokens(input, path, line_number);
    return value;
}

std::uint32_t parse_u32(
    const std::string& token,
    const std::string& path,
    std::size_t line_number,
    const std::string& field) {
    std::uint64_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value, 10);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
        value == 0 || value > std::numeric_limits<std::uint32_t>::max()) {
        throw parse_error(path, line_number, field + " requires a positive uint32 value");
    }
    return static_cast<std::uint32_t>(value);
}

int parse_positive_int(
    const std::string& token,
    const std::string& path,
    std::size_t line_number,
    const std::string& field) {
    std::uint64_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value, 10);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size() ||
        value == 0 || value > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        throw parse_error(path, line_number, field + " requires a positive integer");
    }
    return static_cast<int>(value);
}

std::uint64_t parse_digest64(
    const std::string& token,
    const std::string& path,
    std::size_t line_number,
    const std::string& field) {
    if (token.size() != 18 || token[0] != '0' || token[1] != 'x') {
        throw parse_error(path, line_number, field + " requires 0x plus exactly 16 hex digits");
    }
    std::uint64_t value = 0;
    const char* begin = token.data() + 2;
    const char* end = token.data() + token.size();
    const auto result = std::from_chars(begin, end, value, 16);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw parse_error(path, line_number, field + " contains invalid hex digits");
    }
    return value;
}

void require_output_token(const std::string& field, const std::string& value) {
    if (value.empty()) {
        throw std::invalid_argument("render receipt " + field + " cannot be empty");
    }
    for (unsigned char c : value) {
        if (std::isspace(c) || c == '#') {
            throw std::invalid_argument(
                "render receipt " + field + " must be one token without whitespace or #");
        }
    }
}

void validate_receipt(const RenderReceipt& receipt) {
    require_output_token("renderer", receipt.renderer);
    require_output_token("renderer_version", receipt.renderer_version);
    require_output_token("backend", receipt.backend);
    require_output_token("format", receipt.output_format);
    if (receipt.scene_contract == 0 || receipt.render_request_contract == 0) {
        throw std::invalid_argument("render receipt contract-version metadata must be positive");
    }
    if (receipt.width <= 0 || receipt.height <= 0) {
        throw std::invalid_argument("render receipt dimensions must be positive");
    }
}

} // namespace

std::uint64_t continuity_digest64_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open file for continuity digest: " + path);
    }

    std::uint64_t hash = digest_offset;
    char buffer[4096];
    while (input) {
        input.read(buffer, sizeof(buffer));
        const std::streamsize count = input.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= digest_prime;
        }
    }
    if (!input.eof()) {
        throw std::runtime_error("failed while reading file for continuity digest: " + path);
    }
    return hash;
}

std::string digest64_hex(std::uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

RenderReceipt load_render_receipt_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open render receipt file: " + path);
    }

    RenderReceipt receipt;
    bool saw_header = false;
    bool saw_renderer = false;
    bool saw_renderer_version = false;
    bool saw_backend = false;
    bool saw_scene_contract = false;
    bool saw_render_request_contract = false;
    bool saw_scene_digest = false;
    bool saw_request_digest = false;
    bool saw_width = false;
    bool saw_height = false;
    bool saw_format = false;
    bool saw_frame_digest = false;
    bool saw_output_digest = false;

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
            if (!(tokens >> magic >> version) || magic != "AXM_RENDER_RECEIPT") {
                throw parse_error(path, line_number, "expected header: AXM_RENDER_RECEIPT 1");
            }
            require_no_extra_tokens(tokens, path, line_number);
            if (version != render_receipt_contract_version) {
                throw parse_error(
                    path,
                    line_number,
                    "unsupported render receipt contract version " + std::to_string(version));
            }
            saw_header = true;
            continue;
        }

        std::string directive;
        tokens >> directive;
        if (directive == "renderer") {
            if (saw_renderer) throw parse_error(path, line_number, "duplicate renderer directive");
            receipt.renderer = require_token(tokens, path, line_number, directive);
            saw_renderer = true;
        } else if (directive == "renderer_version") {
            if (saw_renderer_version) throw parse_error(path, line_number, "duplicate renderer_version directive");
            receipt.renderer_version = require_token(tokens, path, line_number, directive);
            saw_renderer_version = true;
        } else if (directive == "backend") {
            if (saw_backend) throw parse_error(path, line_number, "duplicate backend directive");
            receipt.backend = require_token(tokens, path, line_number, directive);
            saw_backend = true;
        } else if (directive == "scene_contract") {
            if (saw_scene_contract) throw parse_error(path, line_number, "duplicate scene_contract directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.scene_contract = parse_u32(value, path, line_number, directive);
            saw_scene_contract = true;
        } else if (directive == "render_request_contract") {
            if (saw_render_request_contract) throw parse_error(path, line_number, "duplicate render_request_contract directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.render_request_contract = parse_u32(value, path, line_number, directive);
            saw_render_request_contract = true;
        } else if (directive == "scene_source_digest64") {
            if (saw_scene_digest) throw parse_error(path, line_number, "duplicate scene_source_digest64 directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.scene_source_digest64 = parse_digest64(value, path, line_number, directive);
            saw_scene_digest = true;
        } else if (directive == "request_source_digest64") {
            if (saw_request_digest) throw parse_error(path, line_number, "duplicate request_source_digest64 directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.request_source_digest64 = parse_digest64(value, path, line_number, directive);
            saw_request_digest = true;
        } else if (directive == "width") {
            if (saw_width) throw parse_error(path, line_number, "duplicate width directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.width = parse_positive_int(value, path, line_number, directive);
            saw_width = true;
        } else if (directive == "height") {
            if (saw_height) throw parse_error(path, line_number, "duplicate height directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.height = parse_positive_int(value, path, line_number, directive);
            saw_height = true;
        } else if (directive == "format") {
            if (saw_format) throw parse_error(path, line_number, "duplicate format directive");
            receipt.output_format = require_token(tokens, path, line_number, directive);
            saw_format = true;
        } else if (directive == "frame_pixels_digest64") {
            if (saw_frame_digest) throw parse_error(path, line_number, "duplicate frame_pixels_digest64 directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.frame_pixels_digest64 = parse_digest64(value, path, line_number, directive);
            saw_frame_digest = true;
        } else if (directive == "output_file_digest64") {
            if (saw_output_digest) throw parse_error(path, line_number, "duplicate output_file_digest64 directive");
            const auto value = require_token(tokens, path, line_number, directive);
            receipt.output_file_digest64 = parse_digest64(value, path, line_number, directive);
            saw_output_digest = true;
        } else {
            throw parse_error(path, line_number, "unsupported directive: " + directive);
        }
    }

    if (!saw_header) {
        throw std::runtime_error("render receipt parse error in " + path + ": missing AXM_RENDER_RECEIPT header");
    }
    if (!input.eof()) {
        throw std::runtime_error("failed while reading render receipt file: " + path);
    }
    if (!saw_renderer || !saw_renderer_version || !saw_backend || !saw_scene_contract ||
        !saw_render_request_contract || !saw_scene_digest || !saw_request_digest ||
        !saw_width || !saw_height || !saw_format || !saw_frame_digest || !saw_output_digest) {
        throw std::runtime_error(
            "render receipt parse error in " + path + ": v1 requires every receipt field exactly once");
    }

    validate_receipt(receipt);
    return receipt;
}

void write_render_receipt_file(const std::string& path, const RenderReceipt& receipt) {
    validate_receipt(receipt);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("failed to open render receipt file: " + path);
    }

    out << "AXM_RENDER_RECEIPT " << render_receipt_contract_version << '\n'
        << "renderer " << receipt.renderer << '\n'
        << "renderer_version " << receipt.renderer_version << '\n'
        << "backend " << receipt.backend << '\n'
        << "scene_contract " << receipt.scene_contract << '\n'
        << "render_request_contract " << receipt.render_request_contract << '\n'
        << "scene_source_digest64 " << digest64_hex(receipt.scene_source_digest64) << '\n'
        << "request_source_digest64 " << digest64_hex(receipt.request_source_digest64) << '\n'
        << "width " << receipt.width << '\n'
        << "height " << receipt.height << '\n'
        << "format " << receipt.output_format << '\n'
        << "frame_pixels_digest64 " << digest64_hex(receipt.frame_pixels_digest64) << '\n'
        << "output_file_digest64 " << digest64_hex(receipt.output_file_digest64) << '\n';

    if (!out) {
        throw std::runtime_error("failed while writing render receipt file: " + path);
    }
}

} // namespace axm::render
