# Theming

Headers to include: `#include <visage_graphics/theme.h>`, `#include <visage_graphics/palette.h>`, and your generated theme definitions if applicable.

Related guides: [Graphics](graphics), [UI & Layout](ui-layout), [Paths](paths), [Shaders](shaders), [Animation](animation), [Assets/Embedding](assets-embedding), [Widgets](widgets), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Overview: themes vs palettes

- **Themes (IDs)**: Compile-time identifiers defined with `VISAGE_THEME_COLOR` / `VISAGE_THEME_VALUE` in `visage_graphics/theme.h`. They describe *what* needs styling (e.g., Background, Accent, Radius) and carry a default value and group name.
- **Palettes**: Runtime containers (`visage::Palette`) that assign concrete colors/values to those IDs. Multiple palettes can exist (e.g., light, dark, high-contrast).
- **Overrides**: Optional variants (`theme::OverrideId`) to adjust palette entries for a subtree or state (hover/active).

Think of theme IDs as the API, palettes as the data, and overrides as scoped deltas.

## How they work together

1. Define IDs in headers using macros (one-time).
2. Create one or more `Palette` instances that set colors/values for those IDs.
3. Attach a palette to a root frame (`frame.setPalette(&palette)`); children inherit it.
4. Optionally apply overrides (`frame.setPaletteOverride(override_id)`) to tweak a subtree.
5. In draw code, consume via `paletteColor(id)` / `paletteValue(id)` so theme switching and overrides take effect.

## Defining theme IDs

- Use `VISAGE_THEME_COLOR(name, default_rgba)` and `VISAGE_THEME_VALUE(name, default_float)` in headers.
- IDs carry a name and group (derived from the file name) for organization and docs.

## Creating and attaching palettes

- Start with `visage::Palette p;` and set values: `p.setColor(ColorId, rgba); p.setValue(ValueId, float);`
- Attach to the top-level frame you want to style: `root.setPalette(&p);` (propagates to children).
- You can clone palettes to create variants: `Palette dark = base; dark.setColor(...);`

### Default palette

Visage provides a built-in palette (see `visage_graphics/palette.cpp`). If you do nothing, widgets use it. To customize globally, create your own palette at startup and attach it to the root frame.

### Example: define and apply a custom palette

```cpp
#include <visage_graphics/palette.h>
#include <visage_graphics/theme.h>

visage::Palette makeDarkPalette() {
  visage::Palette p;
  p.setColor(visage::theme::ColorId::nextId("Background", __FILE__, 0xff12141a), 0xff12141a);
  p.setColor(visage::theme::ColorId::nextId("Accent", __FILE__, 0xff66d9ef), 0xff66d9ef);
  p.setValue(visage::theme::ValueId::nextId("Radius", __FILE__, 6.0f), 6.0f);
  return p;
}

// In your app init:
auto palette = makeDarkPalette();
root_frame.setPalette(&palette);
```

## Overrides and variants

- Define overrides with `VISAGE_THEME_PALETTE_OVERRIDE(Name)` and supply per-override palette entries.
- Apply to a subtree: `frame.setPaletteOverride(override_id, recursive)` (set recursive=false for local-only).
- Common uses: hover/active/disabled states, light/dark branches, contextual panels.

### Example: scoped override for hover state

```cpp
VISAGE_THEME_PALETTE_OVERRIDE(Hover);

visage::Palette base = makeDarkPalette();
visage::Palette hover = base;
hover.setColor(visage::theme::ColorId::nextId("Accent", __FILE__, 0xff66d9ef), 0xff88e0ff);

button.onMouseEnter() += [&] { button.setPaletteOverride(hover.id(), false); };
button.onMouseExit()  += [&] { button.setPaletteOverride(visage::theme::OverrideId::kDefaultId, false); };
```

## Consumption in draw code

- Always use `paletteColor` / `paletteValue` instead of literals to respect themes/overrides.
- Widgets already use palette lookups; custom frames should do the same.

## Tips

- Keep IDs minimal and meaningful; group them by feature/file to keep docs tidy.
- Provide palette factories for your main variants (light/dark/high-contrast) and attach early in app startup.
- Use overrides for stateful tweaks (hover/active), not entirely new themes.
