# AXM Render Fabric

AXM Render Fabric is an open rendering capability layer: one AXM-owned renderer plus stable contracts that can also connect external renderers and future backends.

The architectural direction is deliberately broader than "a renderer inside a game". Games, browser tools, editors, product visualizers, simulations, animation systems, and visual-observer loops should be able to share rendering infrastructure without being trapped inside one product body.

## Current state — v0.1 foundation

The repository now contains a tiny dependency-free C++ reference renderer. It is intentionally small and honest: it rasterizes a demo scene of triangles with a depth buffer and simple directional Lambert lighting, writes a PPM frame, and exposes a same-run determinism self-test.

It is **not** yet a GPU renderer, PBR renderer, scene-file loader, game engine, V-Ray replacement, or browser renderer.

### Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/axm-render --out frame.ppm
```

## Direction

```text
canonical visual / scene state
            |
      render contract
            |
   +--------+---------+----------------+
   |                  |                |
AXM native         WebGPU /         external
renderer           browser          adapters
   |                  |                |
   +------------------+----------------+
                      |
                    pixels
                      |
               visual evidence
                      |
                 observer loop
```

The long-term aim is **own one substrate, connect every useful substrate**. See `FOUNDATION.md`, `ARCHITECTURE.md`, and `docs/TRUTH_BOUNDARY.md`.

## License

Apache-2.0. See `LICENSE`.
