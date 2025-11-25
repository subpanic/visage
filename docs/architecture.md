# Architecture

## High-level flow

1. **Windowing**: Platform-specific `Window` backends (`visage_windowing/windowing.h` and platform folders) surface native events and draw callbacks.
2. **Event handling**: `WindowEventHandler` (`visage_app/window_event_handler.*`) translates native events to `MouseEvent`/`KeyEvent` and dispatches into the `Frame` tree.
3. **UI tree**: `Frame` (`visage_ui/frame.h`) nodes own layout, input, focus, visibility, DPI scale, and dirty-region tracking.
4. **Rendering**: Dirty frames render into `Canvas` regions; `ApplicationEditor::drawWindow` submits to `Renderer` (bgfx) for GPU compositing.
5. **Presentation**: The renderer composites layers (blend modes, post-effects) and presents via swap chain or windowless surface.

## Subsystems

- **UI & Layout**: Flex-like layout (`visage_ui/layout.h`), focus/keyboard model, hit testing, timers (`visage_ui/events.h`), undo stack.
- **Graphics**: Immediate-mode `Canvas` (`visage_graphics/canvas.h`) with shapes, gradients, text, SVG, post-effects, caching, dirty region redraw. Theming via `theme.h` + `palette.h`.
- **Windowing**: Abstract `Window` interface with native/client/popup decoration; plugin windows for embedding into hosts.
- **Widgets**: Higher-level controls in `visage_widgets/` (buttons, text editors, color picker, shader/palette editors, graphs, bar lists).
- **Utilities**: Geometry (`visage_utils/space.h`), dimensions (`visage_utils/dimension.h`), event constants (`visage_utils/events.h`), threading/time helpers.
- **Embedding**: Build-time resource embedding via `visage_file_embed` CMake helpers.

## Lifecycle and threading

- `ApplicationWindow` creates or attaches to a `Window`, sets draw callback, and runs the event loop.
- Rendering is GPU-backed; optional background graphics thread (`VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD`) offloads submission when enabled (non-emscripten).
- DPI and logical vs native pixels: `Window` exposes `dpiScale`; `Canvas` can switch between logical/native pixel scales.
- Dirty-region rendering: `Frame` requests redraw; `Canvas` batches shapes by layer/region and only repaints dirty areas.

## Build-time concerns

- CMake options control amalgamation, widgets, background graphics thread, and debug logging.
- Third-party graphics dependencies (bgfx, bx, bimg, freetype) fetched via `FetchContent` inside `visage_graphics/CMakeLists.txt`.
- Platform backends live under `visage_windowing/{win32,macos,linux,emscripten}` with API surface in `windowing.h`.
