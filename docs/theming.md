# Theming

## Theme IDs

Headers to include: `#include <visage_graphics/theme.h>`

- Theme values are defined in `visage_graphics/theme.h` using `VISAGE_THEME_COLOR` and `VISAGE_THEME_VALUE`.
- Each ID records a name, group, and default color/value; lookup via `theme::ColorId`/`ValueId`.

## Palette

Headers to include: `#include <visage_graphics/palette.h>` and your generated theme definitions if applicable.

- `Palette` (`visage_graphics/palette.h`) stores concrete colors and values for theme IDs.
- Frames and widgets pull colors via `paletteColor`/`paletteValue`; palette overrides allow scoped variants.
- `Frame::setPaletteOverride` switches to alternate palettes (e.g., dark/light) per subtree.

## Overrides

- Theme overrides (`theme::OverrideId`) can redefine colors/values for specific scopes.
- Widgets often expose palette override hooks for hover/active states.

## Guidance

- Keep brand colors and spacing tokens in palette definitions.
- Prefer palette IDs in rendering code instead of hard-coded colors to enable theme switching.

## How palettes are created and used

- **Creation**: Construct a `visage::Palette`, then set colors/values for your theme IDs. You can start from defaults or from an existing palette (`Palette::setColor(id, rgba)`, `setValue(id, float)`).
- **Attachment**: Call `frame.setPalette(&palette)` on the top-most frame you want to style; the palette pointer cascades to children.
- **Overrides**: Use `frame.setPaletteOverride(theme::OverrideId, recursive)` to apply a variant (e.g., hover/active/dark) to a subtree without replacing the base palette.
- **Access**: In draw code, use `paletteColor(theme::ColorId::X)` or `paletteValue(theme::ValueId::Y)` instead of literals; the palette pointer comes from the nearest ancestor.

### Default palette

Visage ships a default palette instantiated internally (see `visage_graphics/palette.cpp`). If you do nothing, widgets will use that palette. To customize globally, create your own `Palette` at app startup and attach it to the root frame.

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

### Example: scoped override for hover state

```cpp
VISAGE_THEME_PALETTE_OVERRIDE(Hover);

visage::Palette p = makeDarkPalette();
visage::Palette hover = p;
hover.setColor(visage::theme::ColorId::nextId("Accent", __FILE__, 0xff66d9ef), 0xff88e0ff);

button.onMouseEnter() += [&] { button.setPaletteOverride(hover.id(), false); };
button.onMouseExit()  += [&] { button.setPaletteOverride(visage::theme::OverrideId::kDefaultId, false); };
```

### Tips

- Keep all drawing using `paletteColor` / `paletteValue` so overrides and themes actually apply.
- Group IDs by file to keep `group` names tidy in generated docs.
- Consider a shared palette factory for light/dark variants and pass the desired instance to the root frame early in app startup.
