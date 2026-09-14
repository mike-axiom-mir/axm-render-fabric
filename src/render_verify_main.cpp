#include "axm/render/render_receipt.hpp"
#include "axm/render/render_verification.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "usage: axm-render-verify-receipt REQUEST.axmrender RECEIPT.axmreceipt [CAPABILITIES.axmcaps]\n";
        return 2;
    }

    try {
        const bool bind_capabilities = argc == 4;
        const auto verification = bind_capabilities
            ? axm::render::verify_render_receipt_files_with_capabilities(argv[1], argv[2], argv[3])
            : axm::render::verify_render_receipt_files(argv[1], argv[2]);
        std::cout << "verification=PASS\n";
        if (bind_capabilities) {
            std::cout << "capability_binding=PASS\n";
        }
        std::cout << "scene_source_digest64="
                  << axm::render::digest64_hex(verification.scene_source_digest64) << '\n'
                  << "request_source_digest64="
                  << axm::render::digest64_hex(verification.request_source_digest64) << '\n'
                  << "frame_pixels_digest64="
                  << axm::render::digest64_hex(verification.frame_pixels_digest64) << '\n'
                  << "output_file_digest64="
                  << axm::render::digest64_hex(verification.output_file_digest64) << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}
