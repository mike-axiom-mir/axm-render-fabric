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

The long-term aim is **own one substrate, connect every useful substrate**.

## State-native rendering research

A second path now lives beside the resident reference approach. The question is whether canonical scene state can remain compact while expanded render data is reconstructed, streamed, cached, and evicted on demand without changing the declared result.

Current research tracks include visible-working-set rendering, reconstructable geometry/materials, procedural asset state, disposable caches, dirty-state propagation, state streaming, renderer migration, memory-pressure adaptation, state deltas, and evidence-first render receipts.

The first executable experiment compares a fully expanded repeated-geometry representation against one base mesh plus compact instance state. It verifies equivalent synthetic visible-geometry digests and reports **modeled owned bytes only**; it does not claim real process RSS, GPU VRAM, game performance, or production-renderer savings.

```bash
./build/state_residency_bench --objects 10000 --visible-percent 10
./build/state_residency_bench --self-test
```

See `research/state-native-rendering/README.md` for the research map and truth boundary.

See also `FOUNDATION.md`, `ARCHITECTURE.md`, and `docs/TRUTH_BOUNDARY.md`.

## License

Apache-2.0. See `LICENSE`.
