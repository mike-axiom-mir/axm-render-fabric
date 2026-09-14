# AXM Render Fabric

AXM Render Fabric is an open rendering capability layer: one AXM-owned renderer plus stable contracts that can also connect external renderers and future backends.

The architectural direction is deliberately broader than "a renderer inside a game". Games, browser tools, editors, product visualizers, simulations, animation systems, and visual-observer loops should be able to share rendering infrastructure without being trapped inside one product body.

## Current state — native reference substrate

The repository contains a tiny dependency-free C++ reference renderer. It is intentionally small and honest: it rasterizes triangles with a depth buffer and simple directional Lambert lighting, writes a PPM frame, and exposes same-run frame hashing.

The renderer is separated from the CLI as the `axm_render_native` static library. Scene primitives now live in the renderer-neutral `include/axm/render/scene_contract.hpp`, while `include/axm/render/reference_renderer.hpp` exposes the native image/renderer surface. The `axm-render` command is a thin client of that library, and independent CTest clients exercise both direct-library rendering and file-loaded scene state.

The first on-disk interoperability boundary is also executable: `AXM_SCENE 1` is a deliberately tiny, versioned triangle-only scene-state format. Incompatible changes require a new version, and unsupported versions/directives are rejected instead of being guessed. It is a stable minimal subset, not a claim that the full future canonical scene model is already known.

It is **not** yet a stable C++ ABI/API promise, installed package, GPU renderer, PBR renderer, complete canonical scene format, renderer-neutral render-request contract, game engine, V-Ray replacement, browser renderer, or external-renderer adapter.

### Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/axm-render --out frame.ppm
./build/axm-render --scene examples/reference.axmscene --out scene-frame.ppm
```

A parent CMake build that includes this repository with `add_subdirectory(...)` can link native C++ code against the in-tree alias target `axm::render_native`. Packaging/install/export rules are not claimed yet.

See `docs/SCENE_CONTRACT_V1.md` for the exact v1 exchange syntax and truth boundary.

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

A second path lives beside the resident reference approach. The question is whether canonical scene state can remain compact while expanded render data is reconstructed, streamed, cached, and evicted on demand without changing the declared result.

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
