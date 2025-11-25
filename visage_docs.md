Visage Documentation Plan (Sphinx + Breathe)
===========================================

Goals
- Publish a browsable HTML site with search that covers overview, architecture, lifecycle, API, and examples.
- Include full SDK/API reference generated from headers.
- Provide quickstarts and task-oriented guides (build, platform targets, theming, layout, widgets, embedding assets).
- Host example gallery with code links and screenshots/embeds.

What to explain (architecture snapshot)
- Event → UI → Render chain: `Window` backends (`visage_windowing/windowing.h`) dispatch to `WindowEventHandler` (`visage_app/window_event_handler.*`), into `Frame` tree (`visage_ui/frame.h`). Dirty frames render via `ApplicationEditor::drawWindow` to `Canvas` → `Renderer` (bgfx) for GPU submission.
- UI + layout: `Frame` lifecycle, callbacks, focus, DPI, regions, hit testing, undo stack; flex-like layout in `visage_ui/layout.h`; timers/events in `visage_ui/events.h`.
- Windowing: abstract `Window` with native/client/popup decoration and plugin windows; platform code under `win32/`, `macos/`, `linux/`, `emscripten/`.
- Graphics: immediate-mode `Canvas` (shapes, gradients, text, SVG, post-effects, layers, dirty-region redraw) and theming/palettes (`visage_graphics/theme.h`, `palette.h`); renderer can be windowless or swap-chain.
- Widgets: packaged controls (`visage_widgets/`: buttons, text editor, color picker, shader editor, palette editor, graphs, bar lists).
- Utilities: geometry/dimensions/events (`visage_utils/*`), file embedding (`visage_file_embed/*`), CMake options for amalgamation, widgets, background graphics thread, debugging.
- Examples: runnable demos in `examples/` (Basic, Showcase, BlendModes, Bloom, Gradients, Paths, PostEffects, Layout, MouseEvents, MultiWindow, BringYourOwnWindow, Emscripten template).

Chosen stack
- Sphinx with MyST Markdown (author guides in Markdown).
- Doxygen to emit XML + (optionally) standalone HTML.
- Breathe (and optionally Exhale) to pull C++ API into Sphinx.
- Theme: `sphinx-book-theme` or `furo` (both ship search); keep defaults first, style later.
- Optional: `sphinxcontrib.mermaid` or `sphinx.ext.graphviz` for diagrams; `sphinx-copybutton`, `sphinx_design` for callouts/cards.

Docs layout (proposed IA under docs/)
- index.md: What is Visage, features, supported platforms, quick links.
- getting-started.md: install/build, minimal app (`ApplicationWindow`), CMake options (`VISAGE_*`), run examples/tests.
- architecture.md: subsystems, event flow diagram, render pipeline, background graphics thread, partial redraw.
- graphics.md: `Canvas` drawing model, layers/regions, brushes/colors/gradients, text/SVG, post-effects, shaders.
- ui-layout.md: `Frame` lifecycle, callbacks, focus model, hit testing, input handling, timers, undo, flex layout and dimensions.
- windowing.md: window creation, decorations, DPI, plugin windows, cursor utilities.
- theming.md: palette, theme values/overrides, widgets and palette overrides.
- widgets.md: catalog of controls with code snippets and screenshots.
- assets-embedding.md: `visage_file_embed` flow, shader embedding, resource lookup.
- examples.md: gallery linking to `examples/*`, code snippets, screenshots/embeds; call out emscripten template.
- api/index.rst: auto-generated Breathe pages for namespaces/classes/functions.
- faq-troubleshooting.md: build issues, backend notes, DPI pitfalls, input quirks.

Build pipeline (local and CI)
- Add `docs/Doxyfile` configured for XML output (INPUT pointing to public headers and generated `build/include/visage/**` when available).
- Add `docs/requirements.txt` for Sphinx/Breathe/etc.
- Add `docs/conf.py` and MyST-based sources.
- CMake target `docs`:
  - Run `doxygen docs/Doxyfile`.
  - Run `sphinx-build -b html docs docs/_build/html`.
- CI job: on push/PR, build docs; on main tag, deploy `_build/html` to GitHub Pages (or equivalent).

API coverage plan
- Start annotating: `visage_app/application_window.h`, `visage_app/application_editor.h`, `visage_app/window_event_handler.h`.
- UI core: `visage_ui/frame.h`, `layout.h`, `events.h`, `undo_history.h`, `scroll_bar.h`, `popup_menu.h`, `svg_frame.h`.
- Graphics: `visage_graphics/canvas.h`, `renderer.h`, `region.h`, `layer.h`, `palette.h`, `theme.h`, `post_effects.h`, `shape_batcher.h`, `shapes.h`, `text.h`, `image.h`, `svg.h`.
- Windowing: `visage_windowing/windowing.h` interface plus platform notes.
- Widgets: `visage_widgets/*` public headers.
- Utilities: `visage_utils/events.h`, `space.h`, `dimension.h`, threading/time helpers as needed.
- Document units and coordinate systems (logical vs native pixels, DPI), threading expectations, and lifecycle hooks for each class.

