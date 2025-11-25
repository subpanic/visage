# Graphics

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
- Logical vs native pixels via `Canvas::setDpiScale`, `setLogicalPixelScale`, `setNativePixelScale`.
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

## Core primitives (Canvas)

- Shapes: rectangles, circles, squircles, arcs, rings, paths (`shapes.h`, `path.h`).
- Brushes/colors: solid, gradients, palette-driven (`palette.h`, `theme.h`).
- Text: fonts (`font.h`), layout (`text.h`), emoji.
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
