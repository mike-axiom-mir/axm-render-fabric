# Second renderer body verification

## Purpose

Turn the renderer-neutral contract boundary into one more executable step without weakening the AXM-owned native renderer. The new `contract_flat_renderer` links only `axm_render_contracts`, consumes `AXM_RENDER_REQUEST 1` plus `AXM_SCENE 1`, writes `ppm-rgb8` pixels, and emits an `AXM_RENDER_RECEIPT 1`.

Its backend identifier is `axm.contract.cpu.flat`. It is intentionally small and independent from `axm_render_native`: an XY flat-fill triangle rasterizer with black clear, declared-order overwrite, no depth test, no lighting, no textures, and no claim of matching the native renderer's pixels.

## Executable checks

CTest adds two checks:

- `contract-flat-renderer-writes-pixels-and-receipt` — consumes the shared request/scene contracts, writes a PPM, computes the receipt continuity digests, writes a receipt, reads it back, and requires exact field round-trip.
- `contract-flat-renderer-rejects-native-backend` — passes a request selecting `axm.native.cpu.reference` and requires this body to fail instead of silently substituting itself.

The target links only `axm_render_contracts`; it does not link `axm_render_native`.

## Recorded CI evidence

Pull-request CI run **#78** on commit `5763a3a108eb735153505517ce32e01137f219fc` completed successfully on Ubuntu 24.04 with GCC 13.3.0.

- configure: PASS
- build: PASS
- CTest: **13/13 PASS**
- second renderer pixel write: PASS
- second renderer receipt write/read: PASS
- second renderer rejection of the native backend: PASS
- native reference evidence step remained PASS

The CI evidence step ran:

```text
./build/contract_flat_renderer build/contract-flat.axmrender build/contract-flat.axmreceipt
```

and recorded:

```text
renderer=axm.contract.flat-demo
renderer_version=0.1.0
backend=axm.contract.cpu.flat
scene_triangles=2
projection=xy-no-depth
frame_pixels_digest64=0x00d03224c187ea45
output_file_digest64=0xa4ba7996664347c8
writes_pixels=YES
writes_receipt=YES
contract_flat_renderer=PASS
```

The generated `contract-flat.ppm` SHA-256 in that CI environment was:

```text
346b6caaf38aea04d0b4cdf737f80dab7dee32c7e8940bd71527b58a83f57776
```

The emitted receipt bound the same scene source digest used by the native path (`0x660a6a3e429b6d1c`) while identifying its own request source (`0x4e19dac32ea6d383`), renderer, backend, dimensions, pixel digest, and output-file digest.

The native reference evidence in the same run remained `frame_pixels_digest64=0x456f404dd94c91da` with PPM SHA-256 `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`.

These values are continuity evidence for that CI execution, not a general determinism guarantee.

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
