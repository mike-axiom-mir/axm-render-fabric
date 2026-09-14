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

The standard CI lane is required to prove both directions:

- positive: a flat-renderer manifest generated before dispatch is supplied as the expected manifest; a separately generated fresh manifest must match and the render/receipt path must complete with `expected_capabilities_match=PASS`;
- negative: the same otherwise-compatible flat manifest is altered only in renderer identity; fresh capability discovery must reject the mismatch before a receipt is produced.

Final observed run/commit evidence is recorded here only after the PR-head workflow finishes.

## Four-root gate

- **Truth:** a prior negotiation result is not silently treated as current; the renderer must freshly redeclare the same v1 capability meaning at dispatch time.
- **Agency / non-domination:** the caller still chooses the renderer and expected manifest explicitly. No ranking, fallback, or automatic renderer substitution is introduced.
- **Continuity:** the existing frozen v1 scene/request/capability/receipt meanings are unchanged; this binds two already-existing phases rather than redefining either contract.
- **Wisdom before speed:** the launcher convention remains experimental. This adds one narrow continuity check instead of freezing a registry, discovery protocol, or selection policy.

## Truth boundary

This is semantic continuity checking between one caller-supplied capability file and one freshly generated capability file. It is not cryptographic authentication, executable/process identity proof, machine-wide renderer discovery, atomic filesystem snapshotting, race-proofing against a same-permission attacker, dependency-closure provenance, renderer ranking, visual equivalence, performance evidence, or cross-machine determinism.

For external adapters whose `renderer_version` already includes a delegated executable-byte continuity digest, matching that field carries that existing bounded identity signal through this check; it does not strengthen that digest into cryptographic provenance.
