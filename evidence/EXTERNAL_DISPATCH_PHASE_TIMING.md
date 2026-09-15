# External dispatch phase timing evidence

## Grounded gap

`ARCHITECTURE.md` keeps timing and exact resource measurements outside `AXM_RENDER_RECEIPT 1`, and `docs/TRUTH_BOUNDARY.md` correctly says receipt timing is not yet claimed. At the same time, `axm-render-external` already has a concrete multi-phase process boundary: capability discovery, render dispatch, and renderer-neutral receipt replay. Before this change the harness exposed no direct elapsed-time observations for those phases, so later backend/process work had no executable timing surface to compare under controlled conditions.

## Improvement

`axm-render-external` now uses `std::chrono::steady_clock` in the parent harness and, after a successful verified dispatch, reports:

- `capability_process_elapsed_us` for launching/waiting on the capability child process;
- `render_process_elapsed_us` for launching/waiting on the render child process;
- `receipt_verification_elapsed_us` for receipt/capability/output replay verification plus receipt reload;
- `external_dispatch_elapsed_us` for the full harness interval after argument parsing through final successful verification/process-continuity checking;
- `timing_clock=steady` to make the clock class explicit.

No frozen scene/request/capability/receipt field or meaning changes.

The existing `external-process-preserves-renderer-process-identity` CTest now also requires all four numeric timing fields plus the timing-clock declaration on its successful dispatch. That keeps this measurement surface executable instead of documentation-only.

## Four-root gate

- **Truth:** each value names the measured parent-side interval and unit; no child CPU/GPU-time or renderer-speed interpretation is inferred.
- **Agency / non-domination:** timing does not rank, select, substitute, or privilege a renderer. The caller still selects the renderer explicitly.
- **Continuity:** frozen v1 contracts remain unchanged; timing is additive process evidence printed by the experimental harness.
- **Wisdom before speed:** this adds a minimal measurement foothold before designing receipt-v2 performance fields, benchmarking policy, or GPU timing semantics.

## Verification gate

Merge requires the final PR head to pass both repository workflows. The standard CTest lane must exercise the strengthened renderer-process test and require the new timing fields while preserving all existing render/evidence tests.

Observed CI values should be recorded here only after a final passing PR-head run; they are one-run observations, not baselines or performance targets.

## Truth boundary

These timings are parent-observed elapsed wall-clock intervals from a monotonic/steady clock. They include process-launch/wait overhead in the two child-process fields and are affected by scheduler load, I/O, filesystem/cache state, runtime startup, and machine state. They do not isolate child CPU time, GPU time, allocator/process memory, VRAM, energy, or renderer-internal stages.

The timing values are not `AXM_RENDER_RECEIPT 1` fields, are not bound into the current receipt continuity evidence, are not cryptographically authenticated, and are not deterministic. A lower number in one run is not sufficient evidence that one renderer is generally faster than another. Cross-machine or cross-version performance claims remain outside the verified boundary.
