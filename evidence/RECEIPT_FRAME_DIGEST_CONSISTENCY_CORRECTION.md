# Receipt frame-digest consistency correction

## Why this correction exists

During a fresh repository inspection, the independent `contract_flat_renderer` was found to compute `AXM_RENDER_RECEIPT 1` `frame_pixels_digest64` with a different FNV offset basis from the frozen receipt-v1 continuity-digest semantics.

Receipt v1 and the native reference renderer use:

```text
offset = 1469598103934665603
prime  = 1099511628211
```

The flat renderer had a local duplicate helper using:

```text
offset = 14695981039346656037
prime  = 1099511628211
```

That extra `7` in the offset meant its pixel bytes were real, its output-file digest was still computed through the shared file helper, and its receipt parsed correctly, but the receipt's **frame pixel digest did not follow the frozen AXM_RENDER_RECEIPT 1 algorithm**. This is an evidence-semantics bug, not a visual renderer bug.

## Repair

The renderer-neutral receipt header now exposes one shared `continuity_digest64_rgb8(...)` helper implementing the frozen receipt-v1 pixel-byte digest semantics.

- the AXM native reference `fnv1a(Image)` delegates to the shared helper;
- `contract_flat_renderer` delegates to the same helper;
- the flat body's duplicate local pixel-digest implementation is removed.

The existing native reference hash is expected to remain unchanged because its previous implementation already used the frozen receipt-v1 parameters.

## Recomputed expected flat-body values

Recomputing the existing flat rasterizer's RGB bytes with the frozen receipt-v1 parameters gives:

```text
64x36 contract-flat request:
old non-v1 frame digest: 0x00d03224c187ea45
correct receipt-v1 frame digest: 0x5c48decbca7cb29f

320x180 same-intent comparison request:
old non-v1 frame digest: 0xf31c764f5e70511f
correct receipt-v1 frame digest: 0x5338e2b2729f1381
```

These corrected values concern only `frame_pixels_digest64`. The flat renderer's pixel output is intentionally not changed by this repair, so its complete PPM bytes and `output_file_digest64` should remain unchanged if the build reproduces the same raster output.

## Historical evidence status

The flat-body `frame_pixels_digest64` values recorded before this correction in `SECOND_RENDERER_BODY_VERIFICATION.md` and `CROSS_BACKEND_RECEIPT_COMPARISON_VERIFICATION.md` are historical observations produced by the buggy local helper. They must not be treated as valid AXM_RENDER_RECEIPT 1 pixel-digest values.

The corresponding pixel files, SHA-256 values, renderer identities, source digests, dimensions, output-file digests, comparison results, and explicit non-equivalence boundaries are not invalidated merely by this digest-parameter mismatch. Fresh CI after this repair is required before the corrected receipt values are claimed as repository evidence.

## Truth boundary

This correction does not claim visual quality, changed pixels, cross-machine determinism, cryptographic integrity, external-renderer equivalence, or production performance. It only repairs one frozen evidence invariant: every current renderer body that emits `AXM_RENDER_RECEIPT 1` should compute `frame_pixels_digest64` with the same declared continuity-digest algorithm.

## Four-root merge gate

- **Truth:** name the prior mismatch instead of preserving a convenient but invalid receipt claim.
- **Agency / non-domination:** the digest semantics live in the renderer-neutral contract layer rather than one renderer body's private copy.
- **Continuity:** native and alternate bodies now bind pixel evidence with one frozen v1 rule.
- **Wisdom before speed:** repair evidence semantics before adding more renderer bodies that could copy the same divergence.
