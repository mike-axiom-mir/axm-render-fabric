#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/render_receipt.hpp"
#include "axm/render/scene_contract.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require_digest_unchanged(
    const std::string& path,
    std::uint64_t expected,
    const char* label) {
    if (axm::render::continuity_digest64_file(path) != expected) {
        throw std::runtime_error(std::string(label) + " changed during capability negotiation");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc < 3) {
            throw std::invalid_argument(
                "usage: axm-render-negotiate REQUEST.axmrender CAPABILITIES.axmcaps "
                "[CAPABILITIES.axmcaps ...]");
        }

        const std::string request_path = argv[1];
        const std::uint64_t request_digest =
            axm::render::continuity_digest64_file(request_path);
        const auto request = axm::render::load_render_request_file(request_path);
        require_digest_unchanged(request_path, request_digest, "render request source");

        // Loading the scene is intentional: request-v1 negotiation should not report
        // compatibility for a scene source that the frozen AXM_SCENE 1 parser rejects.
        const std::uint64_t scene_digest =
            axm::render::continuity_digest64_file(request.scene_path);
        const auto scene = axm::render::load_scene_file(request.scene_path);
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        const std::size_t manifest_count = static_cast<std::size_t>(argc - 2);
        std::size_t compatible_count = 0;

        std::cout << "request_backend=" << request.backend << "\n";
        std::cout << "request_source_digest64="
                  << axm::render::digest64_hex(request_digest) << "\n";
        std::cout << "scene_source_digest64="
                  << axm::render::digest64_hex(scene_digest) << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "requested_dimensions=" << request.width << "x" << request.height << "\n";
        std::cout << "requested_format=" << request.output_format << "\n";
        std::cout << "manifest_count=" << manifest_count << "\n";

        for (int argument_index = 2; argument_index < argc; ++argument_index) {
            const std::size_t manifest_index = static_cast<std::size_t>(argument_index - 1);
            const std::string capabilities_path = argv[argument_index];
            const std::uint64_t capabilities_digest =
                axm::render::continuity_digest64_file(capabilities_path);
            const auto capabilities =
                axm::render::load_render_capabilities_file(capabilities_path);
            require_digest_unchanged(
                capabilities_path, capabilities_digest, "capability manifest");

            // Keep the state that determined compatibility stable while this scan runs.
            require_digest_unchanged(request_path, request_digest, "render request source");
            require_digest_unchanged(request.scene_path, scene_digest, "scene source");

            const std::string incompatibility =
                axm::render::render_request_incompatibility(request, capabilities);

            std::cout << "manifest_begin=" << manifest_index << "\n";
            std::cout << "capabilities_path=" << capabilities_path << "\n";
            std::cout << "capabilities_digest64="
                      << axm::render::digest64_hex(capabilities_digest) << "\n";
            std::cout << "renderer=" << capabilities.renderer << "\n";
            std::cout << "renderer_version=" << capabilities.renderer_version << "\n";
            std::cout << "capability_backend=" << capabilities.backend << "\n";
            std::cout << "scene_contract=" << capabilities.scene_contract << "\n";
            std::cout << "render_request_contract=" << capabilities.render_request_contract << "\n";
            std::cout << "declared_max_dimensions=" << capabilities.max_width << "x"
                      << capabilities.max_height << "\n";

            if (!incompatibility.empty()) {
                std::cout << "compatible=NO\n";
                std::cout << "reason=" << incompatibility << "\n";
            } else {
                ++compatible_count;
                std::cout << "compatible=YES\n";
            }
            std::cout << "manifest_end=" << manifest_index << "\n";
        }

        require_digest_unchanged(request_path, request_digest, "render request source");
        require_digest_unchanged(request.scene_path, scene_digest, "scene source");

        std::cout << "compatible_count=" << compatible_count << "\n";
        if (compatible_count == 0) {
            return 2;
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
