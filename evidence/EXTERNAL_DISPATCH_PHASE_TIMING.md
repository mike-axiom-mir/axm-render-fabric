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

## Verified PR-head evidence

PR-head commit `af39371361bc3908097698234b5ab71f38e617fa` passed both repository workflows before this evidence update:

- standard `ci`: https://github.com/mike-axiom-mir/axm-render-fabric/actions/runs/34914861977 — PASS;
- dedicated `ghostscript-external`: https://github.com/mike-axiom-mir/axm-render-fabric/actions/runs/34914861991 — PASS;
- CTest: **39/39 passed**, including the strengthened `external-process-preserves-renderer-process-identity` timing assertions.

One standard-CI flat-renderer dispatch, with a previously inspected capability manifest bound to the fresh dispatch, observed:

- `timing_clock=steady`;
- `capability_process_elapsed_us=1373`;
- `render_process_elapsed_us=5635`;
- `receipt_verification_elapsed_us=3560`;
- `external_dispatch_elapsed_us=12992`;
- `expected_capabilities_match=PASS`;
- `external_process_dispatch=PASS`;
- unchanged flat same-intent pixel digest `0x5338e2b2729f1381`.

One standard-CI ImageMagick dispatch observed:

- `timing_clock=steady`;
- `capability_process_elapsed_us=6169`;
- `render_process_elapsed_us=20547`;
- `receipt_verification_elapsed_us=3731`;
- `external_dispatch_elapsed_us=33815`;
- `external_process_dispatch=PASS`;
- unchanged ImageMagick pixel digest `0x8d283e4b5e79fd9f`;
- unchanged PPM SHA-256 `966bfb0142ed1c297886b2d5b0ce34c73182048d0b5cbc0b95f13edfd6bdfe2c`.

Existing continuity evidence also remained intact in that run: native pixel digest `0x456f404dd94c91da`, native PPM SHA-256 `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`, flat 64x36 pixel digest `0x5c48decbca7cb29f`, and native-versus-flat `comparable_v1=YES` while their pixel/output digests remained different.

These timing values are intentionally recorded as **single-run observations only**. They are not baselines, targets, regressions thresholds, or evidence that the flat renderer is intrinsically faster than ImageMagick: the bodies do materially different work and the runner/process/cache state is not controlled as a benchmark.

Because this evidence file update changes the PR head, the final merge gate still requires both workflows to pass again on the new final head.

## Truth boundary

These timings are parent-observed elapsed wall-clock intervals from a monotonic/steady clock. They include process-launch/wait overhead in the two child-process fields and are affected by scheduler load, I/O, filesystem/cache state, runtime startup, and machine state. They do not isolate child CPU time, GPU time, allocator/process memory, VRAM, energy, or renderer-internal stages.

The timing values are not `AXM_RENDER_RECEIPT 1` fields, are not bound into the current receipt continuity evidence, are not cryptographically authenticated, and are not deterministic. A lower number in one run is not sufficient evidence that one renderer is generally faster than another. Cross-machine or cross-version performance claims remain outside the verified boundary.
