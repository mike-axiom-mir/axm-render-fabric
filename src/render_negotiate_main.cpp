#include "axm/render/render_capabilities.hpp"
#include "axm/render/render_contract.hpp"
#include "axm/render/scene_contract.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        if (argc != 3) {
            throw std::invalid_argument(
                "usage: axm-render-negotiate REQUEST.axmrender CAPABILITIES.axmcaps");
        }

        const std::string request_path = argv[1];
        const std::string capabilities_path = argv[2];
        const auto request = axm::render::load_render_request_file(request_path);
        const auto capabilities = axm::render::load_render_capabilities_file(capabilities_path);

        // Loading the scene is intentional: request-v1 negotiation should not report
        // compatibility for a scene source that the frozen AXM_SCENE 1 parser rejects.
        const auto scene = axm::render::load_scene_file(request.scene_path);
        const std::string incompatibility =
            axm::render::render_request_incompatibility(request, capabilities);

        std::cout << "renderer=" << capabilities.renderer << "\n";
        std::cout << "renderer_version=" << capabilities.renderer_version << "\n";
        std::cout << "request_backend=" << request.backend << "\n";
        std::cout << "capability_backend=" << capabilities.backend << "\n";
        std::cout << "scene_contract=" << capabilities.scene_contract << "\n";
        std::cout << "render_request_contract=" << capabilities.render_request_contract << "\n";
        std::cout << "scene_triangles=" << scene.triangles.size() << "\n";
        std::cout << "requested_dimensions=" << request.width << "x" << request.height << "\n";
        std::cout << "declared_max_dimensions=" << capabilities.max_width << "x"
                  << capabilities.max_height << "\n";
        std::cout << "requested_format=" << request.output_format << "\n";

        if (!incompatibility.empty()) {
            std::cout << "compatible=NO\n";
            std::cout << "reason=" << incompatibility << "\n";
            return 2;
        }

        std::cout << "compatible=YES\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
