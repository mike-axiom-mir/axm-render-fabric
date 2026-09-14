# Negotiated capability -> dispatch binding evidence

## Grounded gap

`axm-render-negotiate` can scan multiple caller-supplied `AXM_RENDER_CAPABILITIES 1` manifests and report which ones satisfy one explicit request, while `axm-render-external` independently performs fresh capability discovery before launching a selected renderer process.

Before this change there was no executable way to say: **dispatch this explicitly selected renderer only if the capability manifest it publishes now still matches the manifest I previously inspected/selected**. A caller could negotiate against one manifest and later dispatch an executable whose fresh declaration had drifted.

## Improvement

`axm-render-external` now accepts optional:

```text
--expect-capabilities PREVIOUSLY_INSPECTED.axmcaps
```

When present, the harness:

1. continuity-digests and parses the expected manifest before child dispatch;
2. requires that expected manifest to accept the explicit request;
3. keeps the expected file distinct from request, scene, output, receipt, and fresh capability paths;
4. asks the selected renderer process to publish a fresh capability manifest as before;
5. compares the complete v1 capability meaning: renderer identity, renderer version, backend, scene/request contract versions, maximum dimensions, and output-format set;
6. rejects any drift before the render phase starts; and
7. re-checks that the expected manifest bytes did not change during the successful dispatch.

The option is additive. Existing callers that do not supply `--expect-capabilities` keep the previous fresh-discovery + request-preflight + receipt-replay path.

## Executable acceptance evidence

PR-head implementation commit `4739b4714e67ae53c9beb85f399826262e777b37` was exercised by both repository workflows before this evidence-only update:

- standard CI run `34895084478`: **PASS**;
- dedicated Ghostscript external-render run `34895084570`: **PASS**;
- configure/build: **PASS** on Ubuntu 24.04.5 with GCC 13.3.0;
- complete CTest suite: **38/38 PASS**.

The standard CI lane proved both directions:

- positive: a flat-renderer manifest generated before dispatch was supplied through `--expect-capabilities`; a separately generated fresh manifest matched the complete v1 capability meaning and dispatch completed with `expected_capabilities_match=PASS` and `external_process_dispatch=PASS`;
- observed expected-manifest continuity digest: `0xe26ebc23e2862fc1`;
- observed separately generated fresh-manifest digest: `0xe26ebc23e2862fc1`;
- resulting same-intent flat frame pixel digest remained `0x5338e2b2729f1381`;
- negative: an otherwise-compatible expected manifest was changed only from renderer `axm.contract.flat-demo` to `axm.contract.flat-demo-mismatch`; dispatch exited non-zero with a renderer-identity mismatch before the render phase produced a receipt.

Existing evidence paths remained green in the same run, including native rendering, flat rendering, capability negotiation, receipt replay, ImageMagick delegation, and the complete Ghostscript-enabled CTest suite. The native reference frame digest remained `0x456f404dd94c91da`; ImageMagick remained `0x8d283e4b5e79fd9f`. These continuity observations are not visual-quality or cross-machine-determinism claims.

## Four-root gate

- **Truth:** a prior negotiation result is not silently treated as current; the renderer must freshly redeclare the same v1 capability meaning at dispatch time.
- **Agency / non-domination:** the caller still chooses the renderer and expected manifest explicitly. No ranking, fallback, or automatic renderer substitution is introduced.
- **Continuity:** the existing frozen v1 scene/request/capability/receipt meanings are unchanged; this binds two already-existing phases rather than redefining either contract.
- **Wisdom before speed:** the launcher convention remains experimental. This adds one narrow continuity check instead of freezing a registry, discovery protocol, or selection policy.

## Truth boundary

This is semantic continuity checking between one caller-supplied capability file and one freshly generated capability file. It is not cryptographic authentication, executable/process identity proof, machine-wide renderer discovery, atomic filesystem snapshotting, race-proofing against a same-permission attacker, dependency-closure provenance, renderer ranking, visual equivalence, performance evidence, or cross-machine determinism.

For external adapters whose `renderer_version` already includes a delegated executable-byte continuity digest, matching that field carries that existing bounded identity signal through this check; it does not strengthen that digest into cryptographic provenance.
