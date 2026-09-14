# Architecture

## Layers

### 1. Canonical visual / scene state
Describes what should exist: geometry, transforms, materials, lights, cameras, environment, animation state, and provenance. The full future contract is intentionally not frozen in the current implementation.

The repository now owns one deliberately tiny frozen interchange subset, `AXM_SCENE 1`, containing only ordered triangles with positions and flat RGB-byte albedo. That subset is useful as an executable interoperability foothold without pretending it is the whole canonical model. Incompatible changes to the v1 on-disk meaning require a new version.

### 2. Render contract
Defines the minimum translation boundary between state and a renderer. Future versions should make inputs, outputs, feature support, fallbacks, and receipts explicit. Resolution remains a render-request concern rather than being encoded into `AXM_SCENE 1`.

### 3. Renderer bodies
Multiple bodies may coexist:

- AXM native CPU/GPU renderer
- native GPU backend(s)
- WebGPU/browser backend
- game/runtime backend
- external renderer adapters
- future unknown backends

### 4. Residency / derivation policy
A renderer body may choose how much derived render data to keep resident. Expanded geometry, transformed vertices, GPU buffers, acceleration structures, texture residency, caches, and history are not automatically canonical truth.

Two strategies are deliberately kept side by side:

- **resident baseline** — retain expanded render representations where useful;
- **state-native research path** — retain compact canonical state and reconstruct, stream, cache, or evict derived data under explicit policies.

The goal is not to assume that state-native rendering is superior. It is to measure when the same declared scene/output can be preserved with lower residency and what reconstruction, bandwidth, startup, or frame-time costs are introduced.

### 5. Pixel / frame evidence
A render result should be accompanied by enough metadata to say which state, renderer version, backend, settings, residency policy, and frame produced it.

### 6. Observer loop
A visual observer may inspect actual rendered output and propose state revisions. The observer is downstream of evidence; it must not claim to have seen a frame that was never rendered and inspected.

## Current native implementation

The native reference path is separated into a reusable C++ library plus thin clients:

```text
AXM_SCENE v1 file / C++ caller
             |
             v
      SceneState primitives
             |
             v
     axm_render_native
             |
 triangle rasterizer -> depth test -> lighting -> pixels -> frame hash / PPM
```

`include/axm/render/scene_contract.hpp` owns the renderer-neutral v1 primitive scene types and loader. `include/axm/render/reference_renderer.hpp` exposes the native image buffer, renderer, and frame hash. `src/scene_contract.cpp` strictly parses the v1 file subset. `src/reference_renderer.cpp` owns the native implementation. `src/main.cpp` can use either its built-in continuity fixture or `--scene PATH`. `tests/native_library_smoke.cpp` and `tests/scene_file_smoke.cpp` are independent clients of the library.

This is intentionally narrower than a complete canonical scene/render contract. It proves the owned native renderer can be embedded and can consume one versioned declared scene-state subset from disk. It is **not** yet a stable C++ ABI/API promise and does not yet define the full render request or prove an external renderer can reproduce the same pixels.

The repository also contains a first synthetic state-residency benchmark:

```text
resident expanded copies ------------------+
                                            +--> visible-geometry digest
shared base mesh + compact instance state --+
                 |
                 +--> on-demand expansion
```

The benchmark reports modeled owned bytes and digest equivalence only. It is not a process-RSS or GPU-VRAM measurement.

## State-native research gates

Before treating state-native rendering as a production direction, progressively test:

1. visible-working-set materialization;
2. reconstructable geometry and materials;
3. procedural asset state with versioned seeds/recipes;
4. disposable caches with explicit invalidation;
5. dirty-state propagation;
6. state streaming for worlds larger than resident memory;
7. equivalent scene reconstruction across renderer bodies;
8. memory-pressure adaptation;
9. state-delta update paths;
10. render receipts that name state, backend, settings, output digest, timing, and exact memory metric.

See `research/state-native-rendering/README.md`.

## Next likely gates

1. Define a small versioned renderer-neutral render-request contract around declared scene state, output dimensions/settings, backend selection, and explicit unsupported features.
2. Add frame receipts: scene-state digest + renderer build/version + backend/settings + output hash.
3. Prove `AXM_SCENE 1` through a minimal external/mock backend adapter that reports unsupported features explicitly instead of silently degrading.
4. Add a first GPU backend while retaining the CPU reference path.
5. Add browser/WebGPU only after the shared contract is strong enough to avoid two unrelated renderers.
6. Replace synthetic residency estimates with allocator/process measurements, then GPU-VRAM evidence when a GPU backend exists.
