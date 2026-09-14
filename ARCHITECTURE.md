# Architecture

## Layers

### 1. Canonical visual / scene state
Describes what should exist: geometry, transforms, materials, lights, cameras, environment, animation state, and provenance. The full future contract is intentionally not frozen in the current implementation.

The repository owns one deliberately tiny frozen interchange subset, `AXM_SCENE 1`, containing only ordered triangles with positions and flat RGB-byte albedo. That subset is useful as an executable interoperability foothold without pretending it is the whole canonical model. Incompatible changes to the v1 on-disk meaning require a new version.

### 2. Render contract
Defines the minimum translation boundary between state and a renderer. The repository now owns a deliberately tiny frozen `AXM_RENDER_REQUEST 1` envelope for scene path, backend identifier, output dimensions, `ppm-rgb8`, and output path. Unsupported request versions/directives are rejected, and renderer bodies must reject backend identifiers they do not implement rather than silently falling back.

This is still narrower than the future render contract: feature negotiation, fallback policy, richer settings/formats, and receipts remain separate gates.

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
AXM_RENDER_REQUEST v1 ----+
                          |
AXM_SCENE v1 file --------+--> SceneState + request settings
                                   |
                                   v
                          axm_render_native
                                   |
                triangle rasterizer -> depth test -> lighting
                                   |
                              pixels / PPM
```

`include/axm/render/scene_contract.hpp` owns the renderer-neutral v1 primitive scene types and loader. `include/axm/render/render_contract.hpp` owns the v1 render-request envelope and backend identifier convention. `include/axm/render/reference_renderer.hpp` exposes the native image buffer, renderer, and frame hash. The CLI accepts either direct flags or `--request PATH`; request-selected backend identifiers other than `axm.native.cpu.reference` fail explicitly.

This proves the owned native renderer can be embedded and can consume versioned declared scene/request subsets from disk. It does **not** yet prove a stable C++ ABI/API, an external renderer implementation, cross-backend pixel equivalence, or a complete canonical scene/render model.

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

1. Add frame receipts: scene-state digest + request digest + renderer build/version + backend/settings + output hash.
2. Prove the shared scene/request contracts through a minimal external/mock backend adapter that reports unsupported features explicitly instead of silently degrading.
3. Add a first GPU backend while retaining the CPU reference path.
4. Add browser/WebGPU only after the shared contract is strong enough to avoid two unrelated renderers.
5. Replace synthetic residency estimates with allocator/process measurements, then GPU-VRAM evidence when a GPU backend exists.
