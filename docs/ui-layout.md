# UI and Layout

Related guides: [Graphics](graphics), [Paths](paths), [Shaders](shaders), [Animation](animation), [Theming](theming), [Windowing](windowing), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Frame tree

Headers to include: `#include <visage_ui/frame.h>`, `#include <visage_ui/events.h>` for event structs, and palette/theme headers (`<visage_graphics/palette.h>`, `<visage_graphics/theme.h>`) when styling.

- `Frame` (`visage_ui/frame.h`) is the base UI node: bounds, visibility, drawing, input, focus, DPI, and hierarchy.
- Lifecycle hooks: `init`, `draw`, `destroy`, `resized`, `dpiChanged`, `visibilityChanged`, `hierarchyChanged`, `focusChanged`.
- Event callbacks: mouse enter/exit/down/up/move/drag, wheel, key press/release, text input, drag-and-drop, hit testing.
- Dirty rendering: frames request redraw; `ApplicationEditor` collects dirty frames and repaints regions.
- Focus and keyboard: `requestKeyboardFocus`, text input gating, tab focus helpers.

### Minimal custom frame

Headers to include: `#include <visage_ui/frame.h>`, `#include <visage_graphics/font.h>`, `#include <visage_graphics/fonts.h>` (generated).

```cpp
class Card : public visage::Frame {
public:
  Card() { setAcceptsKeystrokes(true); }

  void init() override {
    setBounds(20, 20, 320, 180);
  }

  void draw(visage::Canvas& c) override {
    c.setColor(0xff1e1f2b);
    c.roundedRectangle(0, 0, width(), height(), 12);
    c.setColor(0xffffffff);
    visage::Font font(16, visage::fonts::Lato_Regular_ttf, dpiScale());
    c.text(font, "Hello", 16, 32);
  }

  bool keyPress(const visage::KeyEvent& e) override {
    if (e.keyCode() == visage::KeyCode::Escape) { setVisible(false); return true; }
    return false;
  }
};
```

## Layout

- Flex-inspired layout in `visage_ui/layout.h` with grow/shrink, gaps, wrap, alignment, margins, and padding.
- Dimensions use `visage_utils/dimension.h` to express logical pixels, native pixels, and percentages of parent bounds.
- `computeLayout` populates child bounds relative to parents; bounding boxes available for measurement.

### Dimension helpers

Headers to include: `#include <visage_utils/dimension.h>` and optionally `using namespace visage::dimension;` for literals.

- Logical pixels: `Dimension::logicalPixels(12)` or `12_px` literal (scales with DPI).
- Native pixels: `Dimension::nativePixels(12)` or `12_npx` literal (no DPI scaling).
- Percent: `Dimension::widthPercent(30)` / `Dimension::heightPercent(50)` relative to parent.
- View min/max: `Dimension::viewMinPercent(70)` / `viewMaxPercent(...)` for responsive sizing based on the smaller/larger viewport edge (used in examples).

### Flex example

Headers to include: `#include <visage_ui/layout.h>`, `#include <visage_utils/dimension.h>`

```cpp
using namespace visage::dimension;

root.layout().setFlex(true);
root.layout().setFlexGap(8_px);
root.layout().setPadding(12_px);

for (auto* child : root.children()) {
  child->layout().setFlexGrow(1.0f);
  child->layout().setMargin(4_px);
}

root.computeLayout();
```

### Flex options (key setters on `Layout`)

- `setFlex(true)`: enables flex layout. Direction defaults to rows; `setFlexRows(false)` switches to columns.
- `setFlexGrow(float)`: how much free space a child consumes relative to siblings.
- `setFlexShrink(float)`: how much a child shrinks when space is tight.
- `setFlexGap(Dimension)`: gap between items.
- `setFlexWrap(bool)` / `setFlexWrapReverse(bool)`: allow wrapping to new rows/columns.
- `setFlexReverseDirection(bool)`: reverse item order.
- `setFlexItemAlignment(ItemAlignment)`: cross-axis alignment for children (Stretch/Start/Center/End).
- `setFlexSelfAlignment(ItemAlignment)`: per-child override of cross-axis alignment.
- `setFlexWrapAlignment(WrapAlignment)`: how wrapped lines are spaced (Start/Center/End/Stretch/SpaceBetween/SpaceAround/SpaceEvenly).
- `setPadding*` / `setMargin*`: inset or space around children.
- `setDimensions(width, height)`: force a fixed size for a child instead of flex sizing.

### Wrapped grid example

```cpp
using namespace visage::dimension;

auto& layout = grid.layout();
layout.setFlex(true);
layout.setFlexWrap(true);
layout.setFlexGap(10_px);
layout.setFlexWrapAlignment(visage::Layout::WrapAlignment::SpaceEvenly);

for (auto* tile : grid.children()) {
  tile->layout().setWidth(visage::Dimension::widthPercent(30)); // approx 3 per row
  tile->layout().setHeight(120_px);
  tile->layout().setFlexGrow(1.0f); // allow stretching with available space
}

grid.computeLayout();
```

## Input model

- Normalized events in `visage_ui/events.h`: `MouseEvent`/`KeyEvent` utilities, modifiers, click repeat detection, popup trigger helpers.
- Drag-and-drop file support; per-frame opt-in for receiving or sourcing drags.
- Timers via `EventTimer` and `EventManager::runOnEventThread`.

## DPI and scaling

- `Window` supplies `dpiScale`; `Frame::setDpiScale` cascades to children.
- Hit testing and layout operate in logical pixels; conversion helpers handle native coordinates.

## Undo history

- `UndoHistory` (`visage_ui/undo_history.h`) provides scoped undo/redo stacks; widgets can push undoable actions.
