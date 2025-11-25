# Theming

## Theme IDs

- Theme values are defined in `visage_graphics/theme.h` using `VISAGE_THEME_COLOR` and `VISAGE_THEME_VALUE`.
- Each ID records a name, group, and default color/value; lookup via `theme::ColorId`/`ValueId`.

## Palette

- `Palette` (`visage_graphics/palette.h`) stores concrete colors and values for theme IDs.
- Frames and widgets pull colors via `paletteColor`/`paletteValue`; palette overrides allow scoped variants.
- `Frame::setPaletteOverride` switches to alternate palettes (e.g., dark/light) per subtree.

## Overrides

- Theme overrides (`theme::OverrideId`) can redefine colors/values for specific scopes.
- Widgets often expose palette override hooks for hover/active states.

## Guidance

- Keep brand colors and spacing tokens in palette definitions.
- Prefer palette IDs in rendering code instead of hard-coded colors to enable theme switching.
