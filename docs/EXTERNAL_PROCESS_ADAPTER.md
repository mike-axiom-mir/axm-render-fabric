# External renderer process adapter harness

`axm-render-external` is a renderer-neutral process-boundary harness for testing renderer adapters without linking their renderer body into AXM.

It preserves the current architecture: AXM still owns its native substrate, while another renderer body may live behind the shared `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` files.

## Current executable flow

The harness receives an explicit renderer executable plus request, receipt, and capability-manifest paths.

It then:

1. continuity-checks and parses the request and referenced scene;
2. rejects source/evidence/output path collisions that could overwrite declared state;
3. clears any pre-existing file at the declared capability-manifest path, launches the renderer for capability discovery, and requires that invocation to create a regular capability file;
4. parses the newly emitted capability manifest and rejects an incompatible request before rendering;
5. clears any pre-existing files at the declared render-output and receipt paths, then launches the renderer as a separate process for the render;
6. requires that render invocation to create both output and receipt as regular files;
7. checks that request, scene, and capability bytes did not change across dispatch; and
8. replay-verifies the produced receipt, output bytes, pixel digest, and renderer identity/version/backend against the capability manifest created for the same dispatch.

The harness links only `axm_render_contracts`. It does not link `axm_render_native` or the dispatched renderer body.

The capability, output, and receipt paths are therefore treated as declared outputs owned by this invocation. Existing non-directory entries at those exact paths are removed before the relevant child phase. Existing directories are rejected instead of recursively removed. Produced symlinks and other non-regular artifacts are rejected. This prevents a renderer that exits successfully without writing from accidentally inheriting stale evidence from an earlier run.

## Experimental process convention

The current harness invokes an adapter executable as:

```text
RENDERER --capabilities CAPS.axmcaps
RENDERER REQUEST.axmrender RECEIPT.axmreceipt
```

This invocation convention is deliberately **not frozen as an AXM contract version yet**. It remains executable research for the process boundary. The first real external-rasterizer exercise now exists, but one working integration is still too little evidence to freeze a launcher protocol for future renderers with different invocation and lifecycle needs.

## Current exercises

Two materially different paths now exercise the harness:

1. `contract_flat_renderer` is a repository-owned independent renderer body. It proves that another body can publish capabilities, write pixels/receipts, and be verified through a separate process without linking the AXM native renderer.
2. `imagemagick_svg_renderer` is an AXM adapter that delegates actual rasterization to the external ImageMagick `convert` executable. The adapter translates the frozen `AXM_SCENE 1` triangle/albedo subset to SVG, asks ImageMagick for raw RGB8 pixels, wraps those pixels in the requested PPM envelope, and writes the shared receipt. Its backend identity is `external.imagemagick.svg-raster` and mismatched backend requests are rejected rather than silently redirected.

The ImageMagick path is CI-exercised through `axm-render-external`, not invoked as a bypass around the harness. CI records the actual ImageMagick version used for the observed run and compares the resulting receipt with the native receipt only at the existing body-independent `comparable_v1` intent boundary.

## Truth boundary

This harness demonstrates process separation plus contract/evidence checking when exercised successfully. Requiring newly created artifact files strengthens the statement that the accepted files came from the current dispatch attempt rather than being silently reused from an earlier run. It still does not prove that the child executable is trustworthy, sandboxed, or cryptographically authenticated. An explicitly selected child process has the same operating-system permissions as the invoking user unless the surrounding runtime constrains it.

The ImageMagick exercise establishes that a real external renderer process can sit behind the current AXM contract boundary for the frozen triangle/albedo subset in the tested environment. It does **not** establish a production-grade third-party renderer integration or broad scene-feature portability. The adapter uses an XY SVG translation with flat albedo; it does not preserve the native renderer's depth or lighting behavior and makes no claim of equivalent pixels or equivalent visual semantics.

`AXM_RENDER_RECEIPT 1` and the current continuity digests remain non-cryptographic evidence. Fresh-path enforcement is not process attestation: a child may still invoke other programs, race with another process that has the same filesystem permissions, or deliberately write misleading bytes that only later contract verification can reject. The receipt identifies the AXM adapter version, but it does not currently bind the delegated ImageMagick executable version or executable digest. CI log evidence for an ImageMagick version is therefore not cryptographic or receipt-bound provenance.

Successful replay does not establish visual quality, semantic equivalence between renderer bodies, cross-machine or cross-version determinism, GPU behavior, production performance, sandboxing, or provenance of the executable itself. Exact ImageMagick pixels may vary with platform, build, delegates, SVG rasterization, antialiasing, and colorspace behavior.

See `evidence/IMAGEMAGICK_EXTERNAL_RENDERER_VERIFICATION.md` for the observed CI environment, receipt/output digests, native comparison result, first failed compile run, repaired passing run, and explicit four-root gate.
