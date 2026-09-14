# Architecture

## Layers

### 1. Canonical visual / scene state
Describes what should exist: geometry, transforms, materials, lights, cameras, environment, animation state, and provenance. This contract is intentionally not frozen in v0.1.

### 2. Render contract
Defines the minimum translation boundary between state and a renderer. Future versions should make inputs, outputs, feature support, fallbacks, and receipts explicit.

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

## v0.1 implementation

The first implementation is a dependency-free CPU reference rasterizer. Its purpose is not visual sophistication. Its purpose is to establish a real executable path:

```text
in-memory scene -> rasterizer -> depth test -> lighting -> pixels -> frame hash / PPM
```

This gives the repository a working substrate that can be replaced, compared, and expanded without pretending the future architecture already exists.

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

1. Separate renderer library from CLI.
2. Define a small versioned scene-state contract.
3. Load a scene from disk instead of only the demo scene.
4. Add frame receipts: state hash + renderer build/version + settings + output hash.
5. Add a first GPU backend while retaining the CPU reference path.
6. Add browser/WebGPU only after the shared contract is strong enough to avoid two unrelated renderers.
7. Replace synthetic residency estimates with allocator/process measurements, then GPU-VRAM evidence when a GPU backend exists.
