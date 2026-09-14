# Receipt Replay Verification Evidence

Date: 2026-09-14

PR-head commit verified before this evidence note: `0137a46ec22d0e63ae0759c6a7859d9e86522bda`

GitHub Actions run: `ci` run 137, job `build-and-test`, Ubuntu 24.04, GCC 13.3.0.

## Executable result

The PR-head CI configured and built the full repository successfully and reported:

```text
100% tests passed, 0 tests failed out of 25
```

The new receipt replay tests passed for both current pixel-producing bodies:

- `receipt-verify-native-reference`
- `receipt-verify-flat-reference`

The negative test `receipt-verify-rejects-mismatched-request` also passed under CTest `WILL_FAIL`, demonstrating that a native receipt is not accepted when replayed against the flat renderer request.

CI then invoked `axm-render-verify-receipt` directly against freshly rendered evidence.

Native reference replay:

```text
verification=PASS
scene_source_digest64=0x660a6a3e429b6d1c
request_source_digest64=0x20c8519ed81dacef
frame_pixels_digest64=0x456f404dd94c91da
output_file_digest64=0x7e33cf9168a97ab0
```

Independent flat-body replay at the same declared 320x180 scene intent:

```text
verification=PASS
scene_source_digest64=0x660a6a3e429b6d1c
request_source_digest64=0x141c5e5603de3d34
frame_pixels_digest64=0x5338e2b2729f1381
output_file_digest64=0xe47b2ea5622500df
```

The native continuity value remained `0x456f404dd94c91da`; the verifier did not change renderer output.

## What was actually verified

`axm-render-verify-receipt` links to the renderer-neutral `axm_render_contracts` library rather than either renderer body. For frozen v1 `ppm-rgb8`, it independently re-opens the request, referenced scene, and output and checks:

- request and scene parse through their frozen v1 loaders;
- receipt scene/request contract versions match v1;
- receipt backend, dimensions, and format match the request;
- request and scene source-byte continuity digests match the current files;
- P6 RGB8 dimensions match the request;
- decoded row-major RGB pixel digest matches `frame_pixels_digest64`;
- complete output-file digest matches `output_file_digest64`;
- request, scene, and output digests remain unchanged through the ordinary verification interval.

## Truth boundary

This evidence does **not** establish cryptographic integrity, authentication, provenance, atomic filesystem snapshotting, protection against change-and-revert races, cross-machine determinism, visual quality, semantic equivalence between renderer bodies, timing/performance, or a real third-party renderer integration.

The pixel replay path is explicitly limited to the only frozen v1 output format, `ppm-rgb8`. New output formats require explicit decoding semantics before equivalent pixel-digest replay can be claimed.

The 64-bit continuity digests are non-cryptographic and remain continuity evidence only.

## Root gate

- **Truth:** emitted receipt claims are independently checked against current source/output bytes and decoded pixels.
- **Agency / non-domination:** verification sits in the shared contract layer and does not depend on the AXM native renderer body.
- **Continuity:** the same verification path accepted receipts from both current pixel-producing bodies without requiring pixel equivalence between them.
- **Wisdom before speed:** no frozen v1 meaning changed; the improvement strengthens evidence around the existing contract instead of widening the contract prematurely.
