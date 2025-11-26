# Assets and Embedding

Related guides: [Graphics](graphics), [Paths](paths), [Shaders](shaders), [Animation](animation), [Theming](theming), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Build-time embedding

Headers to include: generated headers from `add_embedded_resources` (e.g., `"my_assets.h"`), and for built-ins: `<visage_graphics/shaders.h>`, `<visage_graphics/fonts.h>`, `<visage_graphics/icons.h>`.

- `visage_file_embed` provides CMake helpers to bake assets (shaders, images, etc.) into the binary.
- Use `add_embedded_resources(project, include_filename, namespace, files)` from `visage_file_embed/CMakeLists.txt`.
- Generated sources end up in a `_generated` folder per target; an include header exposes `EmbeddedFile` lookups.

## Typical flow

1. List files to embed in your CMakeLists.
2. Call `add_embedded_resources` with a namespace (e.g., `visage::graphics`).
3. Include the generated header and fetch resources by name.

```cmake
set(MY_ASSETS
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/image.png
  ${CMAKE_CURRENT_SOURCE_DIR}/resources/my_shader.sc
)
add_embedded_resources(MyAppAssets "my_assets.h" "myapp::assets" "${MY_ASSETS}")
target_link_libraries(my_app PRIVATE MyAppAssets)
```

```cpp
#include "my_assets.h"
using myapp::assets;
auto png = assets::image_png; // EmbeddedFile with .data/.size
```

## Built-in Visage resources

- Core shaders, fonts, and icons are embedded in `VisageGraphicsEmbeds` (see `visage_graphics/embedded.cmake`).
- Namespaces:
  - `visage::shaders` (e.g., `vs_custom`, `fs_sepia`, `fs_glitch`)
  - `visage::fonts` (e.g., `Lato_Regular_ttf`, `DroidSansMono_ttf`, platform emoji fonts)
  - `visage::icons` (SVG assets)

### Using embedded fonts

```cpp
#include <visage_graphics/font.h>
#include <visage_graphics/fonts.h> // generated header

visage::Font title(20.0f, visage::fonts::Lato_Regular_ttf, dpi_scale);
canvas.setColor(0xffffffff);
canvas.text(title, "Visage", 16, 32);
```

### Using embedded shaders

Embedded shader binaries are generated from `.sc` sources per platform via `shaderc` (DirectX/Metal/Vulkan/OpenGL/ES targets). To reference them:

```cpp
#include <visage_graphics/shaders.h> // generated header

auto post = std::make_unique<visage::ShaderPostEffect>(
  visage::shaders::vs_custom,
  visage::shaders::fs_glitch
);
post->setUniformValue("u_time", time);
frame.setPostEffect(post.get());
```

## Guidance

- Keep embedded files small; large assets may belong on disk or streamed.
- Prefer embedding shaders to avoid runtime file I/O and ensure cross-platform packaging.
