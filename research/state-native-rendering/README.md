# State-Native Rendering Research

State-native rendering is an experimental path inside AXM Render Fabric. It asks a measurable question rather than assuming an answer:

> Can a renderer preserve the same canonical visual state and materially equivalent output while keeping much less expanded render data resident at once?

The conventional reference renderer and the state-native path stay in the same repository so they can share contracts, scenes, evidence, and benchmarks.

## Core model

Canonical scene state is the truth. Expanded render objects, GPU buffers, acceleration structures, transformed geometry, caches, and other derived structures are working representations rather than canonical truth unless a specific backend proves otherwise.

A state-native renderer may therefore reconstruct, stream, evict, or regenerate derived data when doing so preserves the requested result and stays within measured performance limits.

```text
canonical scene state
        |
        +--> resident baseline ----------------------+
        |                                            |
        +--> state-native path                       |
             |                                       |
             +-- visible working set                 |
             +-- reconstruction                      |
             +-- disposable caches                   |
             +-- dirty-state updates                 |
             +-- state streaming                     |
             +-- memory-pressure policy              |
                                                     v
                                               render output
                                                     |
                                               evidence/receipt
```

## Research tracks

### 1. Visible-working-set rendering
Materialize only objects, geometry, materials, textures, or acceleration data that can affect the current frame or near-future working set. Measure memory saved against culling/reconstruction overhead.

### 2. Reconstructable geometry and materials
Keep compact canonical state plus shared bases or recipes instead of permanently expanded copies when exact reconstruction is possible.

### 3. Procedural asset state
Test `base + seed + parameters` against storing large numbers of expanded variants. Seeds are useful only when the generation contract is versioned enough to reproduce the intended result.

### 4. Disposable render caches
Treat caches as rebuildable acceleration, not truth. Explore explicit eviction policies and verify that cache loss cannot silently alter canonical scene meaning.

### 5. Dirty-state propagation
Track which state changed and recompute only dependent render data. Compare full rebuild cost against bounded invalidation graphs.

### 6. State streaming
Test scenes whose canonical world is larger than practical resident memory. Stream state and derived render data by deterministic chunks while preserving object identity and continuity.

### 7. Renderer migration
Reconstruct the same canonical scene through CPU, native GPU, WebGPU/browser, game/runtime, and external renderer adapters. Backends may differ in quality or feature support; unsupported features must be reported rather than silently changed.

### 8. Memory-pressure adaptation
Allow a backend to trade cache residency for reconstruction work under explicit memory budgets. A small machine may rebuild more; a large machine may retain more. The canonical scene should remain the same.

### 9. State deltas
Represent changes as small state transitions where practical rather than replacing complete expanded render bodies. Measure update bandwidth, CPU cost, invalidation fan-out, and frame impact.

### 10. Evidence-first render receipts
A state-native experiment should record enough evidence to distinguish canonical state, backend version, settings, working-set policy, output digest, timing, and memory measurements.

## Benchmark questions

Each experiment should try to answer at least these questions:

1. Was the requested scene meaning preserved?
2. Was the rendered/expanded result equivalent within the experiment's declared tolerance?
3. How much canonical memory was resident?
4. How much derived working-set memory was resident?
5. What reconstruction/streaming cost was introduced?
6. What happened to startup time and frame time?
7. What was actually measured: modeled bytes, process RSS, allocator bytes, GPU VRAM, or something else?
8. Can the result be reproduced from the recorded state and versioned implementation?

## First executable experiment

`state_residency_bench` compares two synthetic representations of the same repeated geometry:

- a resident path that stores transformed geometry separately for every object;
- a state path that stores one base mesh plus compact per-instance state and reconstructs visible geometry on demand.

The benchmark compares visible-geometry digests and reports modeled owned bytes. It is intentionally narrow: it does **not** claim process-RSS, GPU-VRAM, real-game, or real-renderer savings.

Run it with:

```bash
./build/state_residency_bench --objects 10000 --visible-percent 10
./build/state_residency_bench --self-test
```

## Truth boundary

This directory is a research lane, not proof that state-native rendering will beat conventional renderers in every workload. State cannot remove unavoidable physical costs such as pixels, geometry processing, texture data, bandwidth, or acceleration structures required by a particular algorithm. The target is unnecessary residency, duplication, history, and derived-state retention.

Any future claim of lower RAM/VRAM must name the exact metric and measurement method. Any claim of equivalent output must name the comparison method and tolerance.
