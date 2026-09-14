# Architecture

## Layers

### 1. Canonical visual / scene state
Describes what should exist: geometry, transforms, materials, lights, cameras, environment, animation state, and provenance. The full future contract is intentionally not frozen in the current implementation.

The repository owns one deliberately tiny frozen interchange subset, `AXM_SCENE 1`, containing only ordered triangles with positions and flat RGB-byte albedo. That subset is useful as an executable interoperability foothold without pretending it is the whole canonical model. Incompatible changes to the v1 on-disk meaning require a new version.

### 2. Render contract
Defines the minimum translation boundary between state and a renderer. The repository owns a deliberately tiny frozen `AXM_RENDER_REQUEST 1` envelope for scene path, backend identifier, output dimensions, `ppm-rgb8`, and output path. Unsupported request versions/directives are rejected, and renderer bodies must reject backend identifiers they do not implement rather than silently falling back.

This is still narrower than the future render contract: richer settings/formats, explicit feature requests, and fallback policy remain separate gates.

### 2.5 Renderer capability discovery
A renderer body should be able to declare what frozen contract subset it accepts before a dispatcher asks it to render.

The repository owns `AXM_RENDER_CAPABILITIES 1`, a small renderer-neutral declaration containing renderer/version identity, backend identifier, supported scene/request contract versions, maximum request dimensions, and supported output formats. The current native, flat, and ImageMagick adapter bodies can each publish this manifest themselves.

`axm-render-negotiate` checks one `AXM_RENDER_REQUEST 1` plus its referenced `AXM_SCENE 1` source against one capability manifest. Unsupported backend, dimensions, format, or contract versions are explicit incompatibilities. Capability v1 deliberately does not invent feature-level claims for depth, lighting, materials, textures, animation, or other semantics not represented in request v1.

### 3. Renderer bodies
Multiple bodies may coexist:

- AXM native CPU/GPU renderer
- native GPU backend(s)
- WebGPU/browser backend
- game/runtime backend
- external renderer adapters
- future unknown backends

### 4. Residency / derivation policy
A renderer body may choose how much derived render data to keep resident. Expanded geometry, transformed vertices, GPU buffers, acceleration structures, texture residency, caches, and history are not automatically canonical truth.

Two strategies are deliberately kept side by side:

- **resident baseline** — retain expanded render representations where useful;
- **state-native research path** — retain compact canonical state and reconstruct, stream, cache, or evict derived data under explicit policies.

The goal is not to assume that state-native rendering is superior. It is to measure when the same declared scene/output can be preserved with lower residency and what reconstruction, bandwidth, startup, or frame-time costs are introduced.

### 5. Pixel / frame evidence
A render result should be accompanied by enough metadata to say which state, renderer version, backend, settings, and output digest produced it.

The repository now owns `AXM_RENDER_RECEIPT 1`, a deliberately small renderer-neutral evidence envelope. It records renderer/backend identity, scene/request contract versions, exact source-byte continuity digests, dimensions/format, a pixel-buffer digest, and an output-file digest. The format has strict read/write support in the renderer-neutral contract library so future adapters can emit and consume the same evidence shape.

Receipt v1 intentionally does not claim cryptographic integrity, exact source-commit attestation, timing, memory metrics, cross-machine determinism, or visual quality. Those require separate evidence.

The `axm-render-compare` executable consumes two receipt-v1 records and checks whether they describe comparable v1 declared frame intent. Under frozen request v1, backend and output path are renderer-specific; the body-independent fields available in the receipt are scene/request contract versions, exact scene-source digest, dimensions, and output format. Pixel and output-file digest equality are reported separately and are not required for comparability.

### 6. Observer loop
A visual observer may inspect actual rendered output and propose state revisions. The observer is downstream of evidence; it must not claim to have seen a frame that was never rendered and inspected.

## Current implementation boundaries

Renderer-neutral scene/request/receipt/capability implementations are separated from the AXM-owned renderer body:

