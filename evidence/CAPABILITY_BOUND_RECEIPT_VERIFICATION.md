# Capability-Bound Receipt Verification Evidence

Date: 2026-09-14

PR: #14

First verified PR-head commit: `87437b4ae6fb0a26b9aa229f5d6997c64979e752`

GitHub Actions: `ci` run 148, job `build-and-test`, Ubuntu runner.

## Executable result

The PR-head CI configured and built the repository successfully. Its CTest step completed successfully with the existing suite plus three new capability-bound receipt cases:

- `receipt-verify-native-reference-with-capabilities`
- `receipt-verify-flat-reference-with-capabilities`
- `receipt-verify-rejects-capability-identity-mismatch` (`WILL_FAIL` negative case)

CI then regenerated both current renderer capability manifests, rendered the native and independent flat evidence frames, emitted their receipts, compared the same-intent receipts, and invoked capability-bound replay verification for both bodies. Every recorded workflow step through capability-bound replay completed successfully.

The existing CI continuity assertions also remained intact during this run:

- native reference `frame_pixels_digest64`: `0x456f404dd94c91da`
- independent flat same-intent `frame_pixels_digest64`: `0x5338e2b2729f1381`

No renderer pixel algorithm or frozen v1 contract meaning was changed by this improvement.

## What was added

`axm-render-verify-receipt` now accepts an optional third path:

```text
axm-render-verify-receipt REQUEST.axmrender RECEIPT.axmreceipt CAPABILITIES.axmcaps
```

The renderer-neutral `axm_render_contracts` library performs the original request/scene/output replay checks first, then checks the current `AXM_RENDER_CAPABILITIES 1` manifest against the request and receipt:

- request compatibility with the capability envelope;
- exact renderer identity match;
- exact renderer version match;
- backend match;
- scene-contract version match;
- render-request-contract version match;
- capability-file continuity across the ordinary verification interval.

The negative fixture deliberately preserves the native backend, request contract, scene contract, dimensions, format, and renderer version while changing only the renderer identity. Rejection therefore demonstrates that capability binding is not merely another backend-compatibility check.

## Truth boundary

This is **current-file corroboration**, not authentication.

`AXM_RENDER_RECEIPT 1` still does not contain a digest of the capability manifest, a signature, a source-commit attestation, or cryptographic proof that the renderer executable which produced the pixels is the renderer named by the manifest. A malicious or coordinated set of files can still agree with itself.

This evidence does not establish cryptographic provenance, protection from change-and-revert races, cross-machine determinism, visual quality, semantic or pixel equivalence across renderers, timing/performance, GPU behavior, or a real third-party renderer integration.

The capability digest used internally is the existing non-cryptographic continuity digest and is checked only for stability during the verification call; it is not added to receipt v1.

## Root gate

- **Truth:** renderer identity/version claims are no longer accepted from a replay-valid receipt without optional corroboration against the renderer's declared v1 capability manifest.
- **Agency / non-domination:** the verifier remains in `axm_render_contracts` and works without linking either renderer body.
- **Continuity:** the same verification path passed for both current pixel-producing bodies, giving future adapters one shared evidence route.
- **Wisdom before speed:** receipt/capability v1 meanings were left frozen; the change strengthens evidence around existing contracts instead of widening them prematurely.
