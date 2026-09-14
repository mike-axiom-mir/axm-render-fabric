# Truth Boundary — current foundation

## Verified by the current implementation

- The repository contains a native C++ reference renderer.
- The native renderer is built as the `axm_render_native` static library with public C++ headers.
- Renderer-neutral scene primitives live in `include/axm/render/scene_contract.hpp`; the native renderer consumes those primitives rather than defining duplicate scene types.
- The `axm-render` CLI links to that library rather than owning a duplicate renderer implementation.
- A separate `native_library_smoke` test client links directly to the library, renders pixels, validates the image dimensions/buffer size, and compares two same-run frame hashes.
- `AXM_SCENE 1` is an explicit, minimal, versioned on-disk scene-state subset for ordered triangles with finite positions and RGB-byte albedo.
- The scene loader rejects unsupported versions, unknown directives, invalid/non-finite triangle data, out-of-range RGB values, and trailing tokens instead of silently guessing.
- `axm-render --scene PATH` can load declared v1 scene state from disk and render it through the AXM-owned native substrate.
- The reference `AXM_SCENE 1` fixture renders to the same known native reference-frame FNV hash as the prior built-in demo scene in the tested build.
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

- Cross-machine or cross-compiler bit-for-bit determinism.
- A stable public ABI or long-term C++ API compatibility guarantee for `axm_render_native`.
- Install/export/package support for consuming the library outside a source/CMake integration.
- A complete canonical visual/scene format beyond the frozen minimal `AXM_SCENE 1` triangle subset.
- A renderer-neutral render-request contract covering backend selection, dimensions/settings, outputs, feature support, or fallbacks.
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

The `AXM_SCENE 1` boundary now proves a small on-disk interoperability foothold: the syntax and semantics in `docs/SCENE_CONTRACT_V1.md` are versioned and incompatible changes require a new version. This does not elevate that tiny subset into the full future canonical scene model, nor does it prove a second renderer implements it yet.

A future implementation must move items across this boundary only with reproducible evidence and must name the exact memory/output metric being compared.
