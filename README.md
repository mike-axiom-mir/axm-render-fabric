# AXM Render Fabric

AXM Render Fabric is an open rendering capability layer: one AXM-owned renderer plus stable contracts that can also connect external renderers and future backends.

The architectural direction is deliberately broader than "a renderer inside a game". Games, browser tools, editors, product visualizers, simulations, animation systems, and visual-observer loops should be able to share rendering infrastructure without being trapped inside one product body.

## Current state — native reference substrate + renderer-neutral contracts

The repository contains a tiny dependency-free C++ reference renderer. It is intentionally small and honest: it rasterizes triangles with a depth buffer and simple directional Lambert lighting, writes a PPM frame, and exposes same-run frame hashing.

Renderer-neutral scene/request/receipt implementations are separated into the `axm_render_contracts` static library (`axm::render_contracts`). The AXM-owned renderer body builds separately as `axm_render_native` (`axm::render_native`) and depends on that contract library. The `axm-render` command is a thin client of the native renderer.

A contract-only mock adapter probe links only to `axm_render_contracts`. It accepts backend `axm.mock.contract-probe`, loads the shared request and scene formats, computes the shared source-byte continuity digests, and rejects a request for the native backend instead of silently falling back. The probe deliberately writes no pixels; it proves a source-level interoperability boundary, not a second renderer.

Three small on-disk interoperability boundaries are executable:

- `AXM_SCENE 1` — a deliberately tiny triangle-only scene-state format;
- `AXM_RENDER_REQUEST 1` — a request envelope for scene path, backend identifier, dimensions, output format, and output path;
- `AXM_RENDER_RECEIPT 1` — a renderer-neutral evidence record binding renderer/backend/settings to scene/request source-byte continuity digests plus pixel/output digests.

Incompatible changes require new versions, and unsupported versions/directives are rejected instead of being guessed. The native executable implements backend identifier `axm.native.cpu.reference` and rejects other requested backends rather than silently substituting itself. Receipt read/write support lives in the contract library so future adapters can use the same evidence shape.

The receipt digests are deliberately non-cryptographic continuity evidence. They do not prove authenticity, collision resistance, visual quality, exact source-commit provenance, or cross-machine determinism.

It is **not** yet a stable C++ ABI/API promise, installed package, GPU renderer, PBR renderer, complete canonical scene format, production external-renderer adapter, second pixel-producing renderer body, game engine, V-Ray replacement, browser renderer, or proof of cross-backend pixel equivalence.

### Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/axm-render --out frame.ppm
./build/axm-render --scene examples/reference.axmscene --out scene-frame.ppm
./build/axm-render --request examples/reference.axmrender
./build/axm-render --request examples/reference.axmrender --receipt frame.axmreceipt
./build/contract_adapter_probe examples/contract-probe.axmrender
```

A parent CMake build that includes this repository with `add_subdirectory(...)` can link renderer-neutral integration code against `axm::render_contracts` or native rendering code against `axm::render_native`. Packaging/install/export rules are not claimed yet.

See `docs/SCENE_CONTRACT_V1.md`, `docs/RENDER_REQUEST_V1.md`, and `docs/RENDER_RECEIPT_V1.md` for the exact frozen minimal interchange/evidence subsets and truth boundaries.

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
              render receipt
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
