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

## Observed CI evidence

Executable code head `5b209dd871f4ce67c763bf8c87870f6ba76ab23c` passed all three PR workflows before this evidence-only update:

- Verified Dispatch Sampler run `34918642536`: PASS;
- standard `ci` run `34918642537`: PASS;
- `ghostscript-external` run `34918642544`: PASS.

The dedicated sampler ran on Ubuntu 24.04.5 with GCC 13.3.0. Five verified flat-renderer dispatches completed with one stable evidence identity:

- `renderer_process_continuity=PASS`;
- renderer `axm.contract.flat-demo`, version `0.1.0`, backend `axm.contract.cpu.flat`;
- renderer-process digest `0x1664974c06a85bfe`;
- capability digest `0xe26ebc23e2862fc1`;
- scene digest `0x660a6a3e429b6d1c`;
- request digest `0x141c5e5603de3d34`;
- frame-pixel digest `0x5338e2b2729f1381`;
- output-file digest `0xe47b2ea5622500df`;
- `verified_dispatch_sample_continuity=PASS`;
- `verified_dispatch_sampling=PASS`.

Observed timing samples, in microseconds:

- capability process: `1059,927,889,875,872` (min `872`, median `889`, max `1059`);
- render process: `2241,2236,2202,2175,2219` (min `2175`, median `2219`, max `2241`);
- receipt verification: `1228,1209,1211,1237,1216` (min `1209`, median `1216`, max `1237`);
- complete verified dispatch: `5904,5397,5355,5354,5354` (min `5354`, median `5355`, max `5904`).

These values are retained as one-run observations only. They are not promoted to renderer speed, throughput, latency SLA, or cross-machine performance claims.

This commit changes evidence text only. Merge still requires the final PR head, including this evidence update, to pass the repository workflows.
