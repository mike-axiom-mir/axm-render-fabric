# AXM_RENDER_RECEIPT 1

`AXM_RENDER_RECEIPT 1` is the first frozen renderer-neutral evidence envelope in AXM Render Fabric. It binds one declared scene/request source pair and one rendered output to explicit renderer/backend/settings metadata plus reproducible 64-bit continuity digests.

It is intentionally small. The purpose is to let the AXM native substrate and future external/back-end adapters emit the same evidence shape without pretending that this is a cryptographic attestation or a proof of cross-machine determinism.

## Syntax

The first non-comment line is:

```text
AXM_RENDER_RECEIPT 1
```

A v1 receipt then contains every field below exactly once:

```text
renderer axm.native.cpu.reference
renderer_version 0.1.0
backend axm.native.cpu.reference
scene_contract 1
render_request_contract 1
scene_source_digest64 0x0000000000000000
request_source_digest64 0x0000000000000000
width 320
height 180
format ppm-rgb8
frame_pixels_digest64 0x0000000000000000
output_file_digest64 0x0000000000000000
```

Blank lines and `#` comments are ignored by the parser. Values are single tokens containing neither whitespace nor `#`; the writer rejects values that would be truncated by comment parsing. Digest values are `0x` plus exactly 16 hexadecimal digits. Unknown directives, duplicate directives, unsupported receipt versions, malformed digests, non-positive dimensions, missing fields, and trailing tokens are rejected.

Field order is not semantically significant to the parser. The AXM writer emits the canonical order shown above.

## Meaning

- `renderer` identifies the renderer body that emitted the receipt.
- `renderer_version` identifies that body's declared implementation version. The current native reference body reports `0.1.0`; this is not an exact source-commit attestation or an ABI promise.
- `backend` records the backend selected by the render request.
- `scene_contract` records the scene contract version consumed.
- `render_request_contract` records the request contract version consumed.
- `scene_source_digest64` covers the exact bytes of the scene source file used as evidence.
- `request_source_digest64` covers the exact bytes of the render-request source file used as evidence.
- `width`, `height`, and `format` record the output settings.
- `frame_pixels_digest64` covers RGB pixel bytes in row-major order, without the PPM header.
- `output_file_digest64` covers every byte of the written output file.

Paths are deliberately not stored in v1. A receipt should remain useful after a bundle is moved, and local paths can disclose machine-specific information. Source identity is represented by byte digests instead.

## Continuity digest64 v1

The digest fields deliberately use the repository's existing 64-bit continuity-hash parameters so the native reference frame retains its already-recorded continuity value.

For each byte, in order:

```text
hash starts at 1469598103934665603
hash = hash XOR byte
hash = hash * 1099511628211 modulo 2^64
```

This is an AXM continuity digest, not a cryptographic primitive. It must **not** be described as collision-resistant, secure, authenticated, or tamper-proof. A future receipt version may add a cryptographic digest without silently changing v1 meaning.

The source digests are byte-level, not semantic-normalization digests. Editing comments, whitespace, or line endings changes them even if parsed scene/request meaning stays the same.

## Native CLI path

The native executable can emit a receipt only from an explicit request:

```bash
./build/axm-render \
  --request examples/reference.axmrender \
  --receipt frame.axmreceipt
```

`--receipt` currently requires `--request` and is rejected with `--self-test`. This keeps v1 source evidence explicit instead of inventing a pseudo-request for direct CLI flags or the built-in demo scene.

When receipt emission is requested, the native CLI digests the request and scene sources around loading and checks them again before receipt emission. If their byte digests change during that interval, receipt emission fails rather than silently binding evidence to a visibly different source. This is a practical change-detection check, not an atomic filesystem snapshot and not protection against a change-and-revert race.

## Independent replay verification

`axm-render-verify-receipt` is renderer-neutral and links only to `axm_render_contracts`. Given a request and receipt, it independently re-opens the request, referenced scene, and declared output and checks that the current files still match the receipt's frozen v1 claims:

```bash
./build/axm-render-verify-receipt build/reference.axmrender build/frame.axmreceipt
```

For `ppm-rgb8`, the verifier checks the request/scene byte digests, contract versions, backend, dimensions, format, complete output-file digest, and RGB pixel digest. It also parses the request and scene through the frozen v1 loaders and checks the source/output digests again after verification to catch ordinary concurrent changes.

This is deliberately a replay check against files available **now**. It does not establish who produced them, when they were produced, or that they were unchanged before or after the verification interval. The continuity digests remain non-cryptographic. The current pixel replay path is limited to the only v1 output format, `ppm-rgb8`; a future output format needs its own explicit pixel-decoding semantics before the verifier can claim to re-check `frame_pixels_digest64` for it.

## Truth boundary

A syntactically valid v1 receipt by itself proves only that a renderer emitted a parseable evidence record with declared metadata and digests. The independent replay verifier raises the evidence level when the referenced request, scene, and output are present by checking those declarations against the current bytes and decoded RGB pixels. For the current native and independent flat paths, repository tests exercise both positive verification and rejection of a mismatched request.

A v1 receipt, even after replay verification, does **not** prove:

- cryptographic integrity, authenticity, or provenance;
- exact source commit or clean working-tree identity of the renderer;
- cross-machine or cross-compiler bitwise determinism;
- visual quality;
- semantic equivalence between two differently formatted scene/request files;
- pixel equivalence between different renderer bodies;
- that a real third-party renderer currently implements AXM contracts;
- atomic filesystem snapshotting or protection against change-and-revert races;
- timing, process RSS, allocator usage, GPU VRAM, energy use, or production performance.

Those remain separate evidence gates.
