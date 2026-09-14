# Multi-manifest capability negotiation verification

## Change under test

`axm-render-negotiate` now accepts one render request followed by one or more `AXM_RENDER_CAPABILITIES 1` manifests. It validates the request and referenced `AXM_SCENE 1` source once, checks each caller-supplied manifest independently, reports a compatibility result and reason per manifest, and returns success only when at least one supplied manifest is compatible.

The tool does not rewrite the request backend, rank renderers, choose one renderer, dispatch a render, or silently fall back. The existing one-manifest use remains valid.

## Observed CI evidence

PR #20 head `94f58d37c45194924dca048700cafaec1ffc6ceb` was exercised by GitHub Actions on Ubuntu 24.04.5 / GCC 13.3.0.

CI run: https://github.com/mike-axiom-mir/axm-render-fabric/actions/runs/34888720831

Observed results:

- configure: PASS
- build: PASS
- complete CTest suite: PASS, 38/38 tests
- existing single-manifest native negotiation: PASS
- existing single-manifest flat negotiation: PASS
- existing explicit backend-mismatch rejection: PASS
- mixed scan of one native request against flat + native manifests: PASS
- mixed scan reported `manifest_count=2`
- flat manifest reported `compatible=NO` with explicit backend-mismatch reason
- native manifest reported `compatible=YES`
- mixed scan reported `compatible_count=1`
- all existing native, flat, ImageMagick, Ghostscript, receipt-replay, external-dispatch, stale-artifact rejection, and state-residency tests remained green

The dedicated Ghostscript workflow for the same PR head also passed:
https://github.com/mike-axiom-mir/axm-render-fabric/actions/runs/34888720820

## Root gate

- **Truth:** each supplied manifest is checked and reported independently. A mismatch remains visible instead of being hidden by the presence of another compatible renderer.
- **Agency / non-domination:** manifests are supplied explicitly by the caller and the tool does not rank, choose, substitute, or launch a renderer.
- **Continuity:** `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, and `AXM_RENDER_CAPABILITIES 1` meanings are unchanged; existing single-manifest invocation remains supported.
- **Wisdom before speed:** this adds only bounded compatibility scanning around the already-frozen v1 envelope. It does not freeze the external launcher convention or invent feature-level negotiation.

## Truth boundary

This evidence proves only that the tested executable can evaluate multiple current capability manifests against one valid v1 request and surface compatible/incompatible results in the tested environment.

It does **not** prove:

- machine-wide renderer discovery;
- automatic fallback or backend selection;
- renderer ranking or quality preference;
- that a manifest's renderer executable is currently installed or launchable;
- visual or pixel equivalence between compatible renderers;
- performance equivalence;
- cryptographic trust or provenance of a manifest;
- cross-machine determinism;
- richer feature negotiation beyond the fields represented in request/capability v1.

Actual renderer execution and output evidence remain the responsibility of the existing dispatch and receipt-verification path.
