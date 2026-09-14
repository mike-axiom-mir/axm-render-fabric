# Renderer capability negotiation verification

## Verified change

This change adds a renderer-neutral `AXM_RENDER_CAPABILITIES 1` contract plus `axm-render-negotiate`.

The current native renderer and the independent flat contract renderer can each publish a capability manifest naming only the v1 fields they can honestly declare today:

- renderer identity and version;
- backend identifier;
- supported `AXM_SCENE` contract version;
- supported `AXM_RENDER_REQUEST` contract version;
- maximum request dimensions;
- supported output format identifiers.

`axm-render-negotiate` loads the request, parses the referenced `AXM_SCENE 1` source, loads one capability manifest, and rejects incompatible backend/dimension/format/contract combinations instead of selecting a different backend silently.

## CI evidence

GitHub Actions run `34834980437`, job `103946642016`, on Ubuntu 24.04 / GCC 13.3.0 built the PR merge ref successfully.

CTest reported:

```text
100% tests passed, 0 tests failed out of 22
```

The five capability-specific tests passed:

```text
capabilities-native-manifest
capabilities-flat-manifest
capabilities-negotiate-native-request
capabilities-negotiate-flat-request
capabilities-reject-backend-mismatch
```

The CI evidence step emitted these native declarations:

```text
AXM_RENDER_CAPABILITIES 1
renderer axm.native.cpu.reference
renderer_version 0.1.0
backend axm.native.cpu.reference
scene_contract 1
render_request_contract 1
max_width 8192
max_height 8192
format ppm-rgb8
```

and these independent flat-body declarations:

```text
AXM_RENDER_CAPABILITIES 1
renderer axm.contract.flat-demo
renderer_version 0.1.0
backend axm.contract.cpu.flat
scene_contract 1
render_request_contract 1
max_width 8192
max_height 8192
format ppm-rgb8
```

Negotiating the 320x180 native reference request against the native manifest reported `compatible=YES`. Negotiating the same scene/dimensions/format with the flat request against the flat manifest also reported `compatible=YES`. The dedicated negative CTest verifies that a native-backend request is rejected against the flat manifest rather than silently substituted.

Existing rendering evidence also remained continuous in the same CI run. The native reference frame still reported pixel digest `0x456f404dd94c91da` and PPM SHA-256 `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`. The same-intent flat body remained intentionally different and the existing receipt comparator continued to report `comparable_v1=YES` with unequal pixel/output digests.

## Truth boundary

This verifies explicit discovery/negotiation only for the frozen v1 request/scene subset. It does **not** establish:

- feature-level negotiation for depth, lighting, materials, textures, animation, or other future scene semantics;
- automatic fallback or backend ranking;
- a third-party renderer adapter;
- GPU or WebGPU support;
- binary ABI stability;
- cross-backend pixel or visual equivalence;
- cross-machine determinism;
- cryptographic provenance;
- performance, memory, or visual-quality superiority.

A capability manifest is a renderer declaration plus parser-enforced structure. It is not independent proof that every future implementation behaves correctly; executable tests and render receipts remain separate evidence gates.
