# External dispatch fresh-artifact verification

## Grounded gap

`axm-render-external` already checked request/scene continuity, performed capability preflight, launched a separate renderer process, and replay-verified the resulting receipt. One evidence gap remained: a successful child exit did not prove that capability/output/receipt files were produced by that invocation. If valid files from an earlier run already existed and a child returned success without writing replacements, those stale files could be accepted as current dispatch evidence.

## Improvement under test

The external-process harness now treats the declared capability manifest, render output, and receipt as outputs owned by the current dispatch phase:

- any pre-existing non-directory entry at the capability path is removed before capability discovery;
- capability discovery must recreate that path as a regular file;
- any pre-existing non-directory entries at render-output and receipt paths are removed before rendering;
- rendering must recreate both paths as regular files;
- existing directories are rejected rather than recursively removed;
- produced symlinks and other non-regular artifacts are rejected;
- request/scene continuity checks, capability compatibility, and capability-bound receipt replay remain in force.

Two deliberately non-producing child programs exercise the regression boundary:

- `contract_silent_renderer` exits successfully while emitting nothing. With valid stale capability/render artifacts already present, dispatch must still fail because the capability path was cleared and not recreated.
- `contract_caps_only_renderer` emits a fresh compatible capability manifest but exits successfully from its render invocation without writing output or receipt. With valid stale render artifacts already present, dispatch must still fail because those paths were cleared and not recreated.

These tests distinguish successful process exit from successful production of current evidence.

## Verified code-bearing PR-head evidence

GitHub Actions run `34864603873` exercised PR #16 head `a04addcf1917befdb8102fcb40f2e0c2f44ca38c` on Ubuntu 24.04.5 with GCC 13.3.0 and completed successfully.

Observed results:

- CMake configure: PASS;
- build: PASS, including both negative-test helper executables;
- CTest: **32/32 PASS**;
- `external-process-flat-dispatch`: PASS, so the normal separate-process path still works;
- `external-process-rejects-backend-mismatch`: PASS under `WILL_FAIL`;
- `external-process-rejects-stale-capability-artifact`: PASS under `WILL_FAIL`, so a successful non-writing capability child could not inherit the old valid manifest;
- `external-process-rejects-stale-render-artifacts`: PASS under `WILL_FAIL`, so a successful non-writing render child could not inherit old valid output/receipt files;
- renderer capability publication and negotiation: PASS;
- native and independent flat render/receipt evidence: PASS;
- cross-backend receipt comparison: PASS;
- capability-bound receipt replay: PASS.

Continuity evidence observed in the same run remained:

- native 320x180 frame pixel digest: `0x456f404dd94c91da`;
- native output-file continuity digest: `0x7e33cf9168a97ab0`;
- native PPM SHA-256: `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`;
- flat 64x36 frame pixel digest: `0x5c48decbca7cb29f`;
- flat 64x36 PPM SHA-256: `346b6caaf38aea04d0b4cdf737f80dab7dee32c7e8940bd71527b58a83f57776`;
- flat same-intent 320x180 frame pixel digest: `0x5338e2b2729f1381`;
- flat same-intent 320x180 PPM SHA-256: `542fe181b96ce28c65dfed6a4e51fa09e1d8fdd477e9278364bc5dd1da85813c`.

The 320x180 native/flat receipt comparison remained `comparable_v1=YES`, while `same_frame_pixels_digest64=NO` and `same_output_file_digest64=NO`. No pixel- or visual-equivalence claim is made.

This file is an evidence-only follow-up to the passing code-bearing head. The final PR head containing this record must also pass CI before merge; this record does not authorize merging an unverified later head.

## Truth boundary

Fresh-artifact enforcement establishes only a bounded filesystem statement: the accepted artifact path was cleared before its producing phase and existed as a regular file after the selected child returned. It blocks passive reuse of a stale file by a child that writes nothing.

It does **not** authenticate which process wrote the new file, prevent races from another process with the same filesystem permissions, sandbox or trust the child, establish cryptographic provenance, or prove a real third-party renderer integration. The shared continuity digests remain non-cryptographic. No claim is made about visual quality, semantic equivalence, cross-machine determinism, GPU/WebGPU behavior, performance, memory use, or production security.

## Four-root gate

- **Truth:** process exit and produced evidence are now separate conditions; stale valid files cannot silently stand in for current output in the exercised cases.
- **Agency / non-domination:** only explicitly declared output/evidence paths are cleared, directories are refused, and renderer selection remains explicit.
- **Continuity:** no frozen scene/request/capability/receipt contract meaning changed; existing native and flat evidence remained continuous in CI.
- **Wisdom before speed:** the process boundary is tightened before attaching a real external renderer, while stronger authentication/sandboxing claims remain deferred until separately implemented and verified.
