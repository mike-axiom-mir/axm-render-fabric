# AXM_RENDER_RECEIPT v1 verification

## Scope

This evidence covers the first executable renderer-neutral receipt boundary added on top of the existing `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, and native CPU reference renderer.

Base `main` inspected before work:

- `730a120c79d94d52c496b18f62fca469e6856d86` — `Render contract: minimal AXM_RENDER_REQUEST v1 (#6)`

Receipt implementation/test head verified by GitHub Actions:

- `43a1012b7362aff2931e9b072083afddea05c59b`
- workflow run: https://github.com/mike-axiom-mir/axm-render-fabric/actions/runs/34816040357
- job `build-and-test`: **PASS**
- configure: **PASS**
- build: **PASS**
- CTest step: **PASS**
- request-backed render + receipt emission: **PASS**
- output/receipt evidence recording step: **PASS**

The CMake test registry at that head contains 9 tests. Because the single `ctest --test-dir build --output-on-failure` workflow step completed successfully, all 9 registered tests passed in that run.

## Executable receipt checks

`render_receipt_smoke` exercises:

- v1 write -> strict parse -> field-for-field round trip;
- rejection of unsupported `AXM_RENDER_RECEIPT 2`;
- rejection of a writer token containing `#`, which would otherwise conflict with receipt comment syntax;
- reference pixel-buffer continuity digest;
- reference written-PPM continuity digest.

The native CLI test separately executes an actual request-backed render with `--receipt` enabled.

Reference continuity values asserted by the executable smoke test:

```text
frame_pixels_digest64 = 0x456f404dd94c91da
output_file_digest64  = 0x7e33cf9168a97ab0
```

The frame value is the existing native reference-frame continuity value already used by the repository. The output-file value covers the complete binary PPM bytes for that same reference render.

Before opening this lane, the current native renderer and exact repository reference scene were independently reconstructed and rendered in the task environment. That produced:

```text
frame_pixels_digest64 = 0x456f404dd94c91da
PPM SHA-256 = c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb
```

That independent check is supporting continuity evidence only. The repository CI remains the authoritative build/test result for the PR head.

For the current LF-encoded repository fixtures, the same documented continuity-digest algorithm produced:

```text
reference.axmscene  = 0x660a6a3e429b6d1c
reference.axmrender = 0x20c8519ed81dacef
```

These are exact-byte values, not semantic scene/request identities. Line-ending, whitespace, or comment edits can change them without changing parsed meaning.

## Truth boundary

This verification demonstrates that the tested repository build can compile the receipt code, run its registered tests, render the reference request, and emit a receipt under GitHub Actions on `ubuntu-latest`.

It does **not** establish:

- cryptographic integrity, authentication, provenance, or collision resistance;
- exact renderer source-commit attestation inside the receipt itself;
- atomic source snapshots or protection against a change-and-revert race;
- cross-machine or cross-compiler bitwise determinism;
- external-renderer implementation or pixel equivalence;
- visual quality;
- process RSS, allocator usage, GPU VRAM, energy, timing, or production performance;
- stable C++ ABI/API compatibility.

The receipt digest is deliberately named an AXM continuity digest and is not represented as a security primitive.

## Root check

- **Truth:** digest scope, parser rules, tested results, and unverified claims are explicit.
- **Agency / non-domination:** receipt semantics are renderer-neutral and can be implemented by future native or external bodies without requiring the current CPU body.
- **Continuity:** exact scene/request source bytes, selected settings/backend, pixel bytes, and output bytes can now be tied together in one versioned evidence record.
- **Wisdom before speed:** v1 freezes only the evidence that is already executable; cryptographic provenance, timing/memory evidence, and cross-backend equivalence remain later gates.
