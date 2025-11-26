# Paths

Headers to include: `#include <visage_graphics/path.h>` and `#include <visage_graphics/canvas.h>`.

Related guides: [Graphics](graphics), [UI & Layout](ui-layout), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## What is a Path?

`visage::Path` builds arbitrary vector shapes from move/line/curve commands. Paths are filled or stroked via the Canvas and participate in batching and dirty-region redraw like other shapes.

## Building paths

Common methods:
- `moveTo(Point)`, `lineTo(Point)`
- `quadraticTo(ctrl, to)`, `cubicTo(ctrl1, ctrl2, to)`
- `closePath()`
- `arc(center, radius, startRadians, radians)`
- `rectangle(x, y, w, h)`, `roundedRectangle(...)`
- `squircle(...)`, `ellipse(...)`

Bounding box: `path.boundingBox()` gives `{left, top, right, bottom}` in path-local space.

## Drawing paths

Fill:
```cpp
visage::Path p;
p.moveTo({0, 0});
p.lineTo({100, 0});
p.lineTo({100, 100});
p.closePath();
canvas.setColor(0xff66ccff);
canvas.fill(p, 10, 10, 200, 200); // scales to width/height
```

Stroke:
```cpp
canvas.setColor(0xffffffff);
canvas.stroke(p, 10, 10, 200, 200, 3.0f,
              visage::Path::Join::Round,
              visage::Path::EndCap::Round);
```

Fill-in-place (no scaling, uses path’s own bounds):
```cpp
canvas.fill(p, 10, 10);
```

## Scaling and positioning

- `fill(path, x, y, width, height)`: scales the path to the target width/height at position.
- `fill(path, x, y)`: uses path’s intrinsic bounds at position.
- Same signatures for `stroke`.
- All coordinates are in logical pixels unless you switch to native scale on the Canvas.

## Joins and caps (stroke)

`Path::Join`: `Miter`, `Round`, `Bevel`  
`Path::EndCap`: `Round`, `Square`, `Butt`  
Optional dash array and offset for dashed strokes.

## Anti-aliasing and quality

Paths are anti-aliased by default via the renderer. Keep stroke widths sensible; extremely thin strokes may be limited by resolution.

## Performance tips

- Reuse `Path` objects; clear and rebuild instead of reallocating.
- Avoid excessive point counts when a curve suffices.
- Keep redraw regions tight—only `redraw()` when path data changes.
- Batch-friendly: paths are batched with other shapes per layer; reduce state changes (blend mode/brush) to maximize batching.

## Examples to study

- `examples/Paths/paths.cpp`: various path constructions and fills/strokes.
- `examples/Gradients` and `examples/Showcase` for combined gradients + paths.
- `visage_graphics/path.h` for API surface and helpers (rounded rects, squircles).
