# ImageMagick external renderer verification

Date: 2026-09-14

## What was exercised

This evidence record covers the first repository path that delegates actual pixel rasterization to a renderer implementation outside AXM Render Fabric rather than to another repository-owned renderer body.

The path exercised was:

```text
AXM_SCENE 1
  -> imagemagick_svg_renderer adapter
  -> SVG translation of ordered XY triangles + flat albedo
  -> external ImageMagick `convert` process
  -> raw RGB8 pixels
  -> adapter-owned PPM envelope + AXM_RENDER_RECEIPT 1
  -> axm-render-external capability/source/fresh-artifact/replay verification
```

The AXM native renderer was not replaced. The adapter links to `axm_render_contracts`, not `axm_render_native`, and publishes its own backend identity `external.imagemagick.svg-raster`.

## CI environment actually observed

Successful branch run: GitHub Actions run `34871137897`, head `14b21d6893474ef3e8d068fddb328f372c56c3f6`.

Observed runner/compiler/external dependency:

- Ubuntu 24.04.5 LTS, `ubuntu-24.04` runner image `20260907.300.1`;
- GNU C++ 13.3.0;
- ImageMagick `6.9.12-98 Q16 x86_64 18038`;
- ImageMagick reported built-in delegates including `pangocairo`, `raw`, and `xml` in that run.

The workflow installs ImageMagick when `convert` is absent, prints `convert -version`, and configures CMake only after the executable is available. The three ImageMagick-specific CTests are registered only when CMake finds `convert`.

## Executable evidence

The corrected branch build completed and CTest reported:

```text
100% tests passed, 0 tests failed out of 35
```

The external-renderer tests that passed were:

- `external-process-imagemagick-dispatch`;
- `external-process-imagemagick-compares-native-intent`;
- `external-process-imagemagick-rejects-native-backend`.

The dispatch route first asked the adapter to publish `AXM_RENDER_CAPABILITIES 1`, preflighted the request, cleared the declared render/receipt paths, launched the adapter for rendering, required fresh regular output artifacts, checked request/scene/capability continuity, and replay-verified the receipt against the current capability manifest.

Observed adapter receipt evidence for the 320x180 reference scene:

```text
renderer axm.adapter.imagemagick-svg
renderer_version 0.1.0
backend external.imagemagick.svg-raster
scene_contract 1
render_request_contract 1
scene_source_digest64 0x660a6a3e429b6d1c
request_source_digest64 0x383f33ce1024b2c9
width 320
height 180
format ppm-rgb8
frame_pixels_digest64 0x8d283e4b5e79fd9f
output_file_digest64 0x7e7f130f59bbe1d1
```

Observed SHA-256 for the produced PPM in that CI run:

```text
966bfb0142ed1c297886b2d5b0ce34c73182048d0b5cbc0b95f13edfd6bdfe2c
```

The existing native reference evidence in the same successful run remained:

```text
frame_pixels_digest64 0x456f404dd94c91da
SHA-256 c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb
```

`axm-render-compare` reported the native and ImageMagick-backed receipts as the same frozen v1 declared frame intent:

```text
same_scene_contract=YES
same_render_request_contract=YES
same_scene_source=YES
same_dimensions=YES
same_output_format=YES
comparable_v1=YES
same_frame_pixels_digest64=NO
same_output_file_digest64=NO
comparison_boundary=PIXEL_DIGEST_RELATION_REPORTED_NOT_REQUIRED
render_receipt_compare=PASS
```

That mismatch is expected and accepted. The ImageMagick path is an XY flat-albedo SVG translation; the AXM native path has its own triangle/depth/lighting semantics. Comparable declared input does not mean equivalent rendering behavior.

## Failure observed and repaired before acceptance

The first PR CI run (`34870968951`) failed during compilation because the adapter used a nonexistent `axm::render::Scene` type. Repository evidence showed the contract loader returns `axm::render::SceneState`. The adapter was corrected to consume the actual shared type, and the next branch run passed the complete 35-test suite and external-render evidence. The failed run is preserved as part of the development evidence rather than described as a pass.

## Four-root gate

**Truth:** the workflow invokes and prints the real external ImageMagick executable/version and accepts the result only after the existing capability/source/receipt verification chain passes. The pixel digest differs from native and is reported as different.

**Agency / non-domination:** AXM keeps its owned native renderer while an explicitly selected external backend can execute through the same renderer-neutral contracts. Backend mismatch is rejected rather than silently falling back.

**Continuity:** `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` meanings were not changed. The external route reuses the same frozen contract and evidence machinery.

**Wisdom before speed:** only the already-frozen triangle/albedo subset is translated. No depth, lighting, texture, material, animation, or richer-renderer feature is invented in v1 merely to make the adapter appear broader.

## Explicit truth boundary

This establishes one **real external process rasterization integration exercise** in the tested GitHub Actions environment. It is not a production external-renderer adapter claim.

The receipt binds the AXM adapter identity/version, backend, source bytes, dimensions/format, and produced pixels/output. `AXM_RENDER_RECEIPT 1` does **not** currently contain the delegated ImageMagick executable version or a digest/attestation of that executable. The version above is runner log evidence, not receipt-bound provenance.

SVG parsing/rasterization, antialiasing, colorspace handling, delegate behavior, and exact pixels may vary across ImageMagick builds or platforms. Therefore this evidence does not establish cross-machine determinism, cross-version pixel stability, visual quality, semantic equivalence with AXM native rendering, performance, GPU behavior, sandboxing, executable trust, or cryptographic provenance.

The continuity digests remain intentionally non-cryptographic. The SHA-256 listed above records the output observed in this run; it is not asserted as a universal golden output for all ImageMagick environments.
