/* Copyright Vital Audio, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "visage_ui/frame.h"

namespace visage {
  class ApplicationEditor;
  class Canvas;
  class Window;
  class WindowEventHandler;
  class ClientWindowDecoration;

  /// Root frame that wraps the application's content and optional client decoration.
  class TopLevelFrame : public Frame {
  public:
    explicit TopLevelFrame(ApplicationEditor* editor);
    ~TopLevelFrame() override;

    /// Updates DPI and bounds when resized; adjusts client decoration if present.
    void resized() override;
    /// Adds client-side title bar/buttons when using client decoration.
    void addClientDecoration();
    bool hasClientDecoration() const { return client_decoration_ != nullptr; }

  private:
    ApplicationEditor* editor_ = nullptr;
    std::unique_ptr<ClientWindowDecoration> client_decoration_;
  };

  /// Base class that manages a Canvas, window attachment, and event bridge.
  ///
  /// ApplicationWindow inherits this to provide a windowed experience; the same
  /// class can render windowless surfaces (e.g., for headless rendering or plugins).
  class ApplicationEditor : public Frame {
  public:
    static constexpr int kDefaultClientTitleBarHeight = 30;

    ApplicationEditor();
    ~ApplicationEditor() override;

    /// Captures the current frame buffer contents.
    const Screenshot& takeScreenshot();

    /// Applies native size and DPI to the Canvas after window changes.
    void setCanvasDetails();

    /// Attach the editor to a Window, set callbacks, and prepare the Canvas.
    void addToWindow(Window* window);
    /// Render into an offscreen surface of the given size.
    void setWindowless(int width, int height);
    /// Detach from the Window and release swap chain.
    void removeFromWindow();
    /// Draws all dirty frames and submits the Canvas.
    void drawWindow();

    bool isFixedAspectRatio() const { return fixed_aspect_ratio_ != 0.0f; }
    /// Enable or disable fixed aspect ratio based on current bounds.
    void setFixedAspectRatio(bool fixed);
    float aspectRatio() const override {
      if (height() && width())
        return width() * 1.0f / height();
      return 1.0f;
    }

    Window* window() const { return window_; }

    void drawStaleChildren();

    void setMinimumDimensions(float width, float height) {
      min_width_ = std::max(0.0f, width);
      min_height_ = std::max(0.0f, height);
    }

    void checkFixedAspectRatio() {
      if (fixed_aspect_ratio_ && width() && height())
        fixed_aspect_ratio_ = aspectRatio();
    }

    /// Enforces minimum size and aspect constraints on native resize.
    void adjustWindowDimensions(int* width, int* height, bool horizontal_resize, bool vertical_resize) const;

    void adjustWindowDimensions(uint32_t* width, uint32_t* height, bool horizontal_resize,
                                bool vertical_resize) const {
      int w = *width;
      int h = *height;
      adjustWindowDimensions(&w, &h, horizontal_resize, vertical_resize);
      *width = w;
      *height = h;
    }

    /// Adds client decoration UI if supported on the current platform.
    void addClientDecoration() { top_level_->addClientDecoration(); }
    HitTestResult hitTest(const Point& position) const override {
      if (position.y < kDefaultClientTitleBarHeight && top_level_->hasClientDecoration())
        return HitTestResult::TitleBar;

      return HitTestResult::Client;
    }

  private:
    Window* window_ = nullptr;
    FrameEventHandler event_handler_;
    std::unique_ptr<Canvas> canvas_;
    std::unique_ptr<TopLevelFrame> top_level_;
    std::unique_ptr<WindowEventHandler> window_event_handler_;
    float fixed_aspect_ratio_ = 0.0f;

    float min_width_ = 0.0f;
    float min_height_ = 0.0f;
    std::vector<Frame*> stale_children_;
    std::vector<Frame*> drawing_children_;

    VISAGE_LEAK_CHECKER(ApplicationEditor)
  };
}
