# Chameleon Painter

Runtime plugin for the Meccha Chameleon-style hider prototype.

## Classes

- `AChameleonHiderBodyActor`: drop-in actor with a procedural metaball body and paint component.
- `UChameleonMetaballBodyComponent`: generates the smooth paintable body mesh at runtime.
- `UChameleonPaintComponent`: applies the current paint color to the body and material parameters.
- `UChameleonColorPickerWidget`: pure C++ UMG widget with swatches, RGB sliders, and an Apply button.

## Quick Start

1. Place `AChameleonHiderBodyActor` in a level, or add `UChameleonMetaballBodyComponent` and `UChameleonPaintComponent` to an actor.
2. Assign a body material that supports at least one vector parameter named `PaintColor`, `BodyColor`, `BaseColor`, or `Color`.
3. Create `UChameleonColorPickerWidget` with `Create Widget`.
4. Call `SetTargetPaintComponent` on the widget and pass the actor's `UChameleonPaintComponent`.

`UChameleonMetaballBodyComponent` also writes vertex colors for `ClearPaint`, `SetCamouflageBaseColor`, and paint strokes.
For visible per-vertex stroke painting, use a material that reads vertex color.

## Paint API

- `SetCamouflageBaseColor`: sets the whole body color.
- `ApplyPaintStrokeLocal`: applies a radial stroke in body-local centimeters.
- `ApplyPaintStrokeWorld`: applies a radial stroke from world position/normal.
- `ApplyPaintStrokeFromHit`: applies a stroke from a hit result on the generated body mesh.
- `ClearPaint`: removes all strokes and restores the base color.

By default, the cosmetic body mesh has collision disabled. Enable `bBuildQueryCollision` only when you need mesh hit tests for brush strokes.