Examples gallery plan
- Script to run examples and capture screenshots via `ApplicationEditor::takeScreenshot()` or OS capture; store under `docs/_static/examples/`.
- For web: build emscripten versions using `examples/emscripten_template.html`; embed iframes or link out.
- Each example entry: short description, key APIs used, source path link, screenshot/iframe.

Search
- Use built-in Sphinx search initially.
- Future: optional Algolia DocSearch if the site goes public; keep HTML IDs stable via MyST anchors.

Publishing
- Default: build to `docs/_build/html`. If hosting on GitHub Pages, publish `docs/_build/html` to `gh-pages` branch.
- Provide `docs/README.md` with quick instructions for contributors.

Comprehensive TODO (Sphinx+Breathe route)
- Repository setup
  - [ ] Create `docs/` with `conf.py`, `requirements.txt`, `index.md` scaffold.
  - [ ] Add `docs/Doxyfile` (XML output on, `EXTRACT_ALL=YES`, `INLINE_SIMPLE_STRUCTS=YES`, `SOURCE_BROWSER=YES`, `REFERENCED_BY_RELATION=YES`, `GENERATE_HTML=NO` or YES for standalone API).
  - [ ] Add `.gitignore` entries for `docs/_build/`, `docs/xml/`, `docs/html/` if needed.
- Build integration
  - [ ] Add CMake `docs` target that runs `doxygen` then `sphinx-build`.
  - [ ] Add pip/venv helper snippet in `docs/README.md`.
  - [ ] Ensure headers generated into `build/include/visage` are included (set `INPUT`/`EXCLUDE_PATTERNS` accordingly).
- Authoring scaffolding
  - [ ] Write `index.md`, `getting-started.md`, `architecture.md`, `graphics.md`, `ui-layout.md`, `windowing.md`, `theming.md`, `widgets.md`, `assets-embedding.md`, `examples.md`, `faq-troubleshooting.md`.
  - [ ] Add nav structure in `index.md` or `toctree` equivalents.
  - [ ] Add initial diagrams (mermaid/graphviz) for event flow and render pipeline.
- API reference wiring
  - [ ] Configure Breathe (`breathe_projects` -> Doxygen XML output) and create `api/index.rst` (or MyST) with `.. doxygenindex::` or Exhale tree.
  - [ ] Decide page granularity (by namespace/class) and add filters for public API only if desired.
- Code annotation pass
  - [ ] Add Doxygen comments to public classes/functions in `visage_app` (window/editor/event handler).
  - [ ] Add comments to `visage_ui` core (`Frame`, layout, events, hit testing, focus, timers).
  - [ ] Add comments to `visage_graphics` core (`Canvas`, `Renderer`, theme/palette, post effects, shapes/text/SVG).
  - [ ] Add comments to `visage_windowing` interface and platform notes.
  - [ ] Add comments to `visage_widgets` public APIs (controls usage).
  - [ ] Add usage examples to key classes (`ApplicationWindow`, `Canvas`, `Frame`).
- Examples gallery
  - [ ] Write capture script for native screenshots; store outputs in `docs/_static/examples/`.
  - [ ] Document each example with description + code link + screenshot; embed web builds if available.
  - [ ] Add instructions to build emscripten examples and link template (`examples/emscripten_template.html`).
- Quality and UX
  - [ ] Enable `sphinx.ext.autosectionlabel`, `sphinx.ext.intersphinx` (for std C++ refs), `sphinx_copybutton`.
  - [ ] Add simple theming (logo, colors) once content stabilizes.
  - [ ] Validate search and internal links; ensure anchors for key sections.
- CI/CD
  - [ ] Add workflow to install doc deps, run `cmake --build . --target docs`, and publish HTML on main/tag.
  - [ ] Gate on warnings-as-errors for Sphinx to keep docs healthy.
- Future enhancements
  - [ ] Algolia DocSearch configuration if public.
  - [ ] Versioned docs (sphinx-multiversion) once releases exist.
  - [ ] Add API changelog page auto-generated from git tags or release notes.

Quick build commands (once scaffolding exists)
- Configure/build code then docs:
  ```
  mkdir -p build && cd build
  cmake ..
  cmake --build . --parallel
  cmake --build . --target docs
  # HTML ends up in docs/_build/html
  ```
- Or standalone docs:
  ```
  pip install -r docs/requirements.txt
  doxygen docs/Doxyfile
  sphinx-build -b html docs docs/_build/html
  ```

Working style guidelines
- Keep guides in MyST Markdown; keep code fences with language tags.
- Prefer API links via Breathe roles once Doxygen XML is in place.
- Include units (logical pixels, native pixels, DPI) and threading expectations in API docs.
- Keep architecture sections diagram-first, then text.
