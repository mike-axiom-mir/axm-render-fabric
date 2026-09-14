# Truth Boundary — v0.1

## Verified by the current implementation

- The repository contains a native C++ executable.
- It can rasterize triangles into an RGB image buffer.
- It performs a depth test.
- It applies simple directional Lambert-style lighting.
- It writes binary PPM output.
- `--self-test` renders the same in-memory scene twice and compares frame hashes.
- The state-native research lane contains an executable synthetic residency comparison.
- That comparison can represent the same repeated geometry as either fully expanded per-object copies or one shared base mesh plus compact instance state.
- Its self-test compares visible expanded-geometry digests between those two synthetic representations.
- It reports modeled owned bytes for those experiment-owned containers and labels that metric explicitly.

## Not yet claimed

- Cross-machine or cross-compiler bit-for-bit determinism.
- A stable canonical scene file format.
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

A future implementation must move items across this boundary only with reproducible evidence and must name the exact memory/output metric being compared.
