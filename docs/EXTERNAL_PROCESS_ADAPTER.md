# External renderer process adapter harness

`axm-render-external` is a renderer-neutral process-boundary harness for testing renderer adapters without linking their renderer body into AXM.

It preserves the current architecture: AXM still owns its native substrate, while another renderer body may live behind the shared `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` files.

## Current executable flow

The harness receives an explicit renderer executable or bare executable name plus request, receipt, and capability-manifest paths. It may also receive `--expect-capabilities` to bind a previously inspected manifest to the fresh manifest produced at dispatch time.

It then:

1. continuity-checks and parses the request and referenced scene;
2. resolves the renderer process selector when possible, pins one invocation path, and records the canonical executable target plus its current non-cryptographic continuity digest;
3. rejects source/evidence/output path collisions that could overwrite declared state or the selected renderer executable;
4. clears any pre-existing file at the declared capability-manifest path, launches the renderer for capability discovery, and requires that invocation to create a regular capability file;
5. parses the newly emitted capability manifest and rejects an incompatible request before rendering;
6. when `--expect-capabilities` is supplied, requires the fresh renderer/version/backend/contract/dimension/format declaration to match that previously inspected manifest;
7. clears any pre-existing files at the declared render-output and receipt paths, then launches the same pinned renderer path as a separate process for the render;
8. requires that render invocation to create both output and receipt as regular files;
9. checks that request, scene, capability, optional expected-capability, and selected renderer executable evidence did not visibly change across dispatch; and
10. replay-verifies the produced receipt, output bytes, pixel digest, and renderer identity/version/backend against the capability manifest created for the same dispatch.

The harness links only `axm_render_contracts`. It does not link `axm_render_native` or the dispatched renderer body.

The capability, output, and receipt paths are treated as declared outputs owned by the current invocation. Existing non-directory entries at those exact paths are removed before the relevant child phase. Existing directories are rejected instead of recursively removed. Produced symlinks and other non-regular artifacts are rejected. This prevents a renderer that exits successfully without writing from accidentally inheriting stale evidence from an earlier run.

## Renderer process selection evidence

Explicit renderer paths and bare names are handled separately.

- An explicit path is normalized and used as the invocation path.
- A bare renderer name is resolved against the current `PATH` before capability discovery when the harness can identify a regular executable candidate.
- Once resolved, both capability discovery and rendering use that pinned invocation path instead of performing two independent PATH lookups.
- The canonical target and current executable bytes are continuity-checked around the child-process phases.

Successful dispatch reports `renderer_process_resolution=EXPLICIT` or `PATH`, the pinned invocation path, canonical target, and a 64-bit continuity digest. If a selector cannot be resolved by the harness, existing launch behavior remains available but process continuity is reported as unavailable rather than invented.

This is continuity evidence, not process authentication or signed software provenance. The existing digest is intentionally non-cryptographic, and the checks do not attest dynamically loaded libraries, drivers, environment variables, kernel state, package identity, or every possible filesystem/exec race.

## Parent-observed dispatch timing

A successful dispatch now emits bounded timing evidence using `std::chrono::steady_clock` in the parent harness:

- `capability_process_elapsed_us` — elapsed parent wall-clock time spent launching and waiting for the capability-discovery child process;
- `render_process_elapsed_us` — elapsed parent wall-clock time spent launching and waiting for the render child process;
- `receipt_verification_elapsed_us` — elapsed parent wall-clock time spent replay-verifying the receipt/capability/output evidence and reloading the receipt;
- `external_dispatch_elapsed_us` — elapsed parent wall-clock time from immediately after argument parsing through successful final verification and process-continuity recheck.

These values are observations for one dispatch, not receipt-v1 fields and not a benchmark claim. Child process CPU time, GPU time, allocator use, I/O decomposition, scheduling noise, startup cache state, and machine load are not separated. Comparing two numbers from different runners, builds, or machine states is therefore not evidence that one renderer is intrinsically faster.

## Experimental process convention

The current harness invokes an adapter executable as:

```text
RENDERER --capabilities CAPS.axmcaps
RENDERER REQUEST.axmrender RECEIPT.axmreceipt
```

This invocation convention is deliberately **not frozen as an AXM contract version yet**. It remains executable research for the process boundary. Two materially different real external rasterizers now use it successfully, which is stronger evidence than one integration, but still not enough to assume every future renderer should share the same lifecycle or invocation shape.

## Current exercises

Four renderer bodies exercise the shared boundaries at different levels:

1. `axm-render` / `axm_render_native` is the AXM-owned native CPU reference substrate.
2. `contract_flat_renderer` is a repository-owned independent renderer body. It proves that another body can publish capabilities, write pixels/receipts, and be verified through a separate process without linking the AXM native renderer.
3. `imagemagick_svg_renderer` is an AXM adapter that delegates rasterization to external ImageMagick `convert`. It translates the frozen `AXM_SCENE 1` triangle/albedo subset to SVG, requests raw RGB8 pixels, wraps those pixels in the requested PPM envelope, and emits the shared receipt.
4. `ghostscript_ps_renderer` is a materially different adapter that translates the same frozen subset to PostScript and delegates rasterization to external Ghostscript `gs` using its `ppmraw` device.

Both real external adapters are exercised through `axm-render-external`, not as bypasses around the harness. Each publishes its own backend identity and rejects mismatched backend requests rather than silently redirecting them. Their adapter versions include the observed delegated executable-byte continuity digest, so capability and receipt identity can be checked against that observed delegated executable file without changing frozen receipt-v1 fields.

## Truth boundary

The harness demonstrates process separation plus contract/evidence checking when exercised successfully. Requiring newly created artifact files strengthens the statement that accepted files were produced during the current dispatch attempt rather than silently reused from an earlier run. Pinning a resolvable renderer path and checking its canonical target/bytes strengthens process-selection continuity. Neither mechanism proves that the child executable is trustworthy, sandboxed, cryptographically authenticated, or unable to invoke other processes.

The ImageMagick and Ghostscript exercises establish that two real external rasterizers can sit behind the current AXM contract boundary for the frozen triangle/albedo subset in tested CI environments. They do **not** establish production-grade third-party renderer integration or broad scene-feature portability. Neither adapter preserves the native renderer's complete depth/lighting semantics, and no equivalent-pixel or equivalent-visual-semantics claim is made.

`AXM_RENDER_RECEIPT 1` and the current continuity digests remain non-cryptographic evidence. The composite renderer-version tokens bind observed delegated executable file bytes only at this bounded continuity level; they do not bind semantic-version text as a separate contract field, loaded shared libraries/delegates, fonts, policy/configuration, environment, package provenance, or cross-machine behavior.

The parent-observed timing fields are likewise not part of receipt v1, are not authenticated, and are not deterministic. They provide a concrete measurement surface for later controlled performance work; they do not by themselves establish renderer performance rankings.

Successful replay does not establish visual quality, semantic equivalence between renderer bodies, cross-machine or cross-version determinism, GPU behavior, production performance, sandboxing, or cryptographic provenance.

See the `evidence/` directory for the bounded CI observations behind each interoperability step.
