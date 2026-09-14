#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr const char* probe_backend = "axm.mock.contract-probe";

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            throw std::invalid_argument("usage: contract_adapter_probe REQUEST.axmrender");
        }

        const std::string request_path = argv[1];
        const std::uint64_t request_digest_before = axm::render::continuity_digest64_file(request_path);
        const auto request = axm::render::load_render_request_file(request_path);
        const std::uint64_t request_digest_after = axm::render::continuity_digest64_file(request_path);
        if (request_digest_before != request_digest_after) {
            throw std::runtime_error("render request source changed while contract probe loaded it");
        }

        if (request.backend != probe_backend) {
            throw std::runtime_error(
                "unsupported backend for contract probe: " + request.backend +
                " (supported: " + probe_backend + ")");
        }

        const std::uint64_t scene_digest_before = axm::render::continuity_digest64_file(request.scene_path);
        const auto scene = axm::render::load_scene_file(request.scene_path);
        const std::uint64_t scene_digest_after = axm::render::continuity_digest64_file(request.scene_path);
        if (scene_digest_before != scene_digest_after) {
            throw std::runtime_error("scene source changed while contract probe loaded it");
        }

        std::cout << "probe_backend=" << probe_backend << "\n";
        std::cout << "scene_contract=" << axm::render::scene_contract_version << "\n";
        std::cout << "render_request_contract=" << axm::render::render_request_contract_version << "\n";
        std::cout << "render_receipt_contract=" << axm::render::render_receipt_contract_version << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "width=" << request.width << "\n";
        std::cout << "height=" << request.height << "\n";
        std::cout << "output_format=" << request.output_format << "\n";
        std::cout << "scene_source_digest64=" << axm::render::digest64_hex(scene_digest_after) << "\n";
        std::cout << "request_source_digest64=" << axm::render::digest64_hex(request_digest_after) << "\n";
        std::cout << "writes_pixels=NO\n";
        std::cout << "contract_adapter_probe=PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
