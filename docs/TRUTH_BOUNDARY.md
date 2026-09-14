# Truth Boundary — current foundation

## Verified by the current implementation

- The repository contains a native C++ reference renderer.
- The native renderer is built as the `axm_render_native` static library with public C++ headers.
- Renderer-neutral scene primitives live in `include/axm/render/scene_contract.hpp`; the native renderer consumes those primitives rather than defining duplicate scene types.
- A renderer-neutral minimal request envelope lives in `include/axm/render/render_contract.hpp` and the frozen `AXM_RENDER_REQUEST 1` on-disk subset.
- A renderer-neutral minimal evidence envelope lives in `include/axm/render/render_receipt.hpp` and the frozen `AXM_RENDER_RECEIPT 1` on-disk subset.
- The shared library can write and strictly parse v1 receipts; unsupported receipt versions, unknown/duplicate directives, malformed digests, invalid dimensions, missing fields, and trailing tokens are rejected.
- Receipt v1 records renderer/version, requested backend, scene/request contract versions, exact source-byte continuity digests, output dimensions/format, a pixel-buffer digest, and an output-file digest.
- The receipt continuity digest algorithm is explicitly specified and deliberately labeled non-cryptographic.
- The native CLI can emit `--receipt PATH` for an explicit request-backed render and refuses receipt emission for self-test/direct-CLI paths that lack the required request-source evidence.
- For receipt-backed renders, the native CLI checks request and scene source-byte digests around loading and again before receipt emission; visible source-digest changes cause failure instead of silent evidence emission.
- Receipt paths are kept separate from rendered output paths.
- The `axm-render` CLI links to the native library rather than owning a duplicate renderer implementation.
- A separate `native_library_smoke` test client links directly to the library, renders pixels, validates the image dimensions/buffer size, and compares two same-run frame hashes.
- `AXM_SCENE 1` is an explicit, minimal, versioned on-disk scene-state subset for ordered triangles with finite positions and RGB-byte albedo.
- The scene loader rejects unsupported versions, unknown directives, invalid/non-finite triangle data, out-of-range RGB values, and trailing tokens instead of silently guessing.
- `AXM_RENDER_REQUEST 1` is an explicit, minimal, versioned request subset carrying scene path, backend identifier, width, height, `ppm-rgb8`, and output path.
- The request loader rejects unsupported versions, unknown/duplicate directives, invalid dimensions, unsupported v1 output formats, and trailing tokens.
- Relative scene/output paths in a request are resolved against the request file's directory.
- `axm-render --request PATH` executes requests selecting `axm.native.cpu.reference`; a different backend identifier is rejected explicitly rather than silently falling back.
- The reference request renders to the same known native frame continuity hash as the existing reference scene in the tested build.
- The renderer can rasterize triangles into an RGB image buffer.
- It performs a depth test.
- It applies simple directional Lambert-style lighting.
- It writes binary PPM output.
- `--self-test` renders the same selected scene twice and compares frame hashes.
- The state-native research lane contains an executable synthetic residency comparison.
- That comparison can represent the same repeated geometry as either fully expanded per-object copies or one shared base mesh plus compact instance state.
- Its self-test compares visible expanded-geometry digests between those two synthetic representations.
- It reports modeled owned bytes for those experiment-owned containers and labels that metric explicitly.

## Not yet claimed

- Cryptographic integrity, authenticity, tamper evidence, or collision resistance for render receipts.
- Exact renderer source-commit identity or proof that a renderer working tree was clean when built.
- Atomic filesystem snapshot semantics for receipt source files; the current before/after digest checks do not defeat a change-and-revert race.
- Cross-machine or cross-compiler bit-for-bit determinism.
- A stable public ABI or long-term C++ API compatibility guarantee for `axm_render_native`.
- Install/export/package support for consuming the library outside a source/CMake integration.
- A complete canonical visual/scene format beyond the frozen minimal `AXM_SCENE 1` triangle subset.
- A complete renderer-neutral render contract beyond frozen `AXM_RENDER_REQUEST 1`; feature negotiation, fallback policy, and richer settings/formats are not defined yet.
- Receipt timing, allocator/process memory, GPU VRAM, energy, or performance evidence.
- Pixel equivalence between the AXM native renderer and any external renderer/backend.
- GPU rendering.
- WebGPU/browser rendering.
- PBR materials, textures, shadows, reflections, path tracing, denoising, or global illumination.
- Production performance.
- External renderer adapters.
- Visual quality judgment.
- Screenshot perception or an automated visual observer.
- Compatibility with AXM Universal Creation, Ghost Studio, WALMI, or other AXM projects yet.
- Lower real process RSS from state-native rendering.
- Lower GPU VRAM use from state-native rendering.
- Better frame time, startup time, bandwidth, energy use, or total memory in production workloads.
- Equivalent final rendered pixels between resident and state-native production backends.

The first state-native benchmark is deliberately synthetic. Its byte counts are a model of memory owned by its chosen C++ containers, not operating-system RSS, allocator telemetry, driver memory, or GPU VRAM. Its digest proves equivalence only for the quantized visible expanded geometry/material IDs used by that experiment.

The reusable library boundary proves source-level reuse of the existing native reference implementation in the tested build. It does not by itself prove binary compatibility or renderer interchangeability.

The `AXM_SCENE 1` boundary proves a small on-disk scene-state interoperability foothold. The `AXM_RENDER_REQUEST 1` boundary adds a small on-disk request foothold that names a backend instead of assuming one. `AXM_RENDER_RECEIPT 1` adds a small evidence foothold that can be shared by future renderer bodies, but it does not prove a second renderer currently implements the contracts or that separate backends produce equivalent pixels.

Receipt v1 source digests cover exact bytes, not semantic normalization. Comments, whitespace, or line-ending changes alter source digests even when parsed meaning may remain equivalent. The receipt's 64-bit continuity digests are intentionally non-cryptographic and must not be treated as security primitives.

A future implementation must move items across this boundary only with reproducible evidence and must name the exact memory/output metric being compared.
