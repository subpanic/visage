# Shaders

Visage ships cross-platform shaders compiled at build time and lets you author custom ones for effects or overlays.

## Where shaders live

- Library shaders are under `visage_graphics/shaders/` (`vs_*` and `fs_*` files).
- Examples include extra shaders in `examples/shaders/`.
- `visage_graphics/embedded.cmake` compiles `.sc` sources with bgfx `shaderc` into platform-specific binaries (D3D/Metal/Vulkan/GL/ES) and embeds them in `visage::shaders`.

## Adding your own shaders

1. Create vertex (`vs_name.sc`) and fragment (`fs_name.sc`) files.
2. Add them to a CMake list and call `visage_embed_shaders` (or `add_embedded_resources` if you already have binaries).
3. Include the generated header to access `EmbeddedFile` handles.

```cmake
set(MY_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_my_effect.sc
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_my_effect.sc
)
visage_embed_shaders(MyAppShaders "my_shaders.h" "myapp::shaders" "${MY_SHADERS}")
target_link_libraries(my_app PRIVATE MyAppShaders)
```

```cpp
#include "my_shaders.h"
auto effect = std::make_unique<visage::ShaderPostEffect>(
  myapp::shaders::vs_my_effect,
  myapp::shaders::fs_my_effect
);
```

## Using ShaderPostEffect

`ShaderPostEffect` applies a fullscreen effect to a frame or backdrop. Set uniforms before submit:

```cpp
post->setUniformValue("u_time", time);
post->setUniformValue("u_params", strength, threshold, 0.0f, 0.0f);
frame.setPostEffect(post.get());          // render over frame
overlay.setBackdropEffect(post.get());    // blur/tint content behind overlay
```

See `examples/PostEffects` and `examples/LiveShaderEditing` for live tweaks and uniform updates.

## Using custom shaders for drawables

For single-draw shaders, wrap vertex/fragment pairs in `visage::Shader` and pass through components that accept shaders (e.g., custom Layer usage). Post-processing via `ShaderPostEffect` is the simplest path for most custom visuals.

## Notes

- Shader entry points are declared in `.sc` files; varyings are shared via `varying.def.sc`.
- `visage_graphics/embedded.cmake` targets multiple backends; ensure shaderc binaries for your host are executable.
- Keep uniform names/types in sync between C++ and shader code; `setUniformValue` packs up to 4 floats. For textures/samplers, follow the patterns in the examples.
