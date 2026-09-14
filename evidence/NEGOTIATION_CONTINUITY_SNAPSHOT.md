# Negotiation continuity snapshot evidence

## Grounded gap

`axm-render-negotiate` already checked whether one or more `AXM_RENDER_CAPABILITIES 1` manifests were semantically compatible with one explicit `AXM_RENDER_REQUEST 1`, but its output did not identify the exact request, scene, or capability bytes that it inspected.

That left a small evidence gap between negotiation and the later `axm-render-external --expect-capabilities` dispatch path, which already reports continuity digests. A caller could see a compatibility result, but could not directly correlate that result with the exact byte snapshots later observed during dispatch or receipt verification.

## Improvement

The negotiation executable now:

1. continuity-digests the request before parsing it and requires the bytes to remain unchanged during the scan;
2. continuity-digests the referenced scene before parsing it and requires the bytes to remain unchanged during the scan;
3. continuity-digests each capability manifest before parsing and requires that manifest to remain unchanged while it is inspected;
4. emits `request_source_digest64`, `scene_source_digest64`, and one `capabilities_digest64` per manifest alongside the existing compatibility result; and
5. preserves the existing policy boundary: it reports compatibility but does not rank, select, launch, or silently substitute a renderer.

No frozen `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, or `AXM_RENDER_RECEIPT 1` field or meaning changed.

## Executable verification

PR-head implementation commit `149ea38df517f3f746e2c3a2d480abe11d097da5` was exercised by both pull-request workflows:

- standard CI run `34900660718`: **PASS**;
- dedicated Ghostscript external-render run `34900660973`: **PASS**;
- Ubuntu 24.04.5 / GCC 13.3.0 configure and build: **PASS**;
- complete CTest suite with Ghostscript present: **38/38 PASS**.

Observed negotiation snapshots in the standard CI run:

### Native request + native capability manifest

- request source digest: `0x20c8519ed81dacef`;
- scene source digest: `0x660a6a3e429b6d1c`;
- native capability manifest digest: `0x119dbeb7656b41d5`;
- compatibility: **YES**.

The later native render receipt in the same run reported the same request digest `0x20c8519ed81dacef` and scene digest `0x660a6a3e429b6d1c`, while the known native frame digest remained `0x456f404dd94c91da`.

### Flat request + flat capability manifest

- request source digest: `0x141c5e5603de3d34`;
- scene source digest: `0x660a6a3e429b6d1c`;
- flat capability manifest digest: `0xe26ebc23e2862fc1`;
- compatibility: **YES**.

The existing explicit dispatch-binding lane then supplied that same flat manifest through `--expect-capabilities` and reported:

- expected capability digest: `0xe26ebc23e2862fc1`;
- freshly generated capability digest: `0xe26ebc23e2862fc1`;
- request source digest: `0x141c5e5603de3d34`;
- scene source digest: `0x660a6a3e429b6d1c`;
- `expected_capabilities_match=PASS`;
- `external_process_dispatch=PASS`;
- same-intent flat frame digest remained `0x5338e2b2729f1381`.

This is the intended new evidence chain: the byte snapshot reported during negotiation can be compared directly with the independently observed byte snapshot at dispatch.

### Multi-manifest scan

The native request was scanned against the flat and native manifests in one invocation:

- `manifest_count=2`;
- flat manifest digest `0xe26ebc23e2862fc1`: **NO**, explicit backend mismatch;
- native manifest digest `0x119dbeb7656b41d5`: **YES**;
- `compatible_count=1`.

The tool still did not choose or launch either renderer.

## Continuity observations, not quality claims

The same CI run kept the existing native, flat, ImageMagick, Ghostscript, receipt-replay, stale-artifact rejection, and state-residency paths green. ImageMagick remained externally delegated with its existing bounded executable identity evidence. The native and external bodies continue to be compared only at the declared v1 intent boundary; differing pixel digests are not promoted to visual equivalence.

## Four-root merge gate

- **Truth:** negotiation now names the exact byte snapshots used for its compatibility statement and rejects visible source changes during the scan.
- **Agency / non-domination:** compatibility remains advisory evidence. No renderer ranking, fallback, auto-selection, or substitution was introduced.
- **Continuity:** the change reuses existing continuity-digest semantics and leaves all frozen v1 contracts unchanged while making negotiation evidence easier to correlate with later dispatch/receipt evidence.
- **Wisdom before speed:** the change strengthens one existing boundary rather than prematurely freezing a machine-wide registry, launcher protocol, GPU abstraction, or richer feature contract.

## Truth boundary

The emitted values are the repository's existing 64-bit non-cryptographic continuity digests. They are not cryptographic authentication, collision-resistant provenance, executable/process identity, an atomic filesystem snapshot, or protection against a same-permission actor changing bytes and restoring them between checks.

Matching negotiation and dispatch digests demonstrates observed byte continuity for the checked files in the exercised run; it does not prove that the two phases are cryptographically bound, that a renderer is trusted, that pixels are visually equivalent, that behavior is deterministic across machines, or that performance/security properties have been established.
