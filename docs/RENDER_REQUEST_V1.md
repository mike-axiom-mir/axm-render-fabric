# AXM_RENDER_REQUEST v1 — minimal renderer-neutral request envelope

Status: **frozen minimal v1 subset**.

`AXM_RENDER_REQUEST 1` separates render intent from scene state. It gives native and future renderer bodies one small request envelope for choosing a scene packet, backend identifier, output dimensions, output encoding, and destination without embedding those concerns into `AXM_SCENE 1`.

## Compatibility rule

Readers must reject unsupported versions rather than guessing. Incompatible changes require a new version number; v1 files keep the syntax and semantics below.

This is an on-disk contract. It is not a promise of stable C++ ABI/API compatibility.

## Required directives

The first non-empty, non-comment line is:

```text
AXM_RENDER_REQUEST 1
```

The following directives are each required exactly once:

```text
scene PATH
backend IDENTIFIER
width N
height N
format ppm-rgb8
output PATH
```

Rules:

- `#` begins a comment to end of line.
- Unknown and duplicate directives are rejected.
- Trailing tokens are rejected.
- `width` and `height` are integers in `1..8192`.
- v1 currently freezes one output format identifier: `ppm-rgb8`.
- Scene and output paths are single tokens. Relative paths are resolved against the request file's directory.
- The generic parser accepts the backend identifier as declared state. A renderer body must explicitly reject identifiers it does not implement rather than silently falling back.

The current AXM native executable implements backend identifier `axm.native.cpu.reference`. A request naming another backend is parsed but rejected at execution by that body.

## Example

```text
AXM_RENDER_REQUEST 1
scene reference.axmscene
backend axm.native.cpu.reference
width 320
height 180
format ppm-rgb8
output request-frame.ppm
```

See `examples/reference.axmrender`.

## Truth boundary

The current evidence proves that the AXM native body can consume this request envelope, load the existing `AXM_SCENE 1` fixture, preserve the known 320x180 native reference hash in the tested build, and reject a request selecting an unsupported backend.

It does **not** prove that any external renderer implements this contract, that two backends produce equivalent pixels, that `ppm-rgb8` is sufficient for production output, that paths are portable across all systems, or that the C++ library ABI/API is stable. Feature negotiation, fallbacks, receipts, and broader formats/settings remain later gates.
