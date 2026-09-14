#include "axm/render/render_capabilities.hpp"
#include "axm/render/scene_contract.hpp"

#include <iostream>
#include <string>

namespace {

axm::render::RenderCapabilities flat_compatible_capabilities() {
    return {
        "axm.contract.flat-demo",
        "0.1.0",
        "axm.contract.cpu.flat",
        axm::render::scene_contract_version,
        axm::render::render_request_contract_version,
        8192,
        8192,
        {"ppm-rgb8"}
    };
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--capabilities") {
        axm::render::write_render_capabilities_file(
            argv[2], flat_compatible_capabilities());
        std::cout << "capabilities_only=PASS\n";
        return 0;
    }

    if (argc == 3) {
        std::cout << "render_intentionally_writes_nothing=PASS\n";
        return 0;
    }

    return 2;
}
