# Fonts and Text

Headers to include: `#include <visage_graphics/font.h>`, `#include <visage_graphics/text.h>`, and `#include <visage_graphics/canvas.h>`. For built-ins: `#include <visage_graphics/fonts.h>`.

Related guides: [Graphics](graphics), [UI & Layout](ui-layout), [Theming](theming), [Animation](animation), [Assets/Embedding](assets-embedding), [Examples](examples), [Getting Started](getting-started)

## What’s provided

- Embedded fonts: `visage_graphics/embedded.cmake` bakes Lato (regular/bold), DroidSansMono, and platform emoji fonts into `visage::fonts::*`. Include `<visage_graphics/fonts.h>` to reference them.
- Unicode + emoji: `Font` and `Canvas::text` handle Unicode strings; emoji glyphs are available through the embedded emoji face.
- Caching: fonts are reference-counted (`FontCache`); reusing `Font`/`Text` objects avoids layout churn.

## Creating fonts

```cpp
#include <visage_graphics/font.h>
#include <visage_graphics/fonts.h>

float dpi = app.dpiScale();                        // from ApplicationWindow
visage::Font title(24.0f, visage::fonts::Lato_Regular_ttf, dpi);
visage::Font mono  = visage::Font(14.0f, visage::fonts::DroidSansMono_ttf, dpi);

// Alternate sources:
visage::Font fromFile(16.0f, "/path/to/custom.ttf", dpi);
visage::Font fromBytes(18.0f, font_bytes, font_size, dpi);
```

Tips:
- Always pass the current DPI. On DPI changes (`Frame::dpiChanged`) rebuild with `font.withDpiScale(new_dpi)` to keep metrics correct.
- Reuse `Font` instances; `withSize` returns a resized copy while sharing the packed font atlas.

## Drawing text with Canvas

Quick draw:
```cpp
visage::Font body(16.0f, visage::fonts::Lato_Regular_ttf, canvas.dpiScale());
canvas.setColor(0xffffffff);
canvas.text("Hello Visage 👋", body, visage::Font::kTopLeft,
            24, 24, canvas.width() - 48, body.lineHeight());
```

Using `Text` for reuse/multi-line:
```cpp
visage::Text label({"Multi-line text\nrespects emojis 😎"}, body,
                   visage::Font::kTopLeft, /*multi_line=*/true);
canvas.text(&label, 32, 32, 300, 200); // cached glyph layout
```

Alignment and sizing:
- `Font::Justification`: combine horizontal + vertical (e.g., `kCenter`, `kTopLeft`, `kBottomRight`).
- `canvas.text(string, font, justification, x, y, width, height)` fits into the box and aligns accordingly.
- Use `canvas.setLogicalPixelScale()` (default) for DPI-scaled coordinates; switch to `setNativePixelScale()` if you need exact device pixels.

## Measuring and wrapping

- `font.lineHeight()`, `capitalHeight()`, `lowerDipHeight()` for layout metrics.
- `font.stringWidth(u32string)` to measure a string at current DPI.
- `font.lineBreaks(text, width)` returns indices where you should wrap for a given width.
- `font.widthOverflowIndex(...)` tells where text exceeds a width (useful for ellipsis).

Example: manual wrapping with ellipsis
```cpp
auto idx = font.widthOverflowIndex(text.c_str(), text.size(), max_width);
if (idx >= 0) {
  text = text.substr(0, idx) + u"...";
}
```

## Choosing fonts and palettes

- Use palette-driven colors: `canvas.setColor(paletteColor(theme::ColorId::...))` so themes can recolor text.
- Mix fonts: e.g., headings with Lato Bold, body with Regular, code with DroidSansMono.

## Performance and quality

- Reuse `Text` instances for static labels to avoid per-frame string parsing.
- Avoid rebuilding fonts every frame; construct once per DPI bucket.
- Large emoji/text blocks: keep draw bounds tight so dirty-region redraw stays small.
- For crisp text at native scale (e.g., screenshots), call `canvas.setNativePixelScale()` before drawing text and restore logical scale afterward.
