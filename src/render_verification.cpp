#include "axm/render/render_verification.hpp"

#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <charconv>
#include <cctype>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace axm::render {
namespace {

[[noreturn]] void verification_error(const std::string& message) {
    throw std::runtime_error("render receipt verification failed: " + message);
}

void require_match(bool condition, const std::string& field) {
    if (!condition) verification_error(field + " mismatch");
}

std::string read_ppm_token(std::ifstream& input, const std::string& path) {
    while (true) {
        const int next = input.peek();
        if (next == EOF) {
            verification_error("unexpected end of PPM header in " + path);
        }
        if (std::isspace(static_cast<unsigned char>(next))) {
            input.get();
            continue;
        }
        if (next == '#') {
            input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        break;
    }

    std::string token;
    while (true) {
        const int next = input.peek();
        if (next == EOF || std::isspace(static_cast<unsigned char>(next)) || next == '#') break;
        token.push_back(static_cast<char>(input.get()));
    }
    if (token.empty()) {
        verification_error("empty PPM header token in " + path);
    }
    return token;
}

int parse_positive_int(const std::string& token, const std::string& field, const std::string& path) {
    int value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value, 10);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() || value <= 0) {
        verification_error("invalid " + field + " in PPM output " + path);
    }
    return value;
}

std::vector<Color> load_ppm_rgb8_pixels(
    const std::string& path,
    int expected_width,
    int expected_height) {
    std::ifstream input(path, std::ios::binary);
    if (!input) verification_error("failed to open output file: " + path);

    const std::string magic = read_ppm_token(input, path);
    if (magic != "P6") verification_error("output is not P6 ppm-rgb8: " + path);

    const int width = parse_positive_int(read_ppm_token(input, path), "width", path);
    const int height = parse_positive_int(read_ppm_token(input, path), "height", path);
    const int max_value = parse_positive_int(read_ppm_token(input, path), "max value", path);

    require_match(width == expected_width, "PPM width");
    require_match(height == expected_height, "PPM height");
    if (max_value != 255) verification_error("ppm-rgb8 requires max value 255");

    const int separator = input.get();
    if (separator == EOF || !std::isspace(static_cast<unsigned char>(separator))) {
        verification_error("PPM header is not separated from pixel bytes by whitespace");
    }
    if (separator == '\r' && input.peek() == '\n') input.get();

    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<Color> pixels(pixel_count);
    for (auto& pixel : pixels) {
        char rgb[3]{};
        input.read(rgb, 3);
        if (input.gcount() != 3) verification_error("PPM pixel payload is truncated");
        pixel = {
            static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[0])),
            static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[1])),
            static_cast<std::uint8_t>(static_cast<unsigned char>(rgb[2]))
        };
    }

    if (input.get() != EOF) verification_error("PPM output contains trailing bytes");
    return pixels;
}

} // namespace

RenderReceiptVerification verify_render_receipt_files(
    const std::string& request_path,
    const std::string& receipt_path) {
    const std::uint64_t request_digest = continuity_digest64_file(request_path);
    const RenderRequest request = load_render_request_file(request_path);
    require_match(
        continuity_digest64_file(request_path) == request_digest,
        "request source changed while loading");

    const std::uint64_t scene_digest = continuity_digest64_file(request.scene_path);
    load_scene_file(request.scene_path);
    require_match(
        continuity_digest64_file(request.scene_path) == scene_digest,
        "scene source changed while loading");

    const RenderReceipt receipt = load_render_receipt_file(receipt_path);

    require_match(receipt.scene_contract == scene_contract_version, "scene contract version");
    require_match(
        receipt.render_request_contract == render_request_contract_version,
        "render request contract version");
    require_match(receipt.backend == request.backend, "backend");
    require_match(receipt.width == request.width, "width");
    require_match(receipt.height == request.height, "height");
    require_match(receipt.output_format == request.output_format, "output format");
    require_match(receipt.scene_source_digest64 == scene_digest, "scene source digest");
    require_match(receipt.request_source_digest64 == request_digest, "request source digest");

    if (request.output_format != "ppm-rgb8") {
        verification_error("unsupported output format for v1 replay verification: " + request.output_format);
    }

    const std::uint64_t output_digest = continuity_digest64_file(request.output_path);
    const auto pixels = load_ppm_rgb8_pixels(request.output_path, request.width, request.height);
    const std::uint64_t pixel_digest = continuity_digest64_rgb8(pixels);

    require_match(receipt.frame_pixels_digest64 == pixel_digest, "frame pixel digest");
    require_match(receipt.output_file_digest64 == output_digest, "output file digest");

    require_match(continuity_digest64_file(request_path) == request_digest, "request source changed during verification");
    require_match(continuity_digest64_file(request.scene_path) == scene_digest, "scene source changed during verification");
    require_match(continuity_digest64_file(request.output_path) == output_digest, "output file changed during verification");

    return {scene_digest, request_digest, pixel_digest, output_digest};
}

RenderReceiptVerification verify_render_receipt_files_with_capabilities(
    const std::string& request_path,
    const std::string& receipt_path,
    const std::string& capabilities_path) {
    const RenderReceiptVerification verification =
        verify_render_receipt_files(request_path, receipt_path);

    const std::uint64_t capabilities_digest = continuity_digest64_file(capabilities_path);
    const RenderCapabilities capabilities = load_render_capabilities_file(capabilities_path);
    require_match(
        continuity_digest64_file(capabilities_path) == capabilities_digest,
        "capabilities source changed while loading");

    const RenderRequest request = load_render_request_file(request_path);
    const RenderReceipt receipt = load_render_receipt_file(receipt_path);

    const std::string incompatibility = render_request_incompatibility(request, capabilities);
    if (!incompatibility.empty()) {
        verification_error("capabilities are incompatible with request: " + incompatibility);
    }

    require_match(receipt.renderer == capabilities.renderer, "renderer identity vs capabilities");
    require_match(
        receipt.renderer_version == capabilities.renderer_version,
        "renderer version vs capabilities");
    require_match(receipt.backend == capabilities.backend, "backend vs capabilities");
    require_match(receipt.scene_contract == capabilities.scene_contract, "scene contract vs capabilities");
    require_match(
        receipt.render_request_contract == capabilities.render_request_contract,
        "render request contract vs capabilities");

    require_match(
        continuity_digest64_file(capabilities_path) == capabilities_digest,
        "capabilities source changed during verification");

    return verification;
}

} // namespace axm::render
