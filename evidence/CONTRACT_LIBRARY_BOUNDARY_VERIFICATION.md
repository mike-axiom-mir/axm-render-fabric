# Contract library boundary verification

Date: 2026-09-14

Scope: verify that renderer-neutral scene/request/receipt implementations can build and execute outside the AXM native renderer target, without claiming a second renderer exists.

## Evidence source

GitHub Actions PR run 34819665297 on PR #8:

- configure: PASS
- build: PASS
- CTest: PASS, 11/11 tests
- `contract-only-adapter-probe`: PASS
- `contract-only-adapter-rejects-native-backend`: PASS
- native request-backed render + receipt: PASS after the library split

Build output shows `libaxm_render_contracts.a` and `libaxm_render_native.a` built as separate static libraries, then `contract_adapter_probe` linked as its own executable.

The native continuity checks remained unchanged in that run:

- native frame continuity digest: `0x456f404dd94c91da`
- native complete reference PPM SHA-256: `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`
- native output-file continuity digest64: `0x7e33cf9168a97ab0`

## What the probe establishes

`contract_adapter_probe` links only to `axm_render_contracts`. It consumes `AXM_RENDER_REQUEST 1`, the referenced `AXM_SCENE 1`, and the receipt contract's continuity-digest helper. The accepted fixture selects backend `axm.mock.contract-probe`. A request selecting `axm.native.cpu.reference` is rejected explicitly; the probe does not silently substitute itself.

## Truth boundary

This verifies source-level separation of renderer-neutral contract code from the AXM native renderer body in the tested GitHub Actions environment. It does not establish a stable ABI/API, installed package support, an external pixel-producing renderer, cross-backend pixel equivalence, visual quality, GPU behavior, timing, process RSS, VRAM, performance, cryptographic provenance, or cross-machine determinism.

The probe explicitly reports `writes_pixels=NO`; moving from contract consumption to a second renderer body remains a separate gate.
