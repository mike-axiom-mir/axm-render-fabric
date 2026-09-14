# Truth Boundary — v0.1

## Verified by the current implementation

- The repository contains a native C++ executable.
- It can rasterize triangles into an RGB image buffer.
- It performs a depth test.
- It applies simple directional Lambert-style lighting.
- It writes binary PPM output.
- `--self-test` renders the same in-memory scene twice and compares frame hashes.

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

A future implementation must move items across this boundary only with reproducible evidence.
