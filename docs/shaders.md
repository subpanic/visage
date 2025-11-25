# Shaders

Visage ships cross-platform shaders compiled at build time and lets you author custom ones for effects or overlays.

## Where shaders live

Headers to include for built-ins: `<visage_graphics/shaders.h>`, `<visage_graphics/post_effects.h>`. For your shaders, include the generated header you declared in CMake.

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

## Approaches: Post-process vs Quad

**ShaderPostEffect (fullscreen/backdrop)**
- What: Applies a shader to a frame’s rendered output or backdrop.
- When: Color transforms, blur/glow, warps, CRT/glitch, vignette—anything “after” normal Canvas drawing.
- Data: You get the frame texture; pass uniforms for parameters.
- Attach: `frame.setPostEffect(effect)` or `frame.setBackdropEffect(effect)`.
- Example:
  ```cpp
  post->setUniformValue("u_time", time);
  post->setUniformValue("u_params", strength, threshold, 0.0f, 0.0f);
  frame.setPostEffect(post.get());
  overlay.setBackdropEffect(post.get());
  ```
  See `examples/PostEffects`, `examples/LiveShaderEditing`.

**ShaderQuad (region/preview)**
- What: Renders your shader on a quad via `Canvas::shader` inside the UI.
- When: Shader previews, small GPU-driven tiles, contained visualizers.
- Data: Uniforms and built-in varyings; you don’t automatically get the frame texture.
- Attach: Instantiate `ShaderQuad` with your embedded vertex/fragment shaders and add it as a child frame.
- See Showcase “Shaders” section for a live example.

Choosing:
- Need whole-frame stylization/backdrop effects? → ShaderPostEffect.
- Need a contained shader-driven region/preview? → ShaderQuad.

## Simple example: blue circle on white background

### Shaders (`vs_circle.sc` / `fs_circle.sc`)
Applies as a ShaderPostEffect (fullscreen/backdrop) or could be adapted for ShaderQuad with the same shader pair.

```glsl
// vs_circle.sc
$input  a_position
$output v_texcoord0
void main() {
  gl_Position = mul(u_modelViewProj, vec4(a_position.xy, 0.0, 1.0));
  v_texcoord0 = a_position.xy * 0.5 + 0.5; // map clip space to 0..1
}
```

```glsl
// fs_circle.sc
$input v_texcoord0
uniform vec4 u_color; // rgba
void main() {
  vec2 uv = v_texcoord0;
  float dist = distance(uv, vec2(0.5));
  float mask = smoothstep(0.5, 0.48, dist);
  gl_FragColor = mix(vec4(1.0), u_color, mask); // white background, blue circle
}
```

Add to CMake (using `visage_embed_shaders`):

```cmake
set(MY_SHADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/vs_circle.sc
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders/fs_circle.sc
)
visage_embed_shaders(MyCircleShaders "circle_shaders.h" "myapp::shaders" "${MY_SHADERS}")
target_link_libraries(my_app PRIVATE MyCircleShaders)
```

### Custom Frame using the shader

```cpp
#include <visage_ui/frame.h>
#include <visage_graphics/post_effects.h>
#include "circle_shaders.h" // generated header from CMake

class CircleFrame : public visage::Frame {
public:
  CircleFrame() {
    effect_ = std::make_unique<visage::ShaderPostEffect>(
      myapp::shaders::vs_circle,
      myapp::shaders::fs_circle
    );
    // RGBA blue
    effect_->setUniformValue("u_color", 0.2f, 0.5f, 0.9f, 1.0f);
  }

  void init() override { setBounds(0, 0, 400, 400); }

  void draw(visage::Canvas& c) override {
    c.setColor(0xffffffff);
    c.fill(0, 0, width(), height());    // white background
    setPostEffect(effect_.get());       // apply the circle shader as post effect
  }

private:
  std::unique_ptr<visage::ShaderPostEffect> effect_;
};
```

Attach `CircleFrame` to your root frame (or `ApplicationWindow`) and it will render the blue circle effect over the white fill.

## Notes

- Shader entry points are declared in `.sc` files; varyings are shared via `varying.def.sc`.
- `visage_graphics/embedded.cmake` targets multiple backends; ensure shaderc binaries for your host are executable.
- Keep uniform names/types in sync between C++ and shader code; `setUniformValue` packs up to 4 floats. For textures/samplers, follow the patterns in the examples.