```text
AXM_RENDER_REQUEST v1 ----+
                          |
AXM_SCENE v1 file --------+--> axm_render_contracts
                          |        |
AXM_RENDER_CAPABILITIES v1+--------+-----------------------------+
                                   |                             |
                 +-----------------+------------------+          v
                 |                 |                  |   axm-render-negotiate
                 v                 v                  v   explicit compatibility
        axm_render_native  contract_adapter_probe  contract_flat_renderer
                 |                 |                  |
        triangle rasterizer  contract validation   independent XY flat fill
        depth test + lighting source digests       no depth / no lighting
                 |                 |                  |
            pixels / PPM      no pixel output       pixels / PPM
                 |                                    |
                 +--> AXM_RENDER_RECEIPT v1 <---------+
                                  |
                                  +-------------------------------+
                                  |                               |
                                  v                               v
                         axm-render-compare               axm-render-external
                    comparable intent evidence          process-boundary verify
                    pixel relation reported only                 |
                                                                v
                                                imagemagick_svg_renderer
                                                   AXM scene -> SVG adapter
                                                                |
                                                                v
                                               external ImageMagick `convert`
                                                  raw RGB8 rasterization
                                                                |
                                                                v
                                                PPM + shared receipt evidence
```

`include/axm/render/scene_contract.hpp` owns the renderer-neutral v1 primitive scene types and loader. `include/axm/render/render_contract.hpp` owns the v1 render-request envelope and backend identifier convention. `include/axm/render/render_receipt.hpp` owns the v1 evidence record, continuity file-digest helper, strict parser, and writer. `include/axm/render/render_capabilities.hpp` owns the v1 capability declaration plus request-compatibility check. Those implementations build as `axm_render_contracts` / `axm::render_contracts`.

`include/axm/render/reference_renderer.hpp` exposes the AXM-owned native image buffer, renderer, renderer version, and existing frame hash. `axm_render_native` contains the renderer body and depends on `axm_render_contracts` instead of owning renderer-neutral parsing/evidence code itself.

The `contract_adapter_probe` executable links only to `axm_render_contracts`. It accepts backend identifier `axm.mock.contract-probe`, loads a render request and scene, computes the same source-byte continuity digests exposed by the receipt contract, and reports the contract versions it consumed. It rejects a request selecting the native backend instead of silently substituting itself. It deliberately writes no pixels and no receipt.

The `contract_flat_renderer` executable also links only to `axm_render_contracts`, but crosses the next interoperability gate: it consumes the same request and scene contracts, rasterizes ordered triangles with an independent XY flat-fill path, writes `ppm-rgb8`, and emits an `AXM_RENDER_RECEIPT 1`. Its backend is `axm.contract.cpu.flat`; it rejects requests for the native backend rather than silently substituting itself. It deliberately has no depth test, lighting, textures, or native-renderer dependency, and its receipt names its distinct renderer/backend identity.

The `imagemagick_svg_renderer` executable is the first integration exercise whose actual pixel rasterization is delegated to a renderer implementation outside this repository. It links only to `axm_render_contracts`, accepts backend `external.imagemagick.svg-raster`, translates the frozen `AXM_SCENE 1` ordered-triangle/albedo subset to SVG, launches the external ImageMagick `convert` process, reads its raw RGB8 raster output, writes the requested `ppm-rgb8` envelope, and emits the same `AXM_RENDER_RECEIPT 1` contract. It is driven through `axm-render-external`, so capability preflight, fresh-artifact enforcement, source continuity checks, and capability-bound receipt replay apply unchanged. Backend mismatch is rejected instead of falling back.

The ImageMagick adapter deliberately does not claim depth, lighting, texture, material, animation, or native-renderer semantics. Its adapter version is now composed with a non-cryptographic continuity digest of the canonical delegated ImageMagick executable bytes (`0.2.0+delegated-fnv64-...`), and the same token is emitted in capability and receipt evidence. The adapter checks those executable bytes remain unchanged across each capability/render phase, and capability-bound replay already requires the capability and receipt renderer versions to match. This binds the observed delegated executable bytes without changing receipt v1 fields. It does not bind ImageMagick semantic-version text as a contract field, dynamically loaded libraries/delegates, policy/configuration, environment, cryptographic provenance, or atomic filesystem race freedom.

