# Graphics

## Canvas model

- Immediate-mode drawing API (`visage_graphics/canvas.h`) for shapes, gradients, text, images, SVG, and post-effects.
- Layered compositing with blend modes; automatic shape batching for GPU efficiency.
- Dirty-region tracking via `Region` so only modified areas redraw.
- Logical vs native pixels: `Canvas::setDpiScale` and `setLogicalPixelScale`/`setNativePixelScale`.
- Time helpers (`updateTime`, `deltaTime`, `frameCount`) for animation.

## Core primitives

- Shapes: rectangles, circles, squircles, arcs, rings, paths (`shapes.h`, `path.h`).
- Brushes/colors: solid, gradients, palette-driven (`palette.h`, `theme.h`).
- Text: font handling (`font.h`), text layout (`text.h`), emoji support per platform.
- SVG: loading and drawing (`svg.h`).
- Effects: blur, bloom, post-processing (`post_effects.h`), backdrop effects, alpha/caching.

## Rendering backend

- `Renderer` (`visage_graphics/renderer.h`) wraps bgfx; supports swap-chain and windowless contexts.
- Platform shader transpilation handled via bgfx toolchain (HLSL→SPIR-V→GL/Metal/D3D).
- Optional background rendering thread (`VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD`) for multi-threaded submission.

## Theming

- Theme IDs defined in `visage_graphics/theme.h`; palettes in `palette.h`.
- Frames and widgets consume palette values; overrides allow scoped theme changes.

## Screenshots and capture

- `ApplicationEditor::takeScreenshot` or `Canvas::takeScreenshot` exposes rendered output for documentation/examples.
