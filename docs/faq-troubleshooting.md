# FAQ and Troubleshooting

## Build issues

- **Graphics dependencies fail to fetch**: ensure network access during CMake configure; bgfx/bx/bimg and freetype are pulled via `FetchContent` in `visage_graphics/CMakeLists.txt`.
- **Missing platform SDK**: install platform toolchains (Xcode for macOS/Metal, Windows SDK + D3D11, Vulkan SDK for Linux).
- **CMake cannot find Objective-C**: on Apple platforms, ensure Xcode command-line tools are installed (`xcode-select --install`).

## Rendering questions

- **Nothing draws**: verify `onDraw` callback is set and frames request redraw (`redraw` / `redrawAll`). Check that `Canvas` is paired to a window (`ApplicationEditor::addToWindow`).
- **Blurry output**: confirm DPI scale (`Window::dpiScale`) and whether you want logical vs native pixel scale (`Canvas::setLogicalPixelScale` vs `setNativePixelScale`).
- **Performance**: leverage dirty-region rendering and shape batching; avoid forcing full redraws unless necessary.

## Input and focus

- **Keyboard input not reaching widget**: ensure the widget accepts keystrokes and has focus (`requestKeyboardFocus`). For text input, `receivesTextInput` must return true.
- **Custom hit testing**: override `Frame::hitTest` for draggable/resize areas; use client window decoration for custom chrome where platform allows.

## Documentation

- **API reference empty**: run `doxygen docs/Doxyfile` before `sphinx-build` so Breathe can load XML.
- **Search missing**: confirm `_build/html` is produced by Sphinx and assets are served from the correct base URL when hosting.
