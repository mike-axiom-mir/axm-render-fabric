# Renderer Process Continuity Guard Verification

Date observed: 2026-09-15 (Europe/Amsterdam)

PR lane: `automation/renderer-process-continuity-guard`

Pre-evidence-doc verified head: `08f4c7a239bc52a86327efcc8dcc6536460bdb33`

## Grounded gap

`axm-render-external` already continuity-checked the render request, scene source, freshly generated capability manifest, produced output, and receipt around its external child-process flow. It did not independently check the selected adapter executable file itself across capability discovery and rendering.

That left two concrete boundaries:

1. a directly supplied adapter path could resolve to different bytes or a different canonical target between phases without the parent harness noticing independently; and
2. because the harness clears caller-declared output/capability/receipt paths before their producing phases, a mutable artifact path equal to the selected adapter executable could remove the executable itself.

The change closes those gaps for directly resolvable renderer executable paths without changing any frozen v1 scene, request, capability, or receipt semantics.

## Implemented behavior

When `--renderer` resolves directly to a regular filesystem file, `axm-render-external` now:

- resolves and records the adapter's canonical target;
- records the existing AXM 64-bit continuity digest of the adapter file bytes;
- rejects render-output, receipt, or generated-capability paths that collide with the supplied executable path or canonical target before any clearing occurs;
- re-resolves the executable target and rechecks its bytes after capability discovery, before rendering, after rendering, and after receipt replay verification;
- reports the canonical adapter path, adapter continuity digest, and `renderer_process_continuity=PASS` on a successful verified dispatch.

A bare executable name that is intended for PATH lookup remains usable. If that bare name is not directly resolvable as a filesystem path by the harness, executable-file continuity evidence is explicitly reported as unavailable instead of inferred.

## Executable regression

A new CTest, `external-process-preserves-renderer-process-identity`, copies the flat renderer to a temporary executable and first presents a render request whose output path is that executable itself.

Expected result:

- dispatch is rejected before artifact clearing;
- the renderer executable still exists;
- the error identifies the render-output / renderer-executable collision.

The same test then submits a safe output path and requires:

- a fresh output, receipt, and capability manifest;
- `renderer_process_continuity=PASS`;
- an emitted renderer process digest; and
- normal verified external dispatch completion.

## Observed PR-head evidence

Both repository workflows passed on pre-evidence-doc head `08f4c7a239bc52a86327efcc8dcc6536460bdb33`.

Standard CI run `34906132227`:

- Ubuntu 24.04.5, GCC 13.3.0;
- configure: PASS;
- build: PASS;
- CTest: **39/39 PASS**;
- new renderer self-clobber / process-continuity regression: PASS;
- flat adapter process continuity: PASS, observed adapter digest `0x2787338c691d82c1`;
- existing flat 320x180 frame continuity remained `0x5338e2b2729f1381`;
- existing native frame continuity remained `0x456f404dd94c91da`;
- ImageMagick adapter process continuity: PASS, observed adapter digest `0x46cb365355c4b6fb`;
- delegated ImageMagick executable continuity remained `/usr/bin/convert-im6.q16`, digest `0x08642e03d077e786`;
- ImageMagick frame continuity remained `0x8d283e4b5e79fd9f`;
- ImageMagick/native receipt comparison remained `comparable_v1=YES` while pixel/output digests remained different.

Dedicated Ghostscript workflow run `34906132243`:

- configure/build: PASS;
- CTest: **39/39 PASS**;
- Ghostscript adapter process continuity: PASS, observed adapter digest `0xa6f35d1d8b4da45b`;
- delegated Ghostscript executable remained `/usr/bin/gs`, digest `0x5a6de77d03e12c56`;
- Ghostscript frame continuity remained `0x34deb087b282e37e`;
- Ghostscript PPM SHA-256 remained `b93468272d7d0a2bf58c979115584f89745574354a87a73285d8cde1d073ce2b`;
- native/Ghostscript receipt comparison remained `comparable_v1=YES` while pixel/output digests remained different.

The evidence-document commit changes the PR head, so merge still requires both workflows to pass again on the final head.

## Four-root merge gate

### Truth

Executable continuity is claimed only when the supplied renderer argument can actually be resolved to a regular file and checked. PATH-only lookup that is not directly resolvable is reported as `UNAVAILABLE`, not promoted to a false proof.

### Agency / non-domination

The caller's explicit renderer selection is preserved and protected from destructive artifact-path collisions. No automatic renderer choice, ranking, fallback, or silent substitution is introduced.

### Continuity

The existing frozen v1 contracts and renderer-neutral receipt verification path are unchanged. This strengthens continuity around the experimental launcher boundary rather than redefining render meaning.

### Wisdom before speed

The change is deliberately narrow: protect and continuity-check the adapter executable already used by the two-phase launcher before freezing that launcher convention or adding larger discovery/orchestration policy.

## Truth boundary

This is **non-cryptographic continuity evidence**, not authentication.

Specifically, it does not prove:

- cryptographic or signed executable provenance;
- that the launched process is trustworthy;
- an atomic binding between the final file check and the operating system's subsequent `exec`/spawn operation;
- immunity to all replace-after-check or same-permission filesystem races;
- the identity of shared libraries, delegates, configuration, environment, drivers, or other dependencies loaded by the adapter;
- PATH-search attestation when the supplied renderer name is not directly resolvable by this harness;
- sandboxing or privilege isolation;
- native/external pixel or visual equivalence;
- visual quality;
- performance or memory behavior; or
- cross-machine determinism.

The 64-bit AXM continuity digest is used as an observed byte-change detector consistent with the repository's existing v1 evidence model. It is not a security hash.
