# Ghostscript second external-renderer verification

Date: 2026-09-14

## Grounded gap

The architecture called for a second materially different real external renderer before treating the process-launch convention as something worth freezing. Before this change, ImageMagick was the only external rasterizer actually exercised behind the AXM scene/request/capability/receipt boundary.

## What changed

`ghostscript_ps_renderer` is a renderer adapter that links only `axm_render_contracts`. It accepts backend `external.ghostscript.postscript-raster`, translates the frozen `AXM_SCENE 1` ordered-triangle / flat-RGB-albedo subset to PostScript, invokes an external Ghostscript process using its `ppmraw` device, reads the resulting P6 RGB8 pixels, and emits the existing `AXM_RENDER_RECEIPT 1` evidence shape.

The adapter publishes `AXM_RENDER_CAPABILITIES 1` before dispatch and rejects backend mismatch instead of silently substituting another renderer. It is exercised through `axm-render-external`, so capability preflight, fresh-artifact enforcement, request/scene continuity checks, and capability-bound receipt replay apply unchanged.

As with the ImageMagick adapter, the exact observed delegated Ghostscript executable bytes contribute to the adapter renderer-version token using the existing non-cryptographic continuity digest:

```text
0.1.0+delegated-fnv64-<16-hex-digit executable digest>
```

No frozen v1 scene, request, capability, or receipt field meaning changed.

## CI evidence actually observed

Successful implementation-head GitHub Actions run: `34883275573`, head `15a606693202f1facac01abe9d294d555476b584`.

Observed environment:

- Ubuntu 24.04.5 LTS;
- `ubuntu-24.04` runner image `20260907.300.1`;
- GNU C++ 13.3.0;
- Ghostscript `10.02.1`;
- ImageMagick `6.9.12-98 Q16` remained present so the existing external-render path was exercised in the same full suite.

Build completed successfully, including the new `ghostscript_ps_renderer` target.

CTest reported:

```text
100% tests passed, 0 tests failed out of 38
```

The three Ghostscript-specific CTests passed:

```text
external-process-ghostscript-dispatch
external-process-ghostscript-compares-native-intent
external-process-ghostscript-rejects-native-backend
```

## Fresh Ghostscript render evidence

The adapter resolved the delegated executable to:

```text
/usr/bin/gs
```

Observed executable continuity digest:

```text
0x5a6de77d03e12c56
```

Observed bound adapter renderer version in capability and receipt evidence:

```text
0.1.0+delegated-fnv64-5a6de77d03e12c56
```

Observed Ghostscript-backed 320x180 reference render:

```text
frame_pixels_digest64 0x34deb087b282e37e
output_file_digest64 0x5cf691c2f64e9b72
SHA-256 b93468272d7d0a2bf58c979115584f89745574354a87a73285d8cde1d073ce2b
```

The same scene-source bytes were rendered through the AXM native reference body in the same run; its established frame continuity digest remained:

```text
0x456f404dd94c91da
```

The renderer-neutral comparison reported:

```text
same_scene_contract=YES
same_render_request_contract=YES
same_scene_source=YES
same_dimensions=YES
same_output_format=YES
comparable_v1=YES
same_frame_pixels_digest64=NO
same_output_file_digest64=NO
```

That is the intended evidence boundary: both bodies consumed comparable frozen-v1 declared frame intent, while their actual pixel/output digests remained different and were not promoted to equivalence.

The CI lane independently extracted the delegated executable digest and required the corresponding renderer-version token to appear in the fresh capability manifest, the capability manifest used by `axm-render-external`, and the resulting receipt.

## Four-root merge gate

**Truth:** a real Ghostscript executable was launched and its produced P6 pixels were replay-verified through the existing receipt path. The evidence records the actual observed version, executable path/digest, frame/output digests, and the fact that native and Ghostscript pixels differ.

**Agency / non-domination:** the AXM native renderer remains the owned substrate. Ghostscript is an explicitly selected optional backend behind the same renderer-neutral boundary; unsupported backend requests are rejected rather than silently redirected.

**Continuity:** `AXM_SCENE 1`, `AXM_RENDER_REQUEST 1`, `AXM_RENDER_CAPABILITIES 1`, and `AXM_RENDER_RECEIPT 1` remain unchanged. The second real external renderer reuses the existing capability, dispatch, receipt, comparison, and verification machinery instead of creating a renderer-specific island.

**Wisdom before speed:** only the already-frozen triangle/albedo subset is translated. The result proves a second external process boundary before any launcher protocol is frozen or richer renderer semantics are invented.

## Explicit truth boundary

This is a second real external-renderer interoperability exercise, not production readiness or a claim that the launcher convention is final.

The PostScript adapter covers only the current `AXM_SCENE 1` ordered-triangle / flat-albedo subset. It does not implement or claim AXM native depth testing, lighting, textures, materials, animation, cameras, PBR, or feature equivalence.

The Ghostscript executable identity is a 64-bit non-cryptographic continuity digest. It is not authentication, code signing, collision-resistant provenance, or tamper-proof attestation. It binds the observed executable file bytes, not shared libraries, fonts, Ghostscript resource/configuration files, environment, kernel/runtime state, or other dependency closure.

`-dSAFER` is passed to the delegated process, but this evidence does not establish a complete sandbox or security boundary.

Ghostscript `10.02.1` is CI-log evidence correlated with the run; the semantic version string is not a frozen receipt field.

Nothing here establishes visual quality, visual or semantic equivalence with AXM native or ImageMagick rendering, cross-machine bitwise determinism, cross-version pixel stability, performance, memory use, GPU behavior, cryptographic provenance, or production suitability.
