# Local verification — AXM_SCENE v1 minimal scene interchange

This records a focused local verification of the new scene-contract/loader path. It is evidence for this observed environment only; repository CI is the separate full-tree check.

Observed environment:

- `g++ (Debian 14.2.0-19) 14.2.0`
- `cmake version 3.31.6`
- C++20 with `-Wall -Wextra -Wpedantic`

Focused build commands compiled the exact native renderer, scene-contract implementation, CLI, and scene-file smoke client directly from source:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude \
  src/reference_renderer.cpp src/scene_contract.cpp tests/scene_file_smoke.cpp \
  -o scene_file_smoke_direct

g++ -std=c++20 -Wall -Wextra -Wpedantic -Iinclude \
  src/reference_renderer.cpp src/scene_contract.cpp src/main.cpp \
  -o axm-render-direct
```

Observed reference scene-file smoke output:

```text
scene_contract_version=1
scene_triangle_count=2
scene_triangle_count_expected=PASS
scene_frame_hash=0x456f404dd94c91da
scene_reference_hash_expected=0x456f404dd94c91da
scene_reference_hash_match=PASS
```

Observed file-loaded render:

```text
scene_source=examples/reference.axmscene
scene_triangles=2
wrote=direct-frame.ppm
frame_hash=0x456f404dd94c91da
c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb  direct-frame.ppm
```

That FNV frame hash and PPM SHA-256 match the previously recorded built-in reference frame, so this run supports continuity of the existing native reference result through the new file-loaded path.

Observed unsupported-version behavior:

```text
error: scene parse error in examples/unsupported-v2.axmscene:1: unsupported scene contract version 2
unsupported_version_exit=1
```

This demonstrates explicit rejection rather than silent interpretation of an unknown scene version in this implementation.

## Truth boundary

This verification supports only the following claims in the observed local build:

- the v1 file loader compiles with the native renderer;
- the two-triangle reference fixture is parsed as two triangles;
- the loaded reference fixture produces the same known native frame hashes as the prior built-in fixture;
- an unsupported v2 header is rejected with a non-zero exit.

It does **not** prove cross-machine/cross-compiler bitwise determinism, external-renderer equivalence, GPU behavior, visual quality, production performance, a complete canonical scene model, or long-term C++ ABI/API stability. No fresh visual-quality judgment was made for this change; byte-level continuity is the relevant evidence here.
