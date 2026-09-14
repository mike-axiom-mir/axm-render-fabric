# PATH-Resolved Renderer Process Continuity Verification

Date observed: 2026-09-15 (Europe/Amsterdam)

PR lane: `growth/path-resolved-renderer-continuity`

Pre-evidence-doc verified head: `0ece6e6d4897276bb5c2bff794518b7c675b4153`

## Grounded gap

`axm-render-external` launches a bare renderer name with operating-system PATH lookup (`execvp` on POSIX and `_spawnvp` on Windows). Before this change, process-continuity evidence attempted `absolute(name)` in the current working directory even for a bare name.

That created two truth problems:

1. if a same-named regular file existed in the working directory, the harness could fingerprint that file while the actual child process was selected from `PATH`; and
2. if no same-named working-directory file existed, a renderer that was plainly available through `PATH` was reported as executable-file continuity `UNAVAILABLE`.

The change closes that selector/evidence mismatch without changing any frozen AXM scene, render-request, capability, or receipt v1 semantics.

## Implemented behavior

`axm-render-external` now distinguishes two resolvable process-selection modes:

- `EXPLICIT`: the caller supplied an absolute path or a path containing a parent component;
- `PATH`: the caller supplied a bare renderer name and the harness resolved a regular executable candidate from the current `PATH` before dispatch.

When either mode resolves successfully, the harness:

- records the selected invocation path;
- resolves its canonical target;
- records the existing AXM 64-bit continuity digest of those executable bytes;
- pins both capability-discovery and render child-process launches to that same invocation path rather than repeating a fresh PATH lookup;
- rechecks target and bytes across the existing external-dispatch checkpoints; and
- reports selection mode, invocation path, canonical path, digest, and `renderer_process_continuity=PASS`.

If a bare name cannot be resolved by the harness, the existing fallback remains explicit: dispatch may still be attempted by the operating system and process continuity is reported as `UNAVAILABLE`, never invented.

## Executable regression

The existing `external-process-preserves-renderer-process-identity` CTest now exercises both explicit and PATH-based selection.

For the PATH case on the tested Unix CI runner, the regression:

1. copies the real flat renderer into a dedicated directory placed first on `PATH`;
2. creates a same-named, non-executable regular decoy file in the process working directory;
3. calls `axm-render-external` with only the bare renderer name;
4. requires `renderer_process_resolution=PATH`;
5. requires the reported invocation and canonical paths to identify the executable in the PATH directory rather than the cwd decoy;
6. requires executable continuity and normal verified external dispatch to pass.

This is specifically designed to fail the pre-change evidence behavior, where the cwd file could be inspected even though `execvp` would select the PATH executable.

## Observed PR-head evidence

Both repository workflows passed on pre-evidence-doc head `0ece6e6d4897276bb5c2bff794518b7c675b4153`.

Standard CI run `34910349955`:

- Ubuntu 24.04.5, GCC 13.3.0;
- configure: PASS;
- build: PASS;
- CTest: **39/39 PASS**;
- `external-process-preserves-renderer-process-identity`: PASS, including the explicit-path checks plus PATH/cwd-decoy assertions in that test script;
- explicit flat adapter dispatch reported `renderer_process_resolution=EXPLICIT`, process continuity PASS, and adapter digest `0x2787338c691d82c1`;
- flat 320x180 frame continuity remained `0x5338e2b2729f1381`;
- native frame continuity remained `0x456f404dd94c91da`, with PPM SHA-256 `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`;
- ImageMagick adapter process continuity remained PASS, adapter digest `0x46cb365355c4b6fb`;
- delegated ImageMagick executable remained `/usr/bin/convert-im6.q16`, digest `0x08642e03d077e786`;
- ImageMagick frame continuity remained `0x8d283e4b5e79fd9f`;
- ImageMagick/native comparison remained `comparable_v1=YES` while pixel/output digests remained different.

Dedicated Ghostscript workflow run `34910350058`:

- configure/build: PASS;
- CTest: **39/39 PASS**;
- Ghostscript adapter process continuity remained PASS, adapter digest `0xa6f35d1d8b4da45b`;
- delegated Ghostscript executable remained `/usr/bin/gs`, digest `0x5a6de77d03e12c56`;
- Ghostscript frame continuity remained `0x34deb087b282e37e`;
- Ghostscript PPM SHA-256 remained `b93468272d7d0a2bf58c979115584f89745574354a87a73285d8cde1d073ce2b`;
- native/Ghostscript comparison remained `comparable_v1=YES` while pixel/output digests remained different.

No renderer algorithm, scene meaning, request meaning, capability meaning, receipt meaning, or expected frame bytes were intentionally changed by this lane.

The evidence-document commit changes the PR head, so merge still requires both workflows to pass again on the final head.

## Four-root merge gate

### Truth

The process fingerprint now corresponds to the executable path the harness actually pins and launches when a bare name is resolvable from PATH. A cwd decoy is not promoted to evidence for a different launched executable. Unresolved selection is still reported honestly as unavailable.

### Agency / non-domination

The caller still chooses the renderer explicitly, either by path or bare name. This adds no automatic renderer ranking, fallback, substitution, or hidden selection policy.

### Continuity

The frozen v1 scene/request/capability/receipt contracts remain unchanged. The improvement strengthens continuity around the experimental process launcher while preserving the AXM-owned native renderer and current external adapters.

### Wisdom before speed

This closes one demonstrated selector/evidence mismatch before freezing the launcher convention or adding larger machine-wide renderer discovery. It is deliberately smaller than a new process protocol.

## Truth boundary

This remains **non-cryptographic continuity evidence**, not authentication or signed provenance.

It does not prove:

- package identity or software publisher identity;
- cryptographic provenance of the selected executable;
- identity of shared libraries, delegates, configuration, drivers, or environment loaded by the process;
- an atomic binding between the final byte check and OS process creation;
- immunity to every same-permission filesystem or environment race;
- sandboxing or privilege isolation;
- visual quality or native/external pixel equivalence;
- performance or memory behavior; or
- cross-machine determinism.

PATH resolution reflects the current process environment. On the tested POSIX path, candidates must be regular executable files. Windows source support also considers PATHEXT candidates, but this lane's decoy regression was executed on Ubuntu CI rather than a Windows runner.
