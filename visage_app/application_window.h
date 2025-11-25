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

#include "application_editor.h"

#include <visage_windowing/windowing.h>

namespace visage {

  /// Application entry point for creating and managing a top-level native window.
  ///
  /// Provides helpers to configure window decoration, title, sizing, parenting,
  /// visibility, and the run loop. Rendering and input handling are inherited
  /// from ApplicationEditor and Frame.
  class ApplicationWindow : public ApplicationEditor {
  public:
    /// Creates an unattached window. Call one of the `show` overloads to present.
    ApplicationWindow();
    ~ApplicationWindow() override;

    /// Current window title.
    const std::string& title() const { return title_; }
    /// Sets the native window title if the window is showing.
    void setTitle(std::string title);

    /// True when the window floats above other apps (where supported).
    bool isAlwaysOnTop() const { return always_on_top_; }
    /// Requests always-on-top mode (where supported).
    void setWindowOnTop(bool on_top);

    /// Choose window decoration before showing (native, client chrome, or popup).
    void setWindowDecoration(Window::Decoration decoration) {
      decoration_ = decoration;
      if (decoration_ == Window::Decoration::Client)
        addClientDecoration();
    }

    /// Sets the backing native pixel dimensions without showing.
    void setNativeWindowDimensions(int width, int height) {
      setNativeBounds(nativeX(), nativeY(), width, height);
      if (window_)
        window_->setNativeWindowSize(width, height);
    }

    /// Sets logical window dimensions using Dimension (px, percent, or native).
    void setWindowDimensions(const Dimension& width, const Dimension& height);
    void setWindowDimensions(const Dimension& x, const Dimension& y, const Dimension& width,
                             const Dimension& height);

    /// Shows the window at its last configured dimensions.
    void show();
    /// Shows embedded in a parent window handle.
    void show(void* parent_window);
    /// Shows embedded with specific logical size.
    void show(const Dimension& width, const Dimension& height, void* parent_window);
    /// Shows with logical width/height at default position.
    void show(const Dimension& width, const Dimension& height);
    /// Shows with logical position and size.
    void show(const Dimension& x, const Dimension& y, const Dimension& width, const Dimension& height);
    /// Shows maximized (using a default percent of display as unmaximized size).
    void showMaximized();
    /// Hides the window if showing.
    void hide();
    /// Closes and releases the native window.
    void close();
    /// True if the native window currently exists and is visible.
    bool isShowing() const;

    /// Callback when the window becomes visible.
    auto& onShow() { return on_show_; }
    /// Callback when the window is hidden or closed.
    auto& onHide() { return on_hide_; }
    /// Callback when the window's drawable contents are resized.
    auto& onWindowContentsResized() { return on_window_contents_resized_; }

    /// Runs the platform event loop until the window closes.
    void runEventLoop();

  private:
    void registerCallbacks();
    void showWindow(bool maximized);

    IPoint initial_position_;
    Window::Decoration decoration_ = Window::Decoration::Native;
    CallbackList<void()> on_show_;
    CallbackList<void()> on_hide_;
    CallbackList<void()> on_window_contents_resized_;
    std::string title_;
    bool always_on_top_ = false;
    std::unique_ptr<Window> window_;
  };
}
