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
- SVG-style helpers exist on `Path::CommandList`: `arcTo`, `horizontalTo`, `verticalTo`, `smoothBezierTo`, `smoothQuadraticTo`, etc.

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

Stroke with dashes and miters:
```cpp
std::vector<float> dash = { 8.0f, 4.0f }; // on/off
canvas.setColor(0xffe89cff);
canvas.stroke(p, 10, 10, 200, 200, 6.0f,
              visage::Path::Join::Miter,
              visage::Path::EndCap::Butt,
              dash, /*dash_offset=*/canvas.time() * 20.0f);
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

## Animating paths

- **Dash offset / stroke animation**: vary the `dash_offset` parameter each frame (see stroke example above) for marching ants or loading rings.
- **Shape morphing**: rebuild a path each tick based on a parameter (e.g., lerp control points), then `redraw()`.
- **Transforms without rebuild**: draw the same `Path` at different positions/sizes by changing the `fill`/`stroke` arguments; only rebuild when topology changes.

Example: orbiting arc with `EventTimer`
```cpp
class Orbit : public visage::Frame, public visage::EventTimer {
public:
  Orbit() { startTimer(16); }
  void timerCallback() override { angle_ += 0.05f; redraw(); }
  void draw(visage::Canvas& c) override {
    visage::Path ring;
    ring.arc({0, 0}, 1.0f, angle_, 1.5f);   // arc(center, radius, start, radians)
    float size = std::min(width(), height()) * 0.6f;
    c.setColor(0xff66ccff);
    c.stroke(ring, width() * 0.2f, height() * 0.2f, size, size, 8.0f,
             visage::Path::Join::Round, visage::Path::EndCap::Round);
  }
private:
  float angle_ = 0.0f;
};
```

## Examples to study

- `examples/Paths/paths.cpp`: various path constructions and fills/strokes.
- `examples/Gradients` and `examples/Showcase` for combined gradients + paths.
- `visage_graphics/path.h` for API surface and helpers (rounded rects, squircles).
