#include "axm/render/render_receipt.hpp"
#include "axm/render/render_verification.hpp"

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: axm-render-verify-receipt REQUEST.axmrender RECEIPT.axmreceipt\n";
        return 2;
    }

    try {
        const auto verification = axm::render::verify_render_receipt_files(argv[1], argv[2]);
        std::cout << "verification=PASS\n"
                  << "scene_source_digest64="
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
