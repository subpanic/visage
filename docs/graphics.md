# Graphics

## Canvas model

- Immediate-mode drawing API (`visage_graphics/canvas.h`) for shapes, gradients, text, images, SVG, and post-effects.
- Layered compositing with blend modes; automatic shape batching for GPU efficiency.
- Dirty-region tracking via `Region` so only modified areas redraw.
- Logical vs native pixels: `Canvas::setDpiScale` and `setLogicalPixelScale`/`setNativePixelScale`.
- Time helpers (`updateTime`, `deltaTime`, `frameCount`) for animation.

### Quick draw example

```cpp
app.onDraw() = [&](visage::Canvas& c) {
  c.setColor(0xff223344);
  c.fill(0, 0, app.width(), app.height());

  c.setColor(visage::Brush::linear(0xff66ccff, 0xffff66cc, {0, 0}, {app.width(), app.height()}));
  c.squircle(40, 40, 200);

  visage::Font title(24, visage::fonts::Lato_Regular_ttf, app.dpiScale());
  c.setColor(0xffffffff);
  c.text(title, "Hello Visage", 60, 80);
};
```

## Core primitives

- Shapes: rectangles, circles, squircles, arcs, rings, paths (`shapes.h`, `path.h`).
- Brushes/colors: solid, gradients, palette-driven (`palette.h`, `theme.h`).
- Text: font handling (`font.h`), text layout (`text.h`), emoji support per platform.
- SVG: loading and drawing (`svg.h`).
- Effects: blur, bloom, post-processing (`post_effects.h`), backdrop effects, alpha/caching.

### Post-effect snippet

```cpp
auto blur = std::make_unique<visage::BlurPostEffect>();
blur->setBlurRadius(24.0f);

content_frame.setPostEffect(blur.get());           // applies when drawing this frame
overlay_frame.setBackdropEffect(blur.get());       // blur content behind overlay
```

## Rendering backend

- `Renderer` (`visage_graphics/renderer.h`) wraps bgfx; supports swap-chain and windowless contexts.
- Platform shader transpilation handled via bgfx toolchain (HLSL→SPIR-V→GL/Metal/D3D).
- Optional background rendering thread (`VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD`) for multi-threaded submission.

### Windowless rendering

```cpp
visage::ApplicationEditor editor;
editor.setWindowless(800, 600);
editor.onDraw() = [&](visage::Canvas& c) { /* draw */ };
editor.drawWindow();
auto& shot = editor.takeScreenshot(); // use shot.pixels/width/height
```

## Theming

- Theme IDs defined in `visage_graphics/theme.h`; palettes in `palette.h`.
- Frames and widgets consume palette values; overrides allow scoped theme changes.

## Screenshots and capture

- `ApplicationEditor::takeScreenshot` or `Canvas::takeScreenshot` exposes rendered output for documentation/examples.
