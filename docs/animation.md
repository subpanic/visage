# Animation and Timing

Headers to include: `#include <visage_ui/events.h>` (EventTimer/runOnEventThread), `#include <visage_graphics/canvas.h>` (time helpers), and your frame/widget headers.

Related guides: [Graphics](graphics), [Paths](paths), [Shaders](shaders), [Assets/Embedding](assets-embedding), [Theming](theming), [Examples](examples), [Architecture](architecture), [Getting Started](getting-started)

## Ways to drive animation

- **EventTimer** (`visage_ui/events.h`): Start a timer with `startTimer(ms)`; override `timerCallback` and call `redraw()`. Runs on the UI/event thread.
- **Per-frame draw time**: In `ApplicationEditor` the draw callback updates `Canvas::time`, `deltaTime`, `frameCount`. Use `canvas.time()` / `canvas.deltaTime()` inside `onDraw`.
- **Manual triggers**: Call `redraw()` when state changes (e.g., audio level update). This leverages dirty-region rendering.
- **Background graphics thread**: If `VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD` is ON (non-emscripten), rendering can happen on a separate thread; keep shared state thread-safe.
- **Cross-thread updates**: From other threads (e.g., audio), use `runOnEventThread` to post a lambda that updates UI state and calls `redraw()`.

## EventTimer details

- Runs on the UI/event thread. Keep work light and drive rendering via `redraw()` rather than heavy computation in the callback.
- `startTimer(ms)` arms the timer; `stopTimer()` pauses it. `isRunning()` reports status. Timers coalesce with the event loop, so short intervals still respect platform scheduling.
- Override `timerCallback()` or attach to `onTimerCallback()`. Use member state; do not capture `this` in a lambda stored on `EventTimer` itself.
- Dynamic cadence: call `startTimer(new_ms)` from inside `timerCallback` to change intervals (e.g., adaptive throttling).
- Manual tick: `checkTimer(current_time_ms)` lets you feed custom clocks (rarely needed; typically the event loop calls this for you).

Example: start on demand and pause when idle
```cpp
class Meter : public visage::Frame, public visage::EventTimer {
public:
  void setLevel(float level) {
    level_ = level;
    if (!isRunning()) startTimer(33); // ~30fps while active
  }

  void timerCallback() override {
    decay_ *= 0.92f;
    if (decay_ < 0.01f) stopTimer();
    redraw();
  }

  void draw(visage::Canvas& c) override {
    // draw using level_ and decay_
  }
private:
  float level_ = 0.0f;
  float decay_ = 1.0f;
};
```

## Patterns

- **Timer-based** (e.g., 60fps):  
  ```cpp
  class Spinner : public visage::Frame, public visage::EventTimer {
  public:
    Spinner() { startTimer(16); }
    void timerCallback() override { angle_ += 0.05f; redraw(); }
    void draw(visage::Canvas& c) override { /* draw using angle_ */ }
  private: float angle_ = 0.0f;
  };
  ```
- **State-driven** (only when data changes): call `redraw()` when new data arrives; avoid a timer to minimize work.
- **Per-frame time** (based on `canvas.time()`): drive animation purely on elapsed time inside `onDraw`, no timer needed; redraw continuously (e.g., via a timer or continuous invalidation).

## Best practices

- Only `redraw()` when necessary; keep dirty regions tight by limiting drawing to your frame’s bounds.
- For cross-thread data, push updates via `runOnEventThread` to avoid race conditions.
- Respect DPI (`dpiScale`) and logical vs native pixels if you animate sub-pixel values.
- If you need fixed-step simulation, accumulate `deltaTime` and step in fixed increments rather than variable per-frame deltas.
- For shader-driven animations, prefer uniforms based on `canvas.time()` and keep CPU work minimal; see {doc}`shaders` and the ShaderPostEffect examples.
