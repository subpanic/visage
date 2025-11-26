# Getting Started

Related guides: [Architecture](architecture), [Graphics](graphics), [UI & Layout](ui-layout), [Paths](paths), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Theming](theming), [Windowing](windowing), [Examples](examples)

## Build the library

Visage uses CMake (>=3.17) and C++17. Typical native build:

```
mkdir -p build && cd build
cmake ..
cmake --build . --parallel
```

Examples and tests are enabled by default when building this repository directly (`VISAGE_BUILD_EXAMPLES=ON`, `VISAGE_BUILD_TESTS=ON`). Optional flags:

- `VISAGE_AMALGAMATED_BUILD` (ON): compile sources together for faster builds.
- `VISAGE_ENABLE_WIDGETS` (ON): include the higher-level widgets module.
- `VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD` (OFF): render on a background thread (non-emscripten).
- `VISAGE_ENABLE_GRAPHICS_DEBUG_LOGGING` (OFF): verbose graphics logs in debug builds.
- `VISAGE_ADDRESS_SANITIZER` (OFF): enable ASan.

## Minimal application

Headers to include: `#include <visage_app/application_window.h>`

```cpp
#include <visage_app/application_window.h>

int main() {
  visage::ApplicationWindow app;

  app.onDraw() = [&app](visage::Canvas& canvas) {
    canvas.setColor(0xffff00ff);
    canvas.fill(0, 0, app.width(), app.height());
  };

  app.show(800, 600);
  app.runEventLoop();
  return 0;
}
```

### Using dimensions and DPI

Headers to include: `#include <visage_utils/dimension.h>`

```cpp
app.setWindowDimensions(
  visage::Dimension::percent(10),  // x: 10% of display width
  visage::Dimension::percent(10),  // y: 10% of display height
  visage::Dimension::pixels(800),  // width: 800 logical px
  visage::Dimension::pixels(600)   // height: 600 logical px
);
```

## Build and run an example

```
cd build
cmake --build . --target ExampleBasic
./examples/ExampleBasic
```

Examples live in `examples/` (Showcase, BlendModes, Bloom, Gradients, Paths, PostEffects, Layout, MouseEvents, MultiWindow, BringYourOwnWindow, Emscripten template).

## Build the docs

```
pip install -r docs/requirements.txt
cd docs
doxygen Doxyfile
sphinx-build -b html . _build/html
```

See `docs/Doxyfile` and `docs/conf.py` for configuration. The CMake `docs` target will run both steps once enabled.
