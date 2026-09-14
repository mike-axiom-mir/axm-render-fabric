# AXM_SCENE v1 — minimal scene-state interchange

Status: **frozen minimal v1 subset**. This is intentionally much smaller than the future canonical visual-state model.

The purpose of v1 is to establish one tiny renderer-neutral scene-state packet that can be loaded from disk and handed to the AXM native renderer today, while remaining simple enough for future external/backend adapters to implement without depending on the native rasterizer.

## Compatibility rule

`AXM_SCENE 1` identifies this exact version. Readers must reject unsupported versions rather than silently guessing. Incompatible changes require a new version number; v1 files keep their documented meaning.

This compatibility promise applies to the **on-disk v1 syntax and semantics below**, not to the C++ ABI/API of `axm_render_native`.

## Grammar

The first non-empty, non-comment line is:

```text
AXM_SCENE 1
```

Each following non-empty line is currently one triangle:

```text
triangle x0 y0 z0 x1 y1 z1 x2 y2 z2 r g b
```

Rules:

- `x/y/z` values are finite floating-point numbers.
- `r/g/b` are integer byte values in `0..255`.
- `#` begins a comment to end of line.
- Unknown directives are rejected.
- Trailing tokens are rejected.
- The repository loader currently bounds a file to at most 1,000,000 triangles as an implementation resource guard.

## v1 semantics

- A scene is an ordered list of independent triangles.
- Each triangle has three positions and one flat RGB-byte albedo.
- Position values are passed unchanged to a backend. The current AXM CPU reference renderer interprets x/y in its existing normalized screen mapping and uses smaller z values as nearer depth values.
- v1 does not define transforms, cameras, lights, textures, material models, animation, object identity, provenance, units, color space, or environment state.
- Render resolution is deliberately **not** scene state and is not encoded here; it remains a render-request concern.

## Example

```text
AXM_SCENE 1
triangle -0.82 -0.62 0.45  0.72 -0.55 0.35  -0.05 0.78 0.40  226 68 92
```

See `examples/reference.axmscene` for the executable reference fixture.

## Truth boundary

The repository currently proves that its v1 loader parses the reference file and that the AXM CPU reference renderer produces the same known reference-frame hash from that loaded state as from the prior built-in demo scene in the tested CI/build environment.

That does **not** yet prove external-renderer pixel equivalence, a renderer-neutral render-request contract, cross-machine bitwise determinism, a complete canonical scene model, or long-term stability of the C++ library ABI/API. Those remain separate gates.
