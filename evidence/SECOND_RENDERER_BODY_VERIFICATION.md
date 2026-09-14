# Second renderer body verification

## Purpose

Turn the renderer-neutral contract boundary into one more executable step without weakening the AXM-owned native renderer. The new `contract_flat_renderer` links only `axm_render_contracts`, consumes `AXM_RENDER_REQUEST 1` plus `AXM_SCENE 1`, writes `ppm-rgb8` pixels, and emits an `AXM_RENDER_RECEIPT 1`.

Its backend identifier is `axm.contract.cpu.flat`. It is intentionally small and independent from `axm_render_native`: an XY flat-fill triangle rasterizer with black clear, declared-order overwrite, no depth test, no lighting, no textures, and no claim of matching the native renderer's pixels.

## Executable checks

CTest adds two checks:

- `contract-flat-renderer-writes-pixels-and-receipt` — consumes the shared request/scene contracts, writes a PPM, computes the receipt continuity digests, writes a receipt, reads it back, and requires exact field round-trip.
- `contract-flat-renderer-rejects-native-backend` — passes a request selecting `axm.native.cpu.reference` and requires this body to fail instead of silently substituting itself.

The target links only `axm_render_contracts`; it does not link `axm_render_native`.

## Evidence status

This document is committed with the implementation before merge. CI results must be checked on the pull request before this change is treated as verified or mergeable. A green test suite is evidence only for the compiled CI environment and the executable assertions above.

## Truth boundary

This change does **not** prove:

- visual quality;
- pixel equivalence with the AXM native renderer;
- depth-correct or physically meaningful rendering;
- cross-machine or cross-compiler bitwise determinism;
- a stable C++ ABI/API;
- compatibility with a real third-party renderer;
- GPU/WebGPU behavior, timing, RSS, VRAM, or performance;
- cryptographic integrity of the 64-bit continuity digests.

The flat body deliberately ignores scene `z` during its XY projection. That behavior is named here rather than treated as an implicit capability. The useful claim is narrower: a second pixel-producing body can consume the shared contracts and emit the shared receipt without linking the native renderer body.
