# Local verification — reusable native renderer boundary

This records one container-local verification run for the change that separates the AXM native reference renderer from the CLI. It is evidence for this run only and is not a claim of cross-platform determinism, ABI stability, or visual quality.

Environment observed:

- `g++ (Debian 14.2.0-19) 14.2.0`
- `cmake version 3.31.6`
- C++20 with the repository warning flags

Commands:

```bash
cmake -S . -B build
cmake --build build --config Release -j2
ctest --test-dir build --output-on-failure
./build/axm-render --self-test
./build/native_library_smoke
./build/axm-render --out build/frame.ppm
sha256sum build/frame.ppm
```

Observed test result:

```text
100% tests passed, 0 tests failed out of 3
```

Observed native CLI self-test:

```text
frame_hash_a=0x456f404dd94c91da
frame_hash_b=0x456f404dd94c91da
deterministic_same_run=PASS
```

Observed direct-library smoke client:

```text
library_dimensions=PASS
library_pixel_buffer_size=PASS
library_same_run_hash=PASS
library_rasterized_pixels=PASS
library_changed_pixels=1425
library_frame_hash=0xd110b9e3b792ec2c
```

Observed generated reference frame:

```text
wrote=build/frame.ppm
frame_hash=0x456f404dd94c91da
c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb  build/frame.ppm
```

The generated frame was opened and visually inspected. It shows the same simple two-overlapping-triangle reference composition on the dark background as the prior foundation evidence. This is a smoke check of continuity only, not a visual-quality claim.

Truth boundary: this verification proves that, in this environment, two independent executables can link to and execute the same `axm_render_native` implementation and that the existing CLI reference frame hash/SHA-256 remained unchanged. It does not prove cross-machine bitwise determinism, stable ABI/API compatibility, external-renderer interoperability, GPU behavior, or production performance.
