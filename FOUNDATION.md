# Foundation

AXM Render Fabric exists because rendering is a separable software capability, not merely an implementation detail hidden inside a game, editor, or design application.

## Root constraints

The project is governed internally by four roots:

1. **Truth** — claims must match evidence. A rendered frame is evidence only for what was actually rendered. Unsupported quality claims are forbidden.
2. **Agency / non-domination** — the renderer must not become a lock-in mechanism. Stable contracts and adapters should preserve user choice.
3. **Continuity** — scene/state contracts should survive renderer replacement where practical. Products must not require one temporary body to preserve their meaning.
4. **Wisdom before speed** — capability growth must not silently outrun evidence, reproducibility, or understandable interfaces.

These roots are the constitutional merge gate. Git permission, a maintainer, a specialist, or a founder is not by itself canonical authority.

## Architectural principle

> Own one substrate. Connect every useful substrate. Do not require either to exclude the other.

AXM should be able to render through its own implementation while still connecting other engines and renderers when they are better for a task.

## Non-goals

This repository is not trying to become an entire game engine, editor, asset generator, or AI system. Those can sit around the fabric.
