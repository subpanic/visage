# Examples

Visage includes runnable demos under `examples/`. Key entries:

- **Basic**: minimal window and draw loop.
- **Showcase**: broad feature tour with widgets and effects.
- **BlendModes**, **Bloom**, **Gradients**, **Paths**, **PostEffects**: focused graphics techniques.
- **Layout**, **MouseEvents**: UI and input demonstrations.
- **MultiWindow**: multiple native windows.
- **BringYourOwnWindow**: render into a window you create.
- **Emscripten template**: web build scaffold.

Headers to include: each example includes the specific module it needs—common ones are `<visage_app/application_window.h>`, `<visage_graphics/canvas.h>`, plus generated resource headers (`<visage_graphics/fonts.h>`, `<visage_graphics/shaders.h>`).

Related guides: [Graphics](graphics), [UI & Layout](ui-layout), [Paths](paths), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Theming](theming), [Architecture](architecture), [Getting Started](getting-started)

## Documentation gallery plan

- Capture screenshots via `ApplicationEditor::takeScreenshot` or OS capture; store in `docs/_static/examples/`.
- Pair each example with a short description, key APIs used, source path link, and screenshot/iframe.
- For web builds, embed iframes using the Emscripten output page where available.

## Running examples

```
cmake --build . --target ExampleShowcase
./examples/ExampleShowcase
```

Each example target follows the pattern `Example<Name>` after configuring with `VISAGE_BUILD_EXAMPLES=ON`.