The native `axm-render` executable, `contract_flat_renderer`, and `imagemagick_svg_renderer` can each write their own `AXM_RENDER_CAPABILITIES 1` manifest. `axm-render-negotiate` links only to `axm_render_contracts`; it validates the referenced scene with the frozen scene parser and compares the request against the selected manifest. It does not choose another backend, rank renderers, or infer future feature support.

The `axm-render-compare` executable links only to `axm_render_contracts`. It does not render and does not depend on either renderer body. It consumes two strict receipt-v1 files, rejects them as incomparable if the declared body-independent v1 frame fields differ, and otherwise reports whether their frame/output continuity digests match. A pixel digest match is only an observed digest relation; a mismatch is allowed for comparable requests and neither outcome is a visual-quality or semantic-equivalence claim.

The native CLI accepts either direct flags or `--request PATH`; request-selected backend identifiers other than `axm.native.cpu.reference` fail explicitly. `--receipt PATH` is deliberately narrower: it currently requires a request-backed render so the receipt can bind explicit scene/request source files rather than inventing missing provenance. `--capabilities PATH` is a discovery-only operation and cannot be combined with rendering arguments.

This proves the AXM-owned renderer can be embedded, renderer-neutral scene/request/receipt/capability code can be consumed without linking the native renderer body, a second repository-owned pixel-producing body can emit the shared receipt contract, an adapter can delegate actual rasterization to a real external ImageMagick process behind the same v1 capability/request/receipt boundary, the observed delegated executable bytes can contribute a non-cryptographic composite renderer identity without changing frozen receipt fields, explicit capability declarations can be checked before dispatch, and shared receipt evidence can compare declared v1 frame intent across bodies without pretending their pixels must match. It does **not** prove a stable C++ ABI/API, a production-grade external renderer integration, semantic-version or dependency-closure attestation of the delegated renderer, feature-level negotiation beyond request v1, automatic fallback selection, cross-backend pixel equivalence, cryptographic provenance, or a complete canonical scene/render model.

The repository also contains a first synthetic state-residency benchmark:

```text
resident expanded copies ------------------+
                                            +--> visible-geometry digest
shared base mesh + compact instance state --+
                 |
                 +--> on-demand expansion
```

The benchmark reports modeled owned bytes and digest equivalence only. It is not a process-RSS or GPU-VRAM measurement.

## State-native research gates

Before treating state-native rendering as a production direction, progressively test:

1. visible-working-set materialization;
2. reconstructable geometry and materials;
3. procedural asset state with versioned seeds/recipes;
4. disposable caches with explicit invalidation;
5. dirty-state propagation;
6. state streaming for worlds larger than resident memory;
7. equivalent scene reconstruction across renderer bodies;
8. memory-pressure adaptation;
9. state-delta update paths;
10. render receipts that name state, backend, settings, output digest, timing, and exact memory metric.

Receipt v1 covers only part of gate 10: state/request source identity, backend/settings, and output continuity digests. Timing and exact memory metrics remain unimplemented rather than implied.

See `research/state-native-rendering/README.md`.

## Next likely gates

1. Exercise a second materially different external renderer before freezing a launcher protocol, or add stronger delegated-runtime provenance only through an explicit additive/new evidence contract if evidence needs exceed the current non-cryptographic executable-byte binding.
2. Add a first GPU backend while retaining the CPU reference path and publishing its own capability manifest.
3. Extend capability/request contracts only when richer scene features require explicit feature-level negotiation; use a new compatible contract version rather than silently changing v1 meaning.
4. Add browser/WebGPU only after the shared contract is strong enough to avoid unrelated renderer islands.
5. Extend evidence with explicitly named timing and real allocator/process measurements; add GPU-VRAM evidence only when a GPU backend exists.
6. Add a cryptographic digest/provenance layer only with a new compatible receipt version or explicitly additive contract, never by silently changing receipt v1 digest meaning.
