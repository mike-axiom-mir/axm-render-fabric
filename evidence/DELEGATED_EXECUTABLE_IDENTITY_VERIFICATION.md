# Delegated ImageMagick executable identity verification

Date: 2026-09-14

## What changed

The ImageMagick SVG adapter now binds the exact delegated `convert` executable bytes observed by the adapter into its existing renderer-version evidence without changing any frozen v1 contract fields.

The adapter resolves the configured `AXM_IMAGEMAGICK_CONVERT` command (or `convert` by default) to a canonical regular-file path, computes the existing AXM 64-bit continuity digest over those executable bytes, and composes the adapter renderer version as:

```text
0.2.0+delegated-fnv64-<16-hex-digit executable digest>
```

That same renderer-version token is emitted in `AXM_RENDER_CAPABILITIES 1` and `AXM_RENDER_RECEIPT 1`. The adapter also rechecks the executable byte digest after capability discovery and after external rasterization so a visible byte change during either phase fails instead of silently producing evidence.

No `AXM_RENDER_CAPABILITIES 1` or `AXM_RENDER_RECEIPT 1` field meaning was changed.

## CI evidence actually observed

Successful branch run: GitHub Actions run `34877147364`, head `1a3f207b29f8e14ebf4ab7a4d446b8e07936819b`.

Observed environment:

- Ubuntu 24.04.5 LTS;
- `ubuntu-24.04` runner image `20260907.300.1`;
- GNU C++ 13.3.0;
- ImageMagick `6.9.12-98 Q16 x86_64 18038`.

The adapter resolved the delegated executable to:

```text
/usr/bin/convert-im6.q16
```

Observed continuity digest of those executable bytes:

```text
0x08642e03d077e786
```

Observed bound renderer version in both the capability manifest and receipt:

```text
0.2.0+delegated-fnv64-08642e03d077e786
```

The CI step independently extracted the adapter-reported executable digest and asserted that the exact corresponding renderer-version token appeared in:

- a fresh ImageMagick capability manifest;
- the capability manifest used by `axm-render-external`;
- the receipt produced by the external render.

CTest reported:

```text
100% tests passed, 0 tests failed out of 35
```

## Render continuity observed

The change is evidence binding, not a rendering-algorithm change. In the successful run, the ImageMagick-backed reference output remained:

```text
frame_pixels_digest64 0x8d283e4b5e79fd9f
output_file_digest64 0x7e7f130f59bbe1d1
SHA-256 966bfb0142ed1c297886b2d5b0ce34c73182048d0b5cbc0b95f13edfd6bdfe2c
```

The AXM native reference remained:

```text
frame_pixels_digest64 0x456f404dd94c91da
SHA-256 c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb
```

The native/ImageMagick receipt comparison still reported the same frozen-v1 declared frame intent as comparable while reporting different pixel/output digests. No pixel-equivalence conclusion was added.

## Four-root gate

**Truth:** delegated executable identity is no longer described only by an adapter label. The exact executable bytes observed by the adapter contribute to the renderer-version evidence, and CI checks that capabilities and receipt carry the same identity. The mechanism is explicitly labeled non-cryptographic.

**Agency / non-domination:** this does not make ImageMagick mandatory or replace the AXM-owned native substrate. It strengthens the evidence for an explicitly selected external backend while preserving renderer choice.

**Continuity:** frozen `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` shapes and meanings remain unchanged. Existing capability-bound receipt replay already compares renderer version, so the stronger composite identity passes through the existing interoperability boundary.

**Wisdom before speed:** the change binds one concrete thing that can be measured now — delegated executable bytes — instead of inventing a new provenance protocol or claiming runtime identity beyond the evidence available.

## Explicit truth boundary

This is **non-cryptographic continuity identity**, not authentication, tamper-proof attestation, code signing, or collision-resistant provenance. The 64-bit digest uses the same continuity-digest family already documented for receipt v1.

The bound identity covers the resolved executable file bytes observed at the adapter path. It does **not** bind ImageMagick's semantic version string as a contract field, dynamically loaded shared libraries, delegates, policy/configuration files, environment variables, kernel/runtime state, or other process dependencies. The semantic version above is CI-log evidence correlated with the observed executable, not a cryptographic attestation.

The before/after byte checks do not establish atomic filesystem snapshot semantics or defeat all same-permission path/file races. The adapter is not a sandbox or trusted-execution boundary.

Nothing in this change establishes cross-machine determinism, cross-version pixel stability, visual quality, semantic equivalence with AXM native rendering, production performance, GPU behavior, or production readiness.
