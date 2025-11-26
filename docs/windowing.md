# Windowing

Related guides: [Graphics](graphics), [UI & Layout](ui-layout), [Paths](paths), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Theming](theming), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Window interface

Headers to include: `#include <visage_windowing/windowing.h>`

- `visage_windowing/windowing.h` exposes the cross-platform `Window` abstraction with draw callbacks and normalized events.
- Decorations: native, client (custom chrome), popup.
- Plugin windows: create windows that embed into a host with `createPluginWindow`.

## Creation helpers

- `createWindow(x, y, width, height, decoration)` and `createWindow(width, height)` convenience overloads.
- `computeWindowBounds` to translate logical dimensions to native pixel rectangles.
- `Window::setWindowTitle`, `setAlwaysOnTop`, `setFixedAspectRatio`, `show/showMaximized/hide/close`.

### Example: plugin window

Headers to include: `#include <visage_windowing/windowing.h>` and your renderer include if you initialize manually.

```cpp
void* host_handle = /* HWND/NSView/etc from host */;
auto window = visage::createPluginWindow(800, 600, host_handle);
window->setWindowTitle("Embedded Visage");
window->setDrawCallback([](double t) {
  visage::Renderer::instance().updateTime(t);
});
window->show();
window->runEventLoop();
```

## DPI and coordinates

- `dpiScale()` reflects platform scale; conversions `convertToNative` / `convertToLogical`.
- Mouse relative mode support, cursor utilities (`setCursorStyle`, `setCursorVisible`, `cursorPosition`, `setCursorPosition`).

## Event handling

- `Window::EventHandler` delivers hit tests, mouse/keyboard/text input, focus, resize, file drag/drop, and drag-source callbacks.
- `WindowEventHandler` bridges native events to the `Frame` tree and manages focus/drag/hover tracking.

## Platform backends

- Implementations live in `visage_windowing/win32`, `macos`, `linux`, and `emscripten`.
- Graphics backend selection per platform: D3D11 (Windows), Metal (macOS), Vulkan (Linux), WebGL (Emscripten).
