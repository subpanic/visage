# Widgets

Visage ships a set of higher-level UI controls built on `Frame`. Public headers live in `visage_widgets/`.

Headers to include: include the specific widget header (e.g., `#include <visage_widgets/button.h>`, `text_editor.h`, `color_picker.h`), and `#include <visage_graphics/font.h>` plus generated fonts header for text.

## Available widgets (high level)

- Buttons (`button.h`)
- Text editor (`text_editor.h`)
- Color picker (`color_picker.h`)
- Palette editor (`palette_editor.h`)
- Shader editor (`shader_editor.h`)
- Shader quad preview (`shader_quad.h`)
- Graphs and bar lists (`graph_line.h`, `bar_list.h`)

## Usage notes

- Widgets use the same event/focus model as `Frame`; set palettes and DPI via parent frames.
- Many widgets expose callbacks for value changes and hooks for custom drawing.
- Palette overrides allow styling hover/active/disabled states without hard-coded colors.

## Examples

- See `examples/Showcase` and other demos for widgets in context.
- Embed snippets in the Examples gallery to illustrate usage with screenshots.
