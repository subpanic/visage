# Visage Onboarding Guide

Goal: give a new agent the shortest path to expert-level understanding of Visage’s architecture, SDK, and docs. Follow the checklist; tick items off as you go.

## Quick orientation
- [x] Skim `README.md` for the high-level pitch and supported platforms.
- [x] Skim `CLAUDE.md` for repo build/testing norms.
- [x] List top-level modules: `visage_app`, `visage_graphics`, `visage_ui`, `visage_widgets`, `visage_windowing`, `visage_utils`, `visage_file_embed`, `examples/`.

## Docs first (no build required)
- [x] Open `docs/_build/html` if present; otherwise skim source docs in `docs/`.
- [x] Scan the nav (guides + API) to know what’s covered.

## Read the docs (guided path)
- [x] `docs/getting-started.md`: build flags, minimal app.
- [x] `docs/architecture.md`: subsystem map and event→UI→render flow.
- [x] `docs/windowing.md`: `Window` abstraction, decorations, plugin windows.
- [x] `docs/ui-layout.md`: `Frame` lifecycle, events, flex layout, DPI.
- [x] `docs/graphics.md`: Canvas drawing vs ShaderPostEffect vs ShaderQuad; post-processing.
- [x] `docs/theming.md`: theme IDs vs palettes vs overrides; consumption patterns.
- [x] `docs/widgets.md`: available widgets and styling hooks.
- [x] `docs/assets-embedding.md`: file embedding, built-in shaders/fonts/icons.
- [x] `docs/shaders.md`: post-process vs quad shaders, custom shader pipeline.
- [x] `docs/animation.md`: timing via timers, canvas time, cross-thread updates.
- [x] `docs/examples.md`: note which demos to run for coverage.

## Code deep dive (headers first)
- [x] `visage_app/application_window.h` / `application_editor.h`: window lifecycle, draw loop, fixed aspect, client decoration.
- [x] `visage_windowing/windowing.h`: `Window` interface and event handler contract.
- [x] `visage_ui/frame.h`, `layout.h`, `events.h`: UI tree, callbacks, input, layout, undo.
- [x] `visage_graphics/canvas.h`, `post_effects.h`, `renderer.h`, `palette.h`, `theme.h`: drawing API, effects, renderer, theming.
- [x] `visage_widgets/*.h`: what higher-level widgets exist and their APIs.
- [x] `visage_utils/space.h`, `dimension.h`, `events.h`: geometry, sizing, event constants.
- [x] `visage_file_embed/*.cmake`: how embedding works.

## Hands-on code inspection (no execution required)
- [x] Read `examples/Basic/basic.cpp` for minimal usage.
- [x] Read `examples/Showcase` for a broad tour of widgets/shaders/effects.
- [x] Read `examples/PostEffects` or `BlendModes` for ShaderPostEffect patterns.
- [x] Read `examples/Layout` and `MouseEvents` for layout/input patterns.
- [x] Skim `examples/BringYourOwnWindow` for standalone window + renderer usage.

## Shader workflow refresher
- [x] Inspect `visage_graphics/embedded.cmake` to see shaderc invocation and target platforms.
- [x] Note embedded namespaces: `visage::shaders`, `visage::fonts`, `visage::icons`.
- [x] Review `examples/PostEffects` and `examples/LiveShaderEditing` for ShaderPostEffect uniform updates.
- [x] Review `visage_widgets/shader_quad.*` and Showcase shader section for ShaderQuad usage.

## Theming and palette usage
- [x] Confirm ID definitions via `visage_graphics/theme.h` macros.
- [x] Trace how widgets consume palette colors/values (e.g., buttons, text_editor).
- [x] Try a quick palette swap in an example to see override propagation.

## Animation and timing
- [x] Review `EventTimer` and `EventManager` in `visage_ui/events.h`.
- [x] Note `Canvas::time()` / `deltaTime()` usage in examples for animation.
- [x] Understand `VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD` flag implications.

## Windowing specifics
- [x] Skim platform backends under `visage_windowing/{win32,macos,linux,emscripten}` to see API coverage (drag/drop, cursor, DPI).
- [x] Review hit testing and focus paths in `visage_app/window_event_handler.*`.

## Notes on build flags (context only, no build needed)
- [x] Know key CMake options: `VISAGE_BUILD_EXAMPLES`, `VISAGE_AMALGAMATED_BUILD`, `VISAGE_ENABLE_WIDGETS`, `VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD`, `VISAGE_ENABLE_GRAPHICS_DEBUG_LOGGING`.
- [x] Remember docs target depends on headers + doc sources if you ever regenerate docs.

## Contribution hygiene
- [x] Follow existing coding patterns (DPI awareness, palette usage, dirty-region redraw).
- [x] Use Doxygen comments on public APIs to keep docs healthy.
- [x] Keep new drawing code palette-aware (`paletteColor`/`paletteValue`) and minimal redraw.

## What “expert” means here
- You can explain the event → Frame → Canvas → Renderer flow and Window abstraction.
- You know when to choose Canvas vs ShaderPostEffect vs ShaderQuad.
- You can add a widget or custom Frame with correct layout, DPI, palette, and input handling.
- You can embed assets/shaders and consume them.
- You can guide others through the docs and examples for specific use-cases.
