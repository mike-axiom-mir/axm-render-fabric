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

### 4. Pixel / frame evidence
A render result should be accompanied by enough metadata to say which state, renderer version, backend, settings, and frame produced it.

### 5. Observer loop
A visual observer may inspect actual rendered output and propose state revisions. The observer is downstream of evidence; it must not claim to have seen a frame that was never rendered and inspected.

## v0.1 implementation

The first implementation is a dependency-free CPU reference rasterizer. Its purpose is not visual sophistication. Its purpose is to establish a real executable path:

```text
in-memory scene -> rasterizer -> depth test -> lighting -> pixels -> frame hash / PPM
```

This gives the repository a working substrate that can be replaced, compared, and expanded without pretending the future architecture already exists.

## Next likely gates

1. Separate renderer library from CLI.
2. Define a small versioned scene-state contract.
3. Load a scene from disk instead of only the demo scene.
4. Add frame receipts: state hash + renderer build/version + settings + output hash.
5. Add a first GPU backend while retaining the CPU reference path.
6. Add browser/WebGPU only after the shared contract is strong enough to avoid two unrelated renderers.
