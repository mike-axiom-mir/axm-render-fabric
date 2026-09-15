# Verified dispatch timing sampler

## Grounded gap

`axm-render-external` already emits parent-observed steady-clock timing for one verified dispatch, but one observation is easy to overread as a performance result. The repository had no small executable helper for repeating the *same explicitly selected renderer/request* while requiring the render/evidence identity to stay stable across the sample set.

## Improvement

`tools/verified_dispatch_sample.py` repeatedly invokes `axm-render-external` for one caller-selected renderer and one unchanged request. Every accepted sample must finish with `external_process_dispatch=PASS` and `timing_clock=steady`.

Across the sample set the helper requires these fields to remain identical:

- renderer process resolution, canonical path, executable continuity digest, and continuity status;
- renderer identity/version and backend;
- capability-manifest digest;
- scene and request source digests;
- frame-pixel and output-file digests.

Only the four existing parent-observed timing fields are allowed to vary. The helper reports the raw samples plus minimum, median, and maximum for each field. It does not compare two renderers and does not choose or rank a renderer.

A dedicated CI workflow builds only `axm-render-external` plus `contract_flat_renderer`, runs five verified samples of the existing flat-renderer reference request, and requires the bounded summary/continuity markers.

## Root gate

- **Truth:** timing summaries are accepted only from fully verified dispatches whose declared render/evidence identity remained stable across the sample set. Raw timing samples are retained in the summary instead of presenting only one aggregate.
- **Agency / non-domination:** the caller explicitly supplies the renderer and request. The helper does not discover, substitute, rank, or auto-select a renderer.
- **Continuity:** frozen `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` meanings are unchanged; the helper consumes existing harness output.
- **Wisdom before speed:** this adds repeatable observation before any renderer-performance policy or benchmark claim. Variation is surfaced rather than hidden.

## Truth boundary

This is repeated observation on one runner, not a controlled performance benchmark. The values remain parent-observed wall-clock elapsed time and include process startup/wait, scheduler effects, I/O, cache state, and machine load. The helper does not measure child CPU time, GPU time, memory/VRAM, energy, visual quality, or cross-machine determinism. Stable 64-bit continuity digests remain non-cryptographic change detectors, not authentication.

Minimum/median/maximum describe only the samples supplied to one invocation. They are not evidence that a renderer is faster or slower than another renderer, and the helper intentionally contains no cross-renderer ranking logic.

## Final-head evidence

Pending final PR-head CI. Merge only after the existing repository workflows and the new verified-dispatch sampler workflow are green; replace this section with the observed sample output before merge.
