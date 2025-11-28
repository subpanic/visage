# Graphics

Related guides: [UI & Layout](ui-layout), [Paths](paths), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Theming](theming), [Windowing](windowing), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Drawing approaches at a glance

- **Canvas immediate mode (most common)**  
  Headers: `#include <visage_graphics/canvas.h>`, `<visage_graphics/font.h>`, generated assets (`<visage_graphics/fonts.h>`).  
  Use when you want CPU-driven drawing of shapes/text/SVG with palette/theming and partial redraw.
- **ShaderPostEffect (fullscreen/backdrop effects)**  
  Headers: `#include <visage_graphics/post_effects.h>`, generated shaders (`<visage_graphics/shaders.h>` or your own).  
  Use for color grading, blur, glow, distortions applied over a frame’s output or behind overlays.
- **ShaderQuad (shader-driven quad content)**  
  Headers: `#include <visage_widgets/shader_quad.h>`, generated shaders.  
  Use for small shader previews or GPU-only content inside the UI; renders one quad with your shader.

## Canvas model (immediate mode)

- Shapes, text, SVG, images, gradients, post-effects, blend modes; dirty-region redraw.
- Logical vs native pixels via `Canvas::setDpiScale`, `setLogicalPixelScale`, `setNativePixelScale`; DPI comes from the hosting `ApplicationWindow`.
- Save/restore state with `saveState`/`restoreState` to manage position, clamp, blend mode, and palette overrides.
- Clamp drawing to a rect with `setClampBounds` (resets on `restoreState`); keep redraw regions tight.
- Layers: `setBlendMode` controls blending per draw call; use separate layers if you need different z-order or mask composition.
- Time helpers (`updateTime`, `deltaTime`, `frameCount`) for animation. Use {doc}`animation` for timing patterns.

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

### Canvas essentials in practice

```cpp
visage::Font label_font(14.0f, visage::fonts::Lato_Regular_ttf, c.dpiScale());
visage::Path ring;
ring.circle({0, 0}, 100.0f);                // build once and reuse

c.saveState();
c.setClampBounds(20, 20, c.width() - 40, c.height() - 40);
c.setColor(visage::Brush::radial(0xff1c1f2b, 0xff0f1118, {0.5f, 0.5f}));
c.fill(0, 0, c.width(), c.height());       // clamped, avoids touching edges

c.setBlendMode(visage::BlendMode::Add);
c.setColor(0x44ff99ff);
c.stroke(ring, c.width() * 0.5f - 110, c.height() * 0.5f - 110,
         220, 220, 6.0f);

c.setBlendMode(visage::BlendMode::Alpha);
c.setColor(0xffffffff);
c.text("Status: Ready", label_font, visage::Font::kBottomRight,
       0, 0, c.width() - 32, c.height() - 24);
c.restoreState();
```

Tips:
- Build shapes/paths once; mutate brushes and positions each frame.
- Palette-aware colors: `c.setColor(theme::ColorId::...)` to respect themes/overrides.
- For masks, use `BlendMode::MaskAdd`/`MaskRemove`; for glow/overlays use `Add` or `Multiply`.
- Switch to `setNativePixelScale()` before pixel-perfect captures, then back to logical scale.

## Core primitives (Canvas)

- Shapes: rectangles, circles, squircles, arcs, rings, paths (`shapes.h`, `path.h`).
- Brushes/colors: solid, gradients, palette-driven (`palette.h`, `theme.h`).
- Text: fonts (`font.h`), layout (`text.h`), emoji. See {doc}`fonts-and-text` for more.
- SVG: `svg.h`.
- Effects: blur, bloom, post-processing (`post_effects.h`), backdrop effects, alpha/caching.

## Post-processing (ShaderPostEffect)

Headers: `#include <visage_graphics/post_effects.h>` and shader headers.

Use when you want to process the rendered frame (or backdrop) with a shader. Best for:
- Glow/blur/bloom, color transforms, warps, CRT/glitch effects.
- Applying different effects per-frame subtree (attach to a Frame).

Snippet:
```cpp
auto blur = std::make_unique<visage::BlurPostEffect>();
blur->setBlurRadius(24.0f);
content_frame.setPostEffect(blur.get());           // over frame
overlay_frame.setBackdropEffect(blur.get());       // behind overlay
```

Pros: one draw call, GPU heavy-lifting, easy fullscreen effects.  
Cons: fullscreen-style; less granular than per-primitive shaders; data must be provided via uniforms/textures.

## Quad shaders (ShaderQuad)

Headers: `#include <visage_widgets/shader_quad.h>` and shader headers.

Use when you want a GPU-driven quad rendered inside the UI (e.g., shader previews, tiny GPU toys). It calls `Canvas::shader` under the hood.

Pros: single draw; good for previews/demos; leverages palette color as a uniform.  
Cons: limited to a rectangle; you own the shader; less flexible for complex layouts than Canvas primitives.

## Rendering backend

- `Renderer` (`visage_graphics/renderer.h`) wraps bgfx; supports swap-chain and windowless contexts.
- Platform shader transpilation handled via bgfx toolchain (HLSL→SPIR-V→GL/Metal/D3D).
- Optional background rendering thread (`VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD`) for multi-threaded submission (non-emscripten).

### Windowless rendering

Headers to include: `#include <visage_app/application_editor.h>`

```cpp
visage::ApplicationEditor editor;
editor.setWindowless(800, 600);
editor.onDraw() = [&](visage::Canvas& c) { /* draw */ };
editor.drawWindow();
auto& shot = editor.takeScreenshot(); // use shot.pixels/width/height
```

### Custom shaders

Headers to include: `#include <visage_graphics/post_effects.h>`, generated shader header (e.g., `<visage_graphics/shaders.h>`), and your own generated shader header if applicable.

For post-processing or custom passes, see {doc}`shaders` for how to add `.sc` shader sources, embed them, and apply with `ShaderPostEffect`.

## Theming

- Theme IDs defined in `visage_graphics/theme.h`; palettes in `palette.h`.
- Frames and widgets consume palette values; overrides allow scoped theme changes.

## Screenshots and capture

- `ApplicationEditor::takeScreenshot` or `Canvas::takeScreenshot` exposes rendered output for documentation/examples.
