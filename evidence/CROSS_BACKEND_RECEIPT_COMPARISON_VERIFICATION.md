# Cross-backend receipt comparison verification

> **Correction (2026-09-14):** the historical flat `frame_pixels_digest64=0xf31c764f5e70511f` recorded below was produced by a body-local offset basis inconsistent with frozen `AXM_RENDER_RECEIPT 1` semantics. Fresh verified v1 value for the same 320x180 flat request is `0x5338e2b2729f1381`. See `RECEIPT_FRAME_DIGEST_CONSISTENCY_CORRECTION.md`. The comparator conclusion remains `same_frame_pixels_digest64=NO`; the old flat digest itself must not be treated as valid receipt-v1 evidence.

## Verified change

The repository now has an executable renderer-neutral comparison gate, `axm-render-compare`, that consumes two strict `AXM_RENDER_RECEIPT 1` files without linking either renderer body.

For frozen `AXM_RENDER_REQUEST 1`, it treats two receipts as comparable declared frame intent only when all body-independent fields preserved by receipt v1 match:

- scene contract version;
- render-request contract version;
- exact scene-source continuity digest;
- width and height;
- output format.

Backend identity and request/output-path source bytes are renderer-specific and are not required to match. Frame-pixel and output-file digest equality are reported separately and are not required for comparability.

## CI evidence

GitHub Actions run `34829958284`, job `103930708619`, on Ubuntu 24.04 / GCC 13.3.0 built the branch implementation and completed successfully.

CTest result:

```text
100% tests passed, 0 tests failed out of 16
```

The added executable path was exercised by these passing tests:

```text
interop-native-reference-receipt
interop-flat-reference-receipt
interop-compare-reference-receipts
```

The CI evidence step then rendered the same `reference.axmscene` source through both current pixel-producing bodies at 320x180 `ppm-rgb8` and compared their receipts.

Native receipt evidence:

```text
renderer axm.native.cpu.reference
backend axm.native.cpu.reference
scene_contract 1
render_request_contract 1
scene_source_digest64 0x660a6a3e429b6d1c
width 320
height 180
format ppm-rgb8
frame_pixels_digest64 0x456f404dd94c91da
output_file_digest64 0x7e33cf9168a97ab0
```

Native output SHA-256:

```text
c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb
```

Independent flat-body receipt evidence for the same declared frame dimensions/source:

```text
renderer axm.contract.flat-demo
backend axm.contract.cpu.flat
scene_contract 1
render_request_contract 1
scene_source_digest64 0x660a6a3e429b6d1c
width 320
height 180
format ppm-rgb8
frame_pixels_digest64 0xf31c764f5e70511f
output_file_digest64 0xe47b2ea5622500df
```

Flat-body output SHA-256:

```text
542fe181b96ce28c65dfed6a4e51fa09e1d8fdd477e9278364bc5dd1da85813c
```

Comparator output:

```text
left_renderer=axm.native.cpu.reference
left_backend=axm.native.cpu.reference
right_renderer=axm.contract.flat-demo
right_backend=axm.contract.cpu.flat
same_scene_contract=YES
same_render_request_contract=YES
same_scene_source=YES
same_dimensions=YES
same_output_format=YES
comparable_v1=YES
same_frame_pixels_digest64=NO
same_output_file_digest64=NO
comparison_boundary=PIXEL_DIGEST_RELATION_REPORTED_NOT_REQUIRED
render_receipt_compare=PASS
```

## What this establishes

- Two distinct pixel-producing bodies consumed the same exact scene-source bytes under the same scene/request contract versions, dimensions, and output format.
- Both emitted the shared receipt-v1 evidence shape; the correction notice above supersedes only the old flat frame-digest interpretation.
- A renderer-neutral executable recognized the receipts as comparable v1 declared frame intent.
- The executable also surfaced that the produced pixel and output-file digests differ rather than treating interoperability as pixel equivalence.

## Truth boundaries

This evidence does **not** establish:

- visual equivalence or visual quality;
- pixel equivalence;
- feature equivalence between the native depth/lit rasterizer and the flat XY/no-depth body;
- cross-machine or cross-compiler determinism;
- cryptographic integrity or provenance;
- a production third-party renderer adapter;
- GPU, WebGPU, timing, RSS, VRAM, performance, or energy claims.

The 64-bit continuity digests are non-cryptographic continuity evidence. The output SHA-256 values above identify the files produced by this CI run but are not renderer authenticity attestations.
