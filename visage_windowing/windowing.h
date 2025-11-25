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

#include "visage_utils/defines.h"
#include "visage_utils/dimension.h"
#include "visage_utils/events.h"
#include "visage_utils/space.h"

#include <climits>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace visage {
  /// Cross-platform native window abstraction used by Visage to normalize events and drawing.
  class Window {
  public:
    static constexpr float kDefaultDpi = 96.0f;

    enum class Decoration {
      Native,
      Client,
      Popup
    };

    static void setDoubleClickSpeed(int ms) { double_click_speed_ = ms; }
    static int doubleClickSpeed() { return double_click_speed_; }

    /// Interface for receiving normalized input and window events from the platform backend.
    class EventHandler {
    public:
      virtual ~EventHandler() = default;

      virtual HitTestResult handleHitTest(int x, int y) = 0;
      virtual HitTestResult currentHitTest() const = 0;
      virtual void handleMouseMove(int x, int y, int button_state, int modifiers) = 0;
      virtual void handleMouseDown(MouseButton button_id, int x, int y, int button_state,
                                   int modifiers, int repeat_clicks) = 0;
      virtual void handleMouseUp(MouseButton button_id, int x, int y, int button_state,
                                 int modifiers, int repeat_clicks) = 0;
      virtual void handleMouseEnter(int x, int y) = 0;
      virtual void handleMouseLeave(int last_x, int last_y, int button_state, int modifiers) = 0;
      virtual void handleMouseWheel(float delta_x, float delta_y, float precise_x, float precise_y,
                                    int mouse_x, int mouse_y, int button_state, int modifiers,
                                    bool momentum) = 0;

      virtual bool handleKeyDown(KeyCode key_code, int modifiers, bool repeat) = 0;
      virtual bool handleKeyUp(KeyCode key_code, int modifiers) = 0;

      virtual bool handleTextInput(const std::string& text) = 0;
      virtual bool hasActiveTextEntry() = 0;

      virtual void handleFocusLost() = 0;
      virtual void handleFocusGained() = 0;
      virtual void handleAdjustResize(int* width, int* height, bool horizontal_resize,
                                      bool vertical_resize) { }
      virtual void handleResized(int width, int height) = 0;

      virtual bool handleFileDrag(int x, int y, const std::vector<std::string>& files) = 0;
      virtual void handleFileDragLeave() = 0;
      virtual bool handleFileDrop(int x, int y, const std::vector<std::string>& files) = 0;

      virtual bool isDragDropSource() = 0;
      virtual std::string startDragDropSource() = 0;
      virtual void cleanupDragDropSource() = 0;
    };

    Window(const Window&) = delete;

    /// Constructs a hidden window. Use show/showMaximized to present.
    Window();
    /// Constructs a window with an initial client size.
    Window(int width, int height);
    virtual ~Window() = default;

    /// Called when the window becomes visible.
    auto& onShow() { return on_show_; }
    /// Called when the window is hidden or closed.
    auto& onHide() { return on_hide_; }
    /// Called when the drawable client area is resized.
    auto& onWindowContentsResized() { return on_contents_resized_; }

    /// Process OS events until termination.
    virtual void runEventLoop() = 0;
    /// Platform-specific native handle (HWND/NSWindow/etc).
    virtual void* nativeHandle() const = 0;
    /// Notify backend that the client area resized.
    virtual void windowContentsResized(int width, int height) = 0;
    /// Called when the user requests close; return false to veto.
    virtual bool closeRequested() { return true; }

    /// Returns platform-specific handle used to initialize rendering.
    virtual void* initWindow() const { return nullptr; }
    /// Returns a global display/connection object if required by the backend.
    virtual void* globalDisplay() const { return nullptr; }
    /// Handle plugin file descriptor events (POSIX backends).
    virtual void processPluginFdEvents() { }
    /// POSIX file descriptor for event polling (Linux).
    virtual int posixFd() const { return 0; }

    /// Present the window.
    virtual void show() = 0;
    /// Present maximized.
    virtual void showMaximized() = 0;
    /// Hide without destroying.
    virtual void hide() = 0;
    /// Close and destroy.
    virtual void close() = 0;
    /// True if visible.
    virtual bool isShowing() const = 0;
    /// Set native window title.
    virtual void setWindowTitle(const std::string& title) = 0;
    /// Lock aspect ratio to current size.
    virtual void setFixedAspectRatio(bool fixed) { }
    /// Maximum supported client dimensions.
    virtual IPoint maxWindowDimensions() const = 0;
    /// Request always-on-top where supported.
    virtual void setAlwaysOnTop(bool on_top) { }

    void notifyShow() const { on_show_.callback(); }
    void notifyHide() const { on_hide_.callback(); }

    void setDrawCallback(std::function<void(double)> callback) {
      draw_callback_ = std::move(callback);
    }

    void drawCallback(double time) const {
      if (draw_callback_)
        draw_callback_(time);
    }

    bool isVisible() const { return visible_; }

    IPoint lastWindowMousePosition() const { return last_window_mouse_position_; }

    void setWindowSize(int width, int height);
    void setNativeWindowSize(int width, int height);
    void setInternalWindowSize(int width, int height);
    void setDpiScale(float scale) { dpi_scale_ = scale; }
    float dpiScale() const { return dpi_scale_; }

    IPoint convertToNative(const Point& logical_point) const {
      return { static_cast<int>(std::round(logical_point.x * dpi_scale_)),
               static_cast<int>(std::round(logical_point.y * dpi_scale_)) };
    }
    Point convertToLogical(const IPoint& point) const {
      return { point.x / dpi_scale_, point.y / dpi_scale_ };
    }

    void setMouseRelativeMode(bool relative) { mouse_relative_mode_ = relative; }
    virtual bool mouseRelativeMode() const { return mouse_relative_mode_; }

    int clientWidth() const { return client_width_; }
    int clientHeight() const { return client_height_; }
    void setEventHandler(EventHandler* event_handler) { event_handler_ = event_handler; }
    void clearEventHandler() { event_handler_ = nullptr; }

    bool hasActiveTextEntry() const;

    HitTestResult handleHitTest(int x, int y);
    HitTestResult currentHitTest() const;
    void handleMouseMove(int x, int y, int button_state, int modifiers);
    void handleMouseDown(MouseButton button_id, int x, int y, int button_state, int modifiers);
    void handleMouseUp(MouseButton button_id, int x, int y, int button_state, int modifiers);
    void handleMouseEnter(int x, int y);
    void handleMouseLeave(int button_state, int modifiers);

    void handleMouseWheel(float delta_x, float delta_y, float precise_x, float precise_y, int x,
                          int y, int button_state, int modifiers, bool momentum = false);
    void handleMouseWheel(float delta_x, float delta_y, int x, int y, int button_state,
                          int modifiers, bool momentum = false);

    void handleFocusLost();
    void handleFocusGained();
    void handleResized(int width, int height);
    void handleAdjustResize(int* width, int* height, bool horizontal_resize, bool vertical_resize);

    bool handleKeyDown(KeyCode key_code, int modifiers, bool repeat);
    bool handleKeyUp(KeyCode key_code, int modifiers);
    bool handleTextInput(const std::string& text);

    bool handleFileDrag(int x, int y, const std::vector<std::string>& files);
    void handleFileDragLeave();
    bool handleFileDrop(int x, int y, const std::vector<std::string>& files);

    bool isDragDropSource() const;
    std::string startDragDropSource();
    void cleanupDragDropSource();
    void setVisible(bool visible) { visible_ = visible; }

  private:
    static int double_click_speed_;

    struct RepeatClick {
      int click_count = 0;
      long long last_click_ms = 0;
    };

    EventHandler* event_handler_ = nullptr;
    IPoint last_window_mouse_position_ = { 0, 0 };
    RepeatClick mouse_repeat_clicks_;

    std::function<void(double)> draw_callback_ = nullptr;
    CallbackList<void()> on_show_;
    CallbackList<void()> on_hide_;
    CallbackList<void()> on_contents_resized_;
    float dpi_scale_ = 1.0f;
    bool visible_ = true;
    bool mouse_relative_mode_ = false;
    int client_width_ = 0;
    int client_height_ = 0;

    VISAGE_LEAK_CHECKER(Window)
  };

  /// Cursor utilities.
  void setCursorStyle(MouseCursor style);
  void setCursorVisible(bool visible);
  Point cursorPosition();
  void setCursorPosition(Point window_position);
  void setCursorScreenPosition(Point screen_position);

  /// Platform feature helpers.
  bool isMobileDevice();
  void showMessageBox(std::string title, std::string message);

  /// Clipboard helpers.
  std::string readClipboardText();
  void setClipboardText(const std::string& text);

  int doubleClickSpeed();
  void setDoubleClickSpeed(int ms);

  /// Platform default DPI scale.
  float defaultDpiScale();

  /// Compute native bounds from logical dimensions and DPI.
  IBounds computeWindowBounds(const Dimension& x, const Dimension& y, const Dimension& width,
                              const Dimension& height);

  /// Create a window with logical position/size and a decoration style.
  std::unique_ptr<Window> createWindow(const Dimension& x, const Dimension& y,
                                       const Dimension& width, const Dimension& height,
                                       Window::Decoration decoration_style = Window::Decoration::Native);
  /// Create a window embedded into a host parent window.
  std::unique_ptr<Window> createPluginWindow(const Dimension& width, const Dimension& height,
                                             void* parent_handle);

  inline std::unique_ptr<Window> createWindow(const Dimension& width, const Dimension& height,
                                              Window::Decoration decoration_style = Window::Decoration::Native) {
    return createWindow({}, {}, width, height, decoration_style);
  }

  inline IBounds computeWindowBounds(const Dimension& width, const Dimension& height) {
    return computeWindowBounds({}, {}, width, height);
  }

  void closeApplication();
}
