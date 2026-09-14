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

This invocation convention is deliberately **not frozen as an AXM contract version yet**. It is executable research for the process boundary. A future third-party adapter may justify a versioned launcher protocol once the required fields and failure semantics are supported by real integrations rather than guessed in advance.

## Truth boundary

This harness demonstrates process isolation plus contract/evidence checking when exercised successfully. Requiring newly created artifact files strengthens the statement that the accepted files came from the current dispatch attempt rather than being silently reused from an earlier run. It still does not prove that the child executable is trustworthy, sandboxed, third-party, or cryptographically authenticated. An explicitly selected child process has the same operating-system permissions as the invoking user unless the surrounding runtime constrains it.

`AXM_RENDER_RECEIPT 1` and the current continuity digests remain non-cryptographic evidence. Fresh-path enforcement is not process attestation: a child may still invoke other programs, race with another process that has the same filesystem permissions, or deliberately write misleading bytes that only later contract verification can reject. Successful replay does not establish visual quality, semantic equivalence between renderer bodies, cross-machine determinism, GPU behavior, production performance, or provenance of the executable itself.

The first repository exercise uses the independent `contract_flat_renderer` body because it already publishes capabilities and emits receipts without linking the native renderer. That is a process-boundary interoperability test, not a claim that a real external vendor renderer has been integrated.
