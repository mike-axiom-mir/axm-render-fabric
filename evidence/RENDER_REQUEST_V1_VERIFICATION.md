# Render request v1 verification

Date: 2026-09-14

## Focused local build/run evidence

The new request parser, native CLI integration, and request smoke test were compiled directly with C++20 plus `-Wall -Wextra -Wpedantic` against the current native renderer and scene loader.

Observed reference request results:

- `render_request_contract_version=1`
- `render_request_backend=axm.native.cpu.reference`
- backend check: `PASS`
- dimensions check (`320x180`): `PASS`
- format check (`ppm-rgb8`): `PASS`
- loaded scene triangle count check: `PASS`
- frame FNV hash: `0x456f404dd94c91da`
- expected existing reference FNV hash: `0x456f404dd94c91da`
- reference hash match: `PASS`
- native `--self-test` through `--request`: `deterministic_same_run=PASS`
- generated PPM SHA-256: `c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb`

Unsupported-backend fixture:

- declared backend: `external.mock.renderer`
- native executable exit code: `1`
- observed error states that the backend is unsupported and names the one supported native backend
- no silent fallback was performed

## Truth boundary

These observations are focused local execution evidence, not cross-machine determinism evidence. The unchanged hashes support continuity for the tested reference request and build only. No new visual-quality judgment was made. No external renderer was exercised. CI status is recorded separately by the pull request before merge.
