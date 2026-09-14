# External process adapter verification

## Grounded gap

Before this change, the repository had renderer-neutral file contracts, a second pixel-producing renderer body, capability negotiation, receipt comparison, and receipt replay verification. However, all interoperability exercises still launched each executable directly from tests/CI. There was no renderer-neutral executable that owned the sequence **discover capabilities -> reject incompatible request -> launch a separate renderer process -> replay-verify its evidence**.

## Improvement under test

`axm-render-external` links only `axm_render_contracts` and accepts an explicitly selected renderer executable.

The first executable exercise uses `contract_flat_renderer`, which is independent of `axm_render_native` and already emits `AXM_RENDER_CAPABILITIES 1`, pixels, and `AXM_RENDER_RECEIPT 1`.

Registered CTest coverage adds:

- `external-process-flat-dispatch`: capability discovery, compatible-request preflight, child-process render, and capability-bound receipt replay must all succeed;
- `external-process-rejects-backend-mismatch`: the same flat renderer presented with the native request must fail during capability preflight rather than rendering or silently substituting a backend.

The adapter also rejects collisions among request source, scene source, render output, receipt, and capability-manifest paths before launching the render phase.

## Evidence required before merge

The branch is mergeable only if GitHub Actions verifies:

- CMake configure succeeds;
- all targets compile with the repository warning policy;
- the complete CTest suite passes, including both new external-process cases;
- all previously established native/flat receipt, capability, comparison, and replay-verification checks remain green.

No PASS result is asserted in this file merely because the code or tests exist. The CI result for the final PR head is the executable evidence gate.

## Truth boundary

This is a **process-boundary adapter harness**, not a real third-party renderer integration. The exercised child renderer is built from this repository and shares the file-contract library, although the parent does not link the child renderer body.

The child process is not sandboxed or authenticated. Receipt/capability continuity digests are not cryptographic provenance. This change does not establish visual quality, native/flat visual equivalence, cross-machine determinism, GPU/WebGPU behavior, performance, memory use, or a frozen launcher protocol.

## Four-root gate

- **Truth:** compatibility is checked before dispatch and produced pixels/receipt are replay-verified after the child exits; unsupported claims remain excluded.
- **Agency / non-domination:** the parent depends on renderer-neutral contracts, not the AXM native body, and the renderer executable is explicit rather than silently selected.
- **Continuity:** request/scene/capability bytes are checked across the process boundary and the existing receipt verifier binds output back to declared state.
- **Wisdom before speed:** the process invocation convention remains experimental until real external integrations justify freezing it.
