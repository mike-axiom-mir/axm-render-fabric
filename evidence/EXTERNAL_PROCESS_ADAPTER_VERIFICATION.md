# External process adapter verification

## Grounded gap

Before this change, the repository had renderer-neutral file contracts, a second pixel-producing renderer body, capability negotiation, receipt comparison, and receipt replay verification. However, all interoperability exercises still launched each executable directly from tests/CI. There was no renderer-neutral executable that owned the sequence **discover capabilities -> reject incompatible request -> launch a separate renderer process -> replay-verify its evidence**.

## Improvement under test

`axm-render-external` links only `axm_render_contracts` and accepts an explicitly selected renderer executable.

The first executable exercise uses `contract_flat_renderer`, which is independent of `axm_render_native` and already emits `AXM_RENDER_CAPABILITIES 1`, pixels, and `AXM_RENDER_RECEIPT 1`.

Registered CTest coverage adds:

- `external-process-flat-dispatch`: capability discovery, compatible-request preflight, child-process render, and capability-bound receipt replay must all succeed;
- `external-process-rejects-backend-mismatch`: the same flat renderer presented with the native request must fail during capability preflight rather than rendering or silently substituting a backend.

The adapter also rejects collisions among request source, scene source, render output, receipt, and capability-manifest paths before launching the render phase.

## Verified PR-head evidence

GitHub Actions run `34858289131` exercised PR #15 head `ef234991f8674b0b65da03d95b7f21fc02918a2d` on Ubuntu 24.04.5 with GCC 13.3.0 and completed successfully.

Observed results:

- CMake configure: PASS;
- build: PASS, including `axm-render-external`;
- CTest: **30/30 PASS**;
- `external-process-flat-dispatch`: PASS;
- `external-process-rejects-backend-mismatch`: PASS under `WILL_FAIL`, so the incompatible native-backend request was rejected as required;
- existing capability publication/negotiation: PASS;
- existing native and flat render/receipt evidence: PASS;
- existing cross-backend receipt comparison: PASS;
- existing capability-bound receipt replay: PASS.

Continuity values observed in that run remained:

- native 320x180 frame pixel digest: `0x456f404dd94c91da`;
- native output-file continuity digest: `0x7e33cf9168a97ab0`;
- native PPM SHA-256: `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`;
- flat 64x36 frame pixel digest: `0x5c48decbca7cb29f`;
- flat 320x180 same-intent frame pixel digest: `0x5338e2b2729f1381`.

The same-intent native/flat receipt comparison remained `comparable_v1=YES` while their pixel and output digests remained different. That is interoperability evidence for shared declared intent, not visual or pixel equivalence.

This evidence section records the passing code-bearing PR head. The final evidence-only commit that adds this record must also pass CI before merge; a green earlier head is not treated as evidence for an unverified later head.

## Truth boundary

This is a **process-boundary adapter harness**, not a real third-party renderer integration. The exercised child renderer is built from this repository and shares the file-contract library, although the parent does not link the child renderer body.

The child process is not sandboxed or authenticated. Receipt/capability continuity digests are not cryptographic provenance. This change does not establish visual quality, native/flat visual equivalence, cross-machine determinism, GPU/WebGPU behavior, performance, memory use, or a frozen launcher protocol.

## Four-root gate

- **Truth:** compatibility is checked before dispatch and produced pixels/receipt are replay-verified after the child exits; unsupported claims remain excluded.
- **Agency / non-domination:** the parent depends on renderer-neutral contracts, not the AXM native body, and the renderer executable is explicit rather than silently selected.
- **Continuity:** request/scene/capability bytes are checked across the process boundary and the existing receipt verifier binds output back to declared state.
- **Wisdom before speed:** the process invocation convention remains experimental until real external integrations justify freezing it.
