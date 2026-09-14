# Local verification — initial v0.1 foundation

This records one container-local verification run performed before opening the foundation PR. It is evidence for this run only and is not a claim of cross-platform determinism.

Environment observed:

- `g++ (Debian 14.2.0-19) 14.2.0`
- C++20 build, `-Wall -Wextra -Wpedantic -O2`

Commands:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -O2 src/main.cpp -o axm-render
./axm-render --self-test
./axm-render --out evidence/frame.ppm
sha256sum evidence/frame.ppm
```

Observed output:

```text
frame_hash_a=0x456f404dd94c91da
frame_hash_b=0x456f404dd94c91da
deterministic_same_run=PASS
wrote=evidence/frame.ppm
frame_hash=0x456f404dd94c91da
c81172b1e42106b4a1032e4c6873974202e8a57efb76d7c7cb881ea568aa6beb  evidence/frame.ppm
```

The generated frame was also opened and visually inspected: it contains two overlapping rasterized triangles on the configured dark background. The binary frame itself is not committed here; CI regenerates a frame and records its digest.
