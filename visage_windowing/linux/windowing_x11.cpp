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

#if VISAGE_LINUX
#include "windowing_x11.h"

#include "visage_utils/thread_utils.h"

#include <cstring>
#include <sstream>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xutil.h>

namespace visage {
  static std::string _clipboard_text;

  class NativeWindowLookup {
  public:
    static NativeWindowLookup& instance() {
      static NativeWindowLookup instance;
      return instance;
    }

    void addWindow(WindowX11* window) { native_window_lookup_[window->nativeHandle()] = window; }

    void removeWindow(WindowX11* window) {
      if (native_window_lookup_.count(window->nativeHandle()))
        native_window_lookup_.erase(window->nativeHandle());
    }

    bool anyWindowOpen() const {
      for (auto& window : native_window_lookup_) {
        if (window.second->isShowing())
          return true;
      }
      return false;
    }

    void closeAll() const {
      auto windows = native_window_lookup_;
      for (auto& window : windows)
        window.second->close();
    }

    // Device changes are reported per window and carry no window identity, and each window on the
    // connection keeps its own cached device state, so every window of this connection is reset.
    void clearInputDeviceStates(X11Connection* x11) const {
      for (auto& window : native_window_lookup_) {
        if (window.second->x11Connection() == x11)
          window.second->clearInputDeviceStates();
      }
    }

    WindowX11* findWindow(::Window handle) {
      auto it = native_window_lookup_.find((void*)handle);
      return it != native_window_lookup_.end() ? it->second : nullptr;
    }

  private:
    NativeWindowLookup() = default;
    ~NativeWindowLookup() = default;

    std::map<void*, WindowX11*> native_window_lookup_;
  };

  class SharedMessageWindow {
  public:
    static ::Window handle() {
      static SharedMessageWindow window;
      return window.window_handle_;
    }

  private:
    SharedMessageWindow() {
      X11Connection* x11 = X11Connection::globalInstance();
      ::Display* display = x11->display();
      window_handle_ = XCreateSimpleWindow(display, x11->rootWindow(), -100, -100, 1, 1, 0, 0, 0);
      XSelectInput(display, window_handle_, StructureNotifyMask);
      XFlush(display);
    }

    ~SharedMessageWindow() {
      XDestroyWindow(X11Connection::globalInstance()->display(), window_handle_);
    }

    ::Window window_handle_ = 0;
  };

  std::string readClipboardText() {
    static constexpr int kSleepWait = 5;
    static constexpr int kMaxWaitTime = 250;
    static constexpr int kTries = kMaxWaitTime / kSleepWait;
    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);

    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return "";

    Display* display = x11->display();

    ::Window selection_owner = XGetSelectionOwner(display, x11->clipboard());
    if (selection_owner == SharedMessageWindow::handle())
      return _clipboard_text;

    Atom selection_property = XInternAtom(display, "VISAGE_SELECT", False);
    XConvertSelection(display, x11->clipboard(), x11->utf8String(), selection_property,
                      SharedMessageWindow::handle(), CurrentTime);

    XEvent event;
    for (int i = 0; i < kTries; ++i) {
      if (XCheckTypedWindowEvent(display, SharedMessageWindow::handle(), SelectionNotify, &event)) {
        if (event.xselection.property == selection_property) {
          Atom actual_type;
          int actual_format = 0;
          unsigned long num_items = 0, bytes_after = 0;
          unsigned char* property = nullptr;
          XGetWindowProperty(display, SharedMessageWindow::handle(), selection_property, 0, ~0,
                             False, AnyPropertyType, &actual_type, &actual_format, &num_items,
                             &bytes_after, &property);

          if (actual_type == x11->utf8String()) {
            if (num_items && property[num_items - 1] == 0)
              num_items--;

            std::string result(reinterpret_cast<char*>(property), num_items);
            XFree(property);
            return result;
          }
          XFree(property);
        }
      }
      Thread::sleep(kSleepWait);
    }

    return "";
  }

  void setClipboardText(const std::string& text) {
    _clipboard_text = text;

    X11Connection* x11 = X11Connection::globalInstance();
    XSetSelectionOwner(x11->display(), XA_PRIMARY, SharedMessageWindow::handle(), CurrentTime);
    XSetSelectionOwner(x11->display(), x11->clipboard(), SharedMessageWindow::handle(), CurrentTime);
  }

  void setCursorStyle(MouseCursor style) {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return;

    X11Connection* x11 = window->x11Connection();
    X11Connection::DisplayLock lock(x11);

    Cursor cursor;
    switch (style) {
    case MouseCursor::Invisible: cursor = x11->cursors().no_cursor; break;
    case MouseCursor::Arrow: cursor = x11->cursors().arrow_cursor; break;
    case MouseCursor::IBeam: cursor = x11->cursors().ibeam_cursor; break;
    case MouseCursor::Crosshair: cursor = x11->cursors().crosshair_cursor; break;
    case MouseCursor::Pointing: cursor = x11->cursors().pointing_cursor; break;
    case MouseCursor::HorizontalResize: cursor = x11->cursors().horizontal_resize_cursor; break;
    case MouseCursor::VerticalResize: cursor = x11->cursors().vertical_resize_cursor; break;
    case MouseCursor::TopLeftResize: cursor = x11->cursors().top_left_resize_cursor; break;
    case MouseCursor::TopRightResize: cursor = x11->cursors().top_right_resize_cursor; break;
    case MouseCursor::BottomLeftResize: cursor = x11->cursors().bottom_left_resize_cursor; break;
    case MouseCursor::BottomRightResize: cursor = x11->cursors().bottom_right_resize_cursor; break;
    case MouseCursor::Dragging:
    case MouseCursor::MultiDirectionalResize:
      cursor = x11->cursors().multi_directional_resize_cursor;
      break;
    default: return;
    }

    XDefineCursor(x11->display(), (::Window)window->nativeHandle(), cursor);
    XFlush(x11->display());
  }

  static MouseCursor windowResizeCursor(int operation) {
    switch (operation) {
    case kResizeLeft | kResizeTop: return MouseCursor::TopLeftResize;
    case kResizeRight | kResizeTop: return MouseCursor::TopRightResize;
    case kResizeLeft | kResizeBottom: return MouseCursor::BottomLeftResize;
    case kResizeRight | kResizeBottom: return MouseCursor::BottomRightResize;
    case kResizeLeft:
    case kResizeRight: return MouseCursor::HorizontalResize;
    case kResizeTop:
    case kResizeBottom: return MouseCursor::VerticalResize;
    default: return MouseCursor::Arrow;
    }
  }

  void setCursorVisible(bool visible) {
    if (visible)
      setCursorStyle(MouseCursor::Arrow);
    else
      setCursorStyle(MouseCursor::Invisible);
  }

  Point cursorPosition() {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return { 0, 0 };

    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);
    ::Window root_return, child_return;
    int root_x = 0, root_y = 0;
    int win_x = 0, win_y = 0;
    unsigned int mask_return = 0;

    XQueryPointer(x11->display(), (::Window)window->nativeHandle(), &root_return, &child_return,
                  &root_x, &root_y, &win_x, &win_y, &mask_return);
    return window->convertToLogical(IPoint(win_x, win_y));
  }

  static void setNativeCursorPosition(IPoint native_position) {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return;

    X11Connection* x11 = window->x11Connection();
    X11Connection::DisplayLock lock(x11);
    XWarpPointer(x11->display(), None, (::Window)window->nativeHandle(), 0, 0, 0, 0,
                 native_position.x, native_position.y);
    XFlush(x11->display());
  }

  void setCursorPosition(Point window_position) {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return;

    setNativeCursorPosition(window->convertToNative(window_position));
  }

  void setCursorScreenPosition(Point window_position) {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return;

    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);

    IPoint position = window->convertToNative(window_position);
    XWarpPointer(x11->display(), None, x11->rootWindow(), 0, 0, 0, 0, position.x, position.y);
    XFlush(x11->display());
  }

  static IPoint nativeCursorScreenPosition() {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window == nullptr)
      return { 0, 0 };

    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);

    ::Window root_return, child_return;
    int root_x = 0, root_y = 0;
    int win_x = 0, win_y = 0;
    unsigned int mask_return = 0;

    XQueryPointer(x11->display(), x11->rootWindow(), &root_return, &child_return, &root_x, &root_y,
                  &win_x, &win_y, &mask_return);
    return { root_x, root_y };
  }

  bool isMobileDevice() {
    return false;
  }

  static double refreshRate(XRRScreenResources* screen_resources, XRRCrtcInfo* info) {
    for (int i = 0; i < screen_resources->nmode; ++i) {
      if (screen_resources->modes[i].id == info->mode)
        return screen_resources->modes[i].dotClock * 1.0 /
               (screen_resources->modes[i].hTotal * screen_resources->modes[i].vTotal);
    }
    return MonitorInfo::kDefaultRefreshRate;
  }

  static MonitorInfo monitorInfoForPosition(IPoint point) {
    static constexpr float kInchToMm = 25.4f;
    static constexpr float kDpiAdjustment = 4.0f / Window::kDefaultDpi;

    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);
    Display* display = x11->display();

    int default_screen = DefaultScreen(display);
    IBounds default_bounds(0, 0, DisplayWidth(display, default_screen),
                           DisplayHeight(display, default_screen));
    MonitorInfo result;
    result.bounds = default_bounds;

    XRRScreenResources* screen_resources = XRRGetScreenResources(display, x11->rootWindow());
    if (screen_resources == nullptr)
      return result;

    for (int i = 0; i < screen_resources->noutput; ++i) {
      XRROutputInfo* output_info = XRRGetOutputInfo(display, screen_resources,
                                                    screen_resources->outputs[i]);
      if (output_info == nullptr)
        continue;

      if (output_info->crtc && output_info->connection == RR_Connected) {
        XRRCrtcInfo* info = XRRGetCrtcInfo(display, screen_resources, output_info->crtc);

        IBounds bounds(info->x, info->y, info->width, info->height);
        if (result.bounds.width() == 0 || bounds.contains(point)) {
          result.bounds = bounds;
          if (output_info->mm_width > 0 && output_info->mm_height > 0)
            result.dpi = info->width * kInchToMm / output_info->mm_width;
          else
            result.dpi = DisplayWidth(display, default_screen) * kInchToMm /
                         DisplayWidthMM(display, default_screen);

          result.dpi = std::round(result.dpi * kDpiAdjustment) / kDpiAdjustment;
          result.refresh_rate = refreshRate(screen_resources, info);
        }
        XRRFreeCrtcInfo(info);
      }
      XRRFreeOutputInfo(output_info);
    }

    XRRFreeScreenResources(screen_resources);
    return result;
  }

  static MonitorInfo activeMonitorInfo() {
    return monitorInfoForPosition(nativeCursorScreenPosition());
  }

  static void drawMessageBox(int width, int height, Display* display, ::Window window, GC gc,
                             const std::string& message) {
    XClearWindow(display, window);

    XFontStruct* font = XLoadQueryFont(display, "-misc-fixed-medium-r-*-*-24-*-*-*-*-*-*-*");
    if (!font) {
      VISAGE_LOG("Unable to load font");
      return;
    }

    int button_width = width / 2;
    int button_height = height / 8;
    int button_x = (width - button_width) / 2;
    int button_y = height - 2 * button_height;

    XSetFont(display, gc, font->fid);
    int text_width = XTextWidth(font, message.c_str(), message.length());
    XDrawString(display, window, gc, (width - text_width) / 2, button_y / 2 + 12, message.c_str(),
                message.size());

    XDrawRectangle(display, window, gc, button_x, button_y, button_width, button_height);
    int ok_width = XTextWidth(font, "OK", 2);
    XDrawString(display, window, gc, (width - ok_width) / 2, button_y + (button_height + 24) / 2, "OK", 2);

    XUnloadFont(display, font->fid);
  }

  void showMessageBox(std::string title, std::string message) {
    IBounds bounds = computeWindowBounds(Dimension::viewMinPercent(30.0f),
                                         Dimension::viewMinPercent(20.0f));
    X11Connection* x11 = X11Connection::globalInstance();
    X11Connection::DisplayLock lock(x11);
    Display* display = x11->display();
    int screen = DefaultScreen(display);

    ::Window message_window = XCreateSimpleWindow(display, x11->rootWindow(), bounds.x(),
                                                  bounds.y(), bounds.width(), bounds.height(), 0,
                                                  BlackPixel(display, screen),
                                                  WhitePixel(display, screen));
    XStoreName(display, message_window, title.c_str());
    Atom wm_state = XInternAtom(display, "_NET_WM_STATE", False);
    Atom wm_state_above = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    XChangeProperty(display, message_window, wm_state, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)&wm_state_above, 1);

    XSizeHints* size_hints = XAllocSizeHints();
    size_hints->flags = PMinSize | PMaxSize | USPosition | USSize | PPosition;
    size_hints->min_width = size_hints->max_width = bounds.width();
    size_hints->min_height = size_hints->max_height = bounds.height();
    size_hints->x = bounds.x();
    size_hints->y = bounds.y();
    XSetWMNormalHints(display, message_window, size_hints);
    XFree(size_hints);

    XSelectInput(display, message_window, ExposureMask | ButtonPressMask | KeyPressMask);
    GC gc = XCreateGC(display, message_window, 0, nullptr);
    Atom wm_delete_message = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, message_window, &wm_delete_message, 1);

    XMapWindow(display, message_window);
    XFlush(display);
    XEvent event;
    while (true) {
      XNextEvent(display, &event);
      if (event.xany.window != message_window)
        continue;

      if (event.type == Expose)
        drawMessageBox(bounds.width(), bounds.height(), display, message_window, gc, message);

      else if (event.type == ButtonPress) {
        int button_width = bounds.width() / 2;
        int button_height = bounds.height() / 8;
        int button_x = (bounds.width() - button_width) / 2;
        int button_y = bounds.height() - 2 * button_height;

        if (event.xbutton.x >= button_x && event.xbutton.x <= button_x + button_width &&
            event.xbutton.y >= button_y && event.xbutton.y <= button_y + button_height) {
          break;
        }
      }
      else if (event.type == KeyPress || event.type == DestroyNotify)
        break;
      else if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wm_delete_message)
          break;
      }
    }

    XFreeGC(display, gc);
    XDestroyWindow(display, message_window);
  }

  float defaultDpiScale() {
    MonitorInfo monitor_info = activeMonitorInfo();
    return monitor_info.dpi / Window::kDefaultDpi;
  }

  IBounds computeWindowBounds(const Dimension& x, const Dimension& y, const Dimension& width,
                              const Dimension& height) {
    MonitorInfo monitor_info = activeMonitorInfo();
    int monitor_width = monitor_info.bounds.width();
    int monitor_height = monitor_info.bounds.height();
    float dpi_scale = monitor_info.dpi / Window::kDefaultDpi;
    int result_w = width.computeInt(dpi_scale, monitor_width, monitor_height, 100);
    int result_h = height.computeInt(dpi_scale, monitor_width, monitor_height, 100);

    int result_x = x.computeInt(monitor_info.dpi / Window::kDefaultDpi, monitor_info.bounds.width(),
                                monitor_info.bounds.height(), (monitor_width - result_w) / 2);
    int result_y = y.computeInt(monitor_info.dpi / Window::kDefaultDpi, monitor_info.bounds.width(),
                                monitor_info.bounds.height(), (monitor_height - result_h) / 2);
    return { monitor_info.bounds.x() + result_x, monitor_info.bounds.y() + result_y, result_w, result_h };
  }

  std::unique_ptr<Window> createWindow(const Dimension& x, const Dimension& y, const Dimension& width,
                                       const Dimension& height, Window::Decoration decoration) {
    IBounds bounds = computeWindowBounds(width, height);
    MonitorInfo monitor_info = activeMonitorInfo();
    int window_x = x.computeInt(monitor_info.dpi / Window::kDefaultDpi, monitor_info.bounds.width(),
                                monitor_info.bounds.height(), bounds.x());
    int window_y = y.computeInt(monitor_info.dpi / Window::kDefaultDpi, monitor_info.bounds.width(),
                                monitor_info.bounds.height(), bounds.y());
    return std::make_unique<WindowX11>(window_x, window_y, bounds.width(), bounds.height(), decoration);
  }

  void* headlessWindowHandle() {
    return nullptr;
  }

  void closeApplication() {
    NativeWindowLookup::instance().closeAll();
  }

  std::unique_ptr<Window> createPluginWindow(const Dimension& width, const Dimension& height,
                                             void* parent_handle) {
    IBounds bounds = computeWindowBounds(width, height);
    return std::make_unique<WindowX11>(bounds.width(), bounds.height(), parent_handle);
  }

  X11Connection::Cursors::Cursors(Display* display) {
    if (display == nullptr)
      return;

    XLockDisplay(display);
    Pixmap blank;
    XColor dummy;
    char data[1] = { 0 };
    blank = XCreateBitmapFromData(display, DefaultRootWindow(display), data, 1, 1);
    no_cursor = XCreatePixmapCursor(display, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(display, blank);

    arrow_cursor = XCreateFontCursor(display, XC_left_ptr);
    ibeam_cursor = XCreateFontCursor(display, XC_xterm);
    crosshair_cursor = XCreateFontCursor(display, XC_crosshair);
    pointing_cursor = XCreateFontCursor(display, XC_hand2);
    horizontal_resize_cursor = XCreateFontCursor(display, XC_sb_h_double_arrow);
    vertical_resize_cursor = XCreateFontCursor(display, XC_sb_v_double_arrow);
    top_left_resize_cursor = XCreateFontCursor(display, XC_top_left_corner);
    top_right_resize_cursor = XCreateFontCursor(display, XC_top_right_corner);
    bottom_left_resize_cursor = XCreateFontCursor(display, XC_bottom_left_corner);
    bottom_right_resize_cursor = XCreateFontCursor(display, XC_bottom_right_corner);
    multi_directional_resize_cursor = XCreateFontCursor(display, XC_fleur);
    XUnlockDisplay(display);
  }

  void X11Connection::initializeXInput2() {
    int opcode = 0;
    int event_base = 0;
    int error_base = 0;
    if (!XQueryExtension(display_, "XInputExtension", &opcode, &event_base, &error_base))
      return;

    // A client has to announce the XI version it supports; the server answers with the newest
    // version it supports up to the requested one. XI 2.4 is the newest version used here, and
    // the request is not limited to what the build headers know so a newer server still reports
    // gesture support.
    int major = XI_2_Major;
    int minor = kGestureMinorVersion;
    if (XIQueryVersion(display_, &major, &minor) != Success || major < XI_2_Major)
      return;

    xinput2_available_ = true;
    xinput2_minor_version_ = minor;
  }

  int displayFps() {
    WindowX11* window = WindowX11::lastActiveWindow();
    if (window)
      return std::round(window->monitorInfo().refresh_rate);
    return MonitorInfo::kDefaultRefreshRate;
  }

  void WindowX11::createWindow(IBounds bounds) {
    VISAGE_ASSERT(bounds.width() && bounds.height());

    ::Display* display = x11_->display();
    int screen = DefaultScreen(display);

    XSetWindowAttributes window_attributes;
    window_attributes.background_pixmap = None;
    window_attributes.background_pixel = None;
    window_attributes.border_pixel = BlackPixel(display, screen);
    window_attributes.backing_store = WhenMapped;
    window_attributes.bit_gravity = NorthWestGravity;
    window_attributes.win_gravity = NorthWestGravity;

    unsigned long attribute_mask = CWBackPixmap | CWBackingPixel | CWBorderPixel | CWBackingStore |
                                   CWBitGravity | CWWinGravity;
    window_handle_ = XCreateWindow(display, x11_->rootWindow(), bounds.x(), bounds.y(),
                                   bounds.width(), bounds.height(), 0, CopyFromParent, InputOutput,
                                   CopyFromParent, attribute_mask, &window_attributes);
    XStoreName(display, window_handle_, VISAGE_APPLICATION_NAME);

    unsigned char blank = 0;
    XChangeProperty(display, window_handle_, x11_->dndActionList(), XA_ATOM, 32, PropModeReplace,
                    x11_->dndActions(), X11Connection::kNumDndActions);
    XChangeProperty(display, window_handle_, x11_->dndActionDescription(), XA_STRING, 8,
                    PropModeReplace, &blank, 0);
    XChangeProperty(display, window_handle_, x11_->dndAware(), XA_ATOM, 32, PropModeReplace,
                    x11_->dndVersion(), 1);
  }

  static bool xinput2IsDeviceChangeEvent(int evtype) {
    return evtype == XI_DeviceChanged || evtype == XI_HierarchyChanged;
  }

  // Window an XInput2 event was delivered to, or None for events that are not bound to a window
  // (device and hierarchy changes).
  static ::Window xinput2EventWindow(const XGenericEventCookie* cookie) {
    if (cookie->evtype == XI_ButtonPress || cookie->evtype == XI_ButtonRelease ||
        cookie->evtype == XI_Motion)
      return ((const XIDeviceEvent*)cookie->data)->event;

    if (cookie->evtype == XI_GesturePinchBegin || cookie->evtype == XI_GesturePinchUpdate ||
        cookie->evtype == XI_GesturePinchEnd)
      return ((const XIGesturePinchEvent*)cookie->data)->event;

    return None;
  }

  void WindowX11::selectXInputEvents() {
    if (!x11_->xinput2Available())
      return;

    // Eight bytes cover every XInput2 event type; bits for events the server does not support
    // stay zero, which the server ignores.
    unsigned char pointer_bits[8] {};
    XIEventMask pointer_mask {};
    pointer_mask.deviceid = XIAllMasterDevices;
    pointer_mask.mask_len = (int)sizeof(pointer_bits);
    pointer_mask.mask = pointer_bits;

    // Selecting master devices keeps pointer delivery identical to core pointer delivery: a slave
    // device sends its own XI2 events in addition to the master's, and only the master pass is
    // requested here.
    XISetMask(pointer_bits, XI_ButtonPress);
    XISetMask(pointer_bits, XI_ButtonRelease);
    XISetMask(pointer_bits, XI_Motion);

    // Device and hierarchy changes describe devices instead of a pointer position, and the server
    // only accepts the hierarchy mask for the "all devices" pseudo device. Pinch gestures are
    // selected for all devices as well: the server resolves gesture ownership against the device
    // that generated the gesture, which is the slave (touchpad) device.
    unsigned char device_bits[8] {};
    XIEventMask device_mask {};
    device_mask.deviceid = XIAllDevices;
    device_mask.mask_len = (int)sizeof(device_bits);
    device_mask.mask = device_bits;

    XISetMask(device_bits, XI_DeviceChanged);
    XISetMask(device_bits, XI_HierarchyChanged);
    if (x11_->xinput2GesturesAvailable()) {
      // The server requires all three pinch events to be selected together.
      XISetMask(device_bits, XI_GesturePinchBegin);
      XISetMask(device_bits, XI_GesturePinchUpdate);
      XISetMask(device_bits, XI_GesturePinchEnd);
    }

    XIEventMask event_masks[] = { pointer_mask, device_mask };
    XISelectEvents(x11_->display(), window_handle_, event_masks, 2);
  }

  // Device state is loaded once per device and window from the device's native XInput2 classes.
  // Trackpads are recognized from device metadata only: dependent (non-direct touch) devices,
  // devices offering XI 2.4 gestures, and devices carrying read-only settings that only exist for
  // touchpad-class devices (libinput tapping, disable-while-typing and click methods, or the
  // synaptics capability set). Direct touch devices and plain pointing devices stay non-trackpad,
  // so wheel scrolling keeps its discrete handling.
  WindowX11::InputDeviceState& WindowX11::inputDeviceState(int device_id) {
    InputDeviceState& state = input_devices_[device_id];
    if (state.queried)
      return state;

    state.queried = true;

    int num_devices = 0;
    X11Connection::DisplayLock lock(x11_);
    XIDeviceInfo* devices = XIQueryDevice(x11_->display(), device_id, &num_devices);
    if (devices == nullptr || num_devices < 1)
      return state;

    loadInputDeviceState(devices[0], state);
    XIFreeDeviceInfo(devices);
    return state;
  }

  void WindowX11::loadInputDeviceState(const XIDeviceInfo& device, InputDeviceState& state) {
    state.scroll_valuators.clear();
    state.smooth_scroll_active = false;
    state.trackpad = false;
    state.queried = true;

    for (int i = 0; i < device.num_classes; ++i) {
      XIAnyClassInfo* class_info = device.classes[i];
      if (class_info == nullptr)
        continue;

      if (class_info->type == XIScrollClass) {
        XIScrollClassInfo* scroll = (XIScrollClassInfo*)class_info;
        ScrollValuator valuator;
        valuator.axis = scroll->number;
        valuator.vertical = scroll->scroll_type == XIScrollTypeVertical;
        // Increments are used as reported, they may legitimately be negative and then also carry
        // the scroll direction of the axis.
        valuator.increment = scroll->increment != 0.0 ? scroll->increment : 1.0;
        state.scroll_valuators.push_back(valuator);
      }
      else if (class_info->type == XITouchClass) {
        XITouchClassInfo* touch = (XITouchClassInfo*)class_info;
        if (touch->mode == XIDependentTouch)
          state.trackpad = true;
      }
      else if (class_info->type == XIGestureClass)
        state.trackpad = true;
    }

    // The valuator classes report the device's current axis position, which is the baseline a
    // smooth scroll delta is measured against. Seeding it here, before the events carrying the
    // deltas arrive, keeps the first scroll from being measured against an absent baseline.
    for (int i = 0; i < device.num_classes; ++i) {
      XIAnyClassInfo* class_info = device.classes[i];
      if (class_info == nullptr || class_info->type != XIValuatorClass)
        continue;

      XIValuatorClassInfo* valuator_info = (XIValuatorClassInfo*)class_info;
      for (ScrollValuator& valuator : state.scroll_valuators) {
        if (valuator.axis != valuator_info->number)
          continue;

        valuator.last_value = valuator_info->value;
        valuator.has_last_value = true;
      }
    }

    if (state.trackpad)
      return;

    int num_properties = 0;
    Atom* properties = XIListProperties(x11_->display(), device.deviceid, &num_properties);
    if (properties == nullptr)
      return;

    for (int i = 0; i < num_properties; ++i) {
      if (x11_->isTouchpadProperty(properties[i])) {
        state.trackpad = true;
        break;
      }
    }
    XFree(properties);
  }

  // Device classes and axis positions change while the pointer is outside the window, so all
  // cached device state is reloaded on pointer entry: the baselines are refreshed to where the
  // device is now, which avoids reporting scrolling that happened over another window while
  // keeping scrolling in this window lossless.
  void WindowX11::reloadInputDeviceStates() {
    if (!x11_->xinput2Available())
      return;

    input_devices_.clear();

    int num_devices = 0;
    X11Connection::DisplayLock lock(x11_);
    XIDeviceInfo* devices = XIQueryDevice(x11_->display(), XIAllDevices, &num_devices);
    if (devices == nullptr)
      return;

    for (int i = 0; i < num_devices; ++i)
      loadInputDeviceState(devices[i], input_devices_[devices[i].deviceid]);
    XIFreeDeviceInfo(devices);
  }

  void WindowX11::pointerScrollForEvent(const XIDeviceEvent* device_event, InputDeviceState& state,
                                        float& delta_x, float& delta_y) {
    delta_x = 0.0f;
    delta_y = 0.0f;

    if (state.scroll_valuators.empty())
      return;

    double horizontal = 0.0;
    double vertical = 0.0;
    int value_index = 0;
    int axis_count = device_event->valuators.mask_len * 8;
    for (int axis = 0; axis < axis_count; ++axis) {
      if (!XIMaskIsSet(device_event->valuators.mask, axis))
        continue;

      // Valuator values of set axes are packed in axis order.
      double value = device_event->valuators.values[value_index++];
      for (ScrollValuator& valuator : state.scroll_valuators) {
        if (valuator.axis != axis)
          continue;

        // Values are the device's accumulated axis position, so scrolling is the difference to
        // the previous event, scaled by the axis increment that equals one legacy scroll button.
        if (valuator.has_last_value) {
          double delta = (value - valuator.last_value) / valuator.increment;
          if (valuator.vertical)
            vertical += delta;
          else
            horizontal += delta;
        }
        valuator.last_value = value;
        valuator.has_last_value = true;
      }
    }

    // Valuators report positive values for scrolling down and right, the wheel handler follows
    // the legacy button convention where scrolling up is positive.
    delta_x = horizontal;
    delta_y = -vertical;
    if (delta_x != 0.0f || delta_y != 0.0f)
      state.smooth_scroll_active = true;
  }

  void WindowX11::processXInput2Motion(XIDeviceEvent* device_event) {
    PointerEvent pointer { (int)device_event->event_x, (int)device_event->event_y,
                           (int)device_event->root_x, (int)device_event->root_y, 0,
                           device_event->time };

    InputDeviceState& state = inputDeviceState(device_event->sourceid);
    float delta_x = 0.0f;
    float delta_y = 0.0f;
    pointerScrollForEvent(device_event, state, delta_x, delta_y);
    if (delta_x != 0.0f || delta_y != 0.0f) {
      handleMouseWheel(delta_x, delta_y, pointer.x, pointer.y, mouseButtonState(), modifierState(),
                       false, state.trackpad);
    }

    // Scroll events carry the unchanged pointer position, so they must not be reported as
    // pointer movement.
    bool pointer_moved = XIMaskIsSet(device_event->valuators.mask, 0) ||
                         XIMaskIsSet(device_event->valuators.mask, 1);
    if (pointer_moved)
      processPointerMotion(pointer);
  }

  void WindowX11::processXInput2Button(XIDeviceEvent* device_event, bool press) {
    PointerEvent pointer { (int)device_event->event_x, (int)device_event->event_y,
                           (int)device_event->root_x, (int)device_event->root_y,
                           device_event->detail, device_event->time };

    if (pointer.button >= 4 && pointer.button <= 7) {
      // Only the press carries the scroll, the matching release is not forwarded either.
      if (!press)
        return;

      InputDeviceState& state = inputDeviceState(device_event->sourceid);

      // Servers emulate the legacy scroll buttons from a device's smooth scroll valuators. Once
      // smooth deltas are being delivered those emulated events repeat the same scrolling, so
      // they are dropped. A device whose deltas could not be measured yet keeps them, which
      // preserves its first step instead of losing it.
      if ((device_event->flags & XIPointerEmulated) && state.smooth_scroll_active)
        return;

      float y = (pointer.button == 4 ? 1.0f : 0.0f) - (pointer.button == 5 ? 1.0f : 0.0f);
      float x = (pointer.button == 7 ? 1.0f : 0.0f) - (pointer.button == 6 ? 1.0f : 0.0f);
      handleMouseWheel(x, y, pointer.x, pointer.y, mouseButtonState(), modifierState(), false,
                       state.trackpad);
      return;
    }

    if (press)
      processPointerButtonPress(pointer);
    else
      processPointerButtonRelease(pointer);
  }

  void WindowX11::processXInput2Pinch(XIGesturePinchEvent* pinch_event) {
    InputDeviceState& state = inputDeviceState(pinch_event->sourceid);
    bool cancelled = (pinch_event->flags & XIGesturePinchEventCancelled) != 0;

    if (pinch_event->evtype == XI_GesturePinchEnd || cancelled) {
      state.pinch_scale = 1.0;
      return;
    }

    // An invalid scale would poison the per update ratio below, so it is dropped.
    if (pinch_event->scale <= 0.0)
      return;

    if (pinch_event->evtype == XI_GesturePinchBegin) {
      state.pinch_scale = pinch_event->scale;
      return;
    }

    // The protocol reports the scale relative to the start of the gesture, while the windowing
    // API takes the magnification of this update only.
    double previous_scale = state.pinch_scale > 0.0 ? state.pinch_scale : 1.0;
    state.pinch_scale = pinch_event->scale;

    float magnification = (float)(pinch_event->scale / previous_scale - 1.0);
    if (magnification != 0.0f)
      handleMouseMagnify(magnification, (int)pinch_event->event_x, (int)pinch_event->event_y,
                         mouseButtonState(), modifierState());
  }

  void WindowX11::processXInput2Event(XEvent& event) {
    XGenericEventCookie* cookie = &event.xcookie;
    switch (cookie->evtype) {
    case XI_ButtonPress: processXInput2Button((XIDeviceEvent*)cookie->data, true); break;
    case XI_ButtonRelease: processXInput2Button((XIDeviceEvent*)cookie->data, false); break;
    case XI_Motion: processXInput2Motion((XIDeviceEvent*)cookie->data); break;
    case XI_GesturePinchBegin:
    case XI_GesturePinchUpdate:
    case XI_GesturePinchEnd: processXInput2Pinch((XIGesturePinchEvent*)cookie->data); break;
    default: break;
    }
  }

  // Generic (extension) events carry their payload in a cookie that has to be claimed with
  // XGetEventData and released with XFreeEventData. The window the event was delivered to is part
  // of that payload, which is why these events are routed before the core event window lookup.
  void WindowX11::processXInput2GenericEvent(XEvent& event) {
    Display* display = x11_->display();
    XGenericEventCookie* cookie = &event.xcookie;
    if (!XGetEventData(display, cookie))
      return;

    if (xinput2IsDeviceChangeEvent(cookie->evtype)) {
      // Device classes can change and a master device switches to the valuators of another slave,
      // so cached scroll valuators and their last values are no longer valid.
      NativeWindowLookup::instance().clearInputDeviceStates(x11_);
    }
    else {
      ::Window event_window = xinput2EventWindow(cookie);
      WindowX11* window = event_window ? NativeWindowLookup::instance().findWindow(event_window) :
                                         nullptr;
      if (window)
        window->processXInput2Event(event);
    }

    XFreeEventData(display, cookie);
  }

  WindowX11* WindowX11::last_active_window_ = nullptr;

  WindowX11::WindowX11(int x, int y, int width, int height, Decoration decoration) :
      Window(width, height), decoration_(decoration) {
    x11_ = X11Connection::globalInstance();

    monitor_info_ = activeMonitorInfo();
    X11Connection::DisplayLock lock(x11_);
    ::Display* display = x11_->display();
    IBounds bounds(x, y, width, height);
    createWindow(bounds);

    if (decoration == Decoration::Popup) {
      XSetWindowAttributes attributes;
      attributes.override_redirect = True;
      XChangeWindowAttributes(display, window_handle_, CWOverrideRedirect, &attributes);
    }
    else if (decoration == Decoration::Client)
      removeWindowDecorationButtons();

    XSizeHints* size_hints = XAllocSizeHints();
    size_hints->flags = USPosition | PMinSize;
    size_hints->x = x;
    size_hints->y = y;
    size_hints->min_width = kMinWidth;
    size_hints->min_height = kMinHeight;
    XSetWMNormalHints(display, window_handle_, size_hints);
    XFree(size_hints);

    XSelectInput(display, window_handle_, kEventMask);
    selectXInputEvents();
    reloadInputDeviceStates();
    start_draw_microseconds_ = time::microseconds();
    setDpiScale(monitor_info_.dpi / kDefaultDpi);
    XFlush(display);
    NativeWindowLookup::instance().addWindow(this);
  }

  static void threadTimerCallback(WindowX11* window) {
    while (window->timerThreadRunning()) {
      Thread::sleep(window->timerMs());

      X11Connection* x11 = window->x11Connection();
      X11Connection::DisplayLock lock(x11);
      ::Window window_handle = (::Window)window->nativeHandle();

      XEvent event;
      memset(&event, 0, sizeof(event));
      event.type = ClientMessage;
      event.xclient.window = window_handle;
      event.xclient.message_type = x11->timerEvent();
      event.xclient.format = 32;
      event.xclient.data.l[0] = 0;

      XSendEvent(x11->display(), window_handle, False, NoEventMask, &event);
      XFlush(x11->display());
    }
  }

  WindowX11::WindowX11(int width, int height, void* parent_handle) : Window(width, height) {
    static constexpr long kEmbedVersion = 0;
    static constexpr long kEmbedMapped = 1;

    plugin_x11_ = std::make_unique<X11Connection>();
    x11_ = plugin_x11_.get();

    monitor_info_ = activeMonitorInfo();
    X11Connection::DisplayLock lock(x11_);
    ::Display* display = x11_->display();

    parent_handle_ = (::Window)parent_handle;
    XSelectInput(display, parent_handle_, StructureNotifyMask);

    createWindow({ 0, 0, width, height });
    Atom atom_embed_info = XInternAtom(display, "_XEMBED_INFO", False);
    long embed_info[2] = { kEmbedVersion, kEmbedMapped };

    XChangeProperty(display, window_handle_, atom_embed_info, atom_embed_info, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(embed_info), 2);

    XChangeProperty(display, parent_handle_, x11_->dndAware(), XA_ATOM, 32, PropModeReplace,
                    x11_->dndVersion(), 1);
    XChangeProperty(display, parent_handle_, x11_->dndProxy(), XA_WINDOW, 32, PropModeReplace,
                    (unsigned char*)&window_handle_, 1);

    // XDND spec: a window referenced by XdndProxy must carry XdndProxy pointing
    // at itself, or sources (e.g. Qt) discard the proxy as stale and deliver
    // to the host's window instead, where the messages never reach us.
    XChangeProperty(display, window_handle_, x11_->dndProxy(), XA_WINDOW, 32, PropModeReplace,
                    (unsigned char*)&window_handle_, 1);
    XReparentWindow(display, window_handle_, parent_handle_, 0, 0);

    XSelectInput(display, window_handle_, kEventMask);
    selectXInputEvents();
    reloadInputDeviceStates();
    XFlush(display);

    parent_configure_recreate_pending_ = true;

    timer_thread_running_ = true;
    timer_thread_ = std::make_unique<std::thread>(threadTimerCallback, this);
    start_draw_microseconds_ = time::microseconds();
    // Embedded plugin windows are always physical pixels: hosts negotiate plugin sizes in
    // physical pixels and X11 has no per-window DPI, so ignore the monitor-derived scale here.
    setDpiScale(1.0f);
    NativeWindowLookup::instance().addWindow(this);
  }

  WindowX11::~WindowX11() {
    NativeWindowLookup::instance().removeWindow(this);

    timer_thread_running_ = false;
    if (timer_thread_ && timer_thread_->joinable())
      timer_thread_->join();

    timer_thread_.reset();

    X11Connection::DisplayLock lock(x11_);
    if (window_handle_)
      XDestroyWindow(x11_->display(), window_handle_);
  }

  int WindowX11::resizeOperationForPosition(int x, int y) const {
    if (decoration_ != Decoration::Client)
      return 0;

    int border = kClientResizeBorder * dpiScale();
    int operation = 0;
    if (x <= border)
      operation = kResizeLeft;
    else if (x >= clientWidth() - border)
      operation = kResizeRight;
    if (y <= border)
      operation |= kResizeTop;
    else if (y >= clientHeight() - border)
      operation |= kResizeBottom;

    return operation;
  }

  void WindowX11::removeWindowDecorationButtons() {
    Atom mwm_hints = XInternAtom(x11_->display(), "_MOTIF_WM_HINTS", False);
    struct MwmHints {
      unsigned long flags = 0;
      unsigned long functions = 0;
      unsigned long decorations = 0;
      long input_mode = 0;
      unsigned long status = 0;
    };

    MwmHints hints;
    hints.flags = 2;
    XChangeProperty(x11_->display(), window_handle_, mwm_hints, mwm_hints, 32, PropModeReplace,
                    reinterpret_cast<unsigned char*>(&hints), 5);
  }

  void* WindowX11::initWindow() const {
    return (void*)SharedMessageWindow::handle();
  }

  void WindowX11::setFixedAspectRatio(bool fixed) {
    Window::setFixedAspectRatio(fixed);
    XSizeHints* size_hints = XAllocSizeHints();
    size_hints->flags = fixed ? PAspect : 0;

    ::Window handle = parent_handle_ ? parent_handle_ : window_handle_;

    long supplied_return;
    XGetWMNormalHints(x11_->display(), handle, size_hints, &supplied_return);
    size_hints->flags = fixed ? (size_hints->flags | PAspect) : (size_hints->flags & ~PAspect);
    size_hints->min_aspect.x = clientWidth();
    size_hints->min_aspect.y = clientHeight();
    size_hints->max_aspect.x = clientWidth();
    size_hints->max_aspect.y = clientHeight();
    XSetWMNormalHints(x11_->display(), handle, size_hints);
    XFree(size_hints);
  }

  IPoint WindowX11::retrieveWindowDimensions() {
    X11Connection::DisplayLock lock(x11_);
    XWindowAttributes attributes;
    XGetWindowAttributes(x11_->display(), window_handle_, &attributes);

    return { attributes.width, attributes.height };
  }

  void WindowX11::passEventToParent(XEvent& event) {
    X11Connection::DisplayLock lock(x11_);

    ::Window root = 0, parent = 0;
    ::Window* children = nullptr;
    unsigned int num_children = 0;
    Display* display = x11_->display();

    if (XQueryTree(display, window_handle_, &root, &parent, &children, &num_children) == 0)
      return;

    if (children)
      XFree(children);

    event.xany.window = parent;
    XSendEvent(display, parent, False, NoEventMask, &event);
    XFlush(display);
  }

  // Buttons this window does not use are handed to the embedding parent as core events, the same
  // way core button events are forwarded.
  void WindowX11::passButtonToParent(bool press, const PointerEvent& pointer) {
    int button_state = mouseButtonState();
    int modifiers = modifierState();

    XEvent event = {};
    event.xbutton.type = press ? ButtonPress : ButtonRelease;
    event.xbutton.display = x11_->display();
    event.xbutton.root = x11_->rootWindow();
    event.xbutton.subwindow = None;
    event.xbutton.time = pointer.time;
    event.xbutton.x = pointer.x;
    event.xbutton.y = pointer.y;
    event.xbutton.x_root = pointer.root_x;
    event.xbutton.y_root = pointer.root_y;
    event.xbutton.same_screen = True;
    event.xbutton.button = pointer.button;
    if (button_state & kMouseButtonLeft)
      event.xbutton.state |= Button1Mask;
    if (button_state & kMouseButtonMiddle)
      event.xbutton.state |= Button2Mask;
    if (button_state & kMouseButtonRight)
      event.xbutton.state |= Button3Mask;
    if (modifiers & kModifierShift)
      event.xbutton.state |= ShiftMask;
    if (modifiers & kModifierRegCtrl)
      event.xbutton.state |= ControlMask;
    if (modifiers & kModifierAlt)
      event.xbutton.state |= Mod1Mask;
    if (modifiers & kModifierMeta)
      event.xbutton.state |= Mod4Mask;

    passEventToParent(event);
  }

  int WindowX11::mouseButtonState() const {
    X11Connection::DisplayLock lock(x11_);

    ::Window root_return = 0, child_return = 0;
    int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
    unsigned int mask_return = 0;

    XQueryPointer(x11_->display(), window_handle_, &root_return, &child_return, &root_x, &root_y,
                  &win_x, &win_y, &mask_return);
    int result = 0;
    if (mask_return & Button1Mask)
      result = result | kMouseButtonLeft;
    if (mask_return & Button2Mask)
      result = result | kMouseButtonMiddle;
    if (mask_return & Button3Mask)
      result = result | kMouseButtonRight;
    return result;
  }

  int WindowX11::modifierState() const {
    X11Connection::DisplayLock lock(x11_);

    ::Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;

    XQueryPointer(x11_->display(), window_handle_, &root_return, &child_return, &root_x, &root_y,
                  &win_x, &win_y, &mask_return);
    int result = 0;
    if (mask_return & ShiftMask)
      result = result | kModifierShift;
    if (mask_return & ControlMask)
      result = result | kModifierRegCtrl;
    if (mask_return & Mod1Mask)
      result = result | kModifierAlt;
    if (mask_return & Mod4Mask)
      result = result | kModifierMeta;
    return result;
  }

  // XInput2 reports the button number in the event, core events use the same numbering.
  static MouseButton buttonFrom(int button) {
    if (button == Button1)
      return kMouseButtonLeft;
    if (button == Button2)
      return kMouseButtonMiddle;
    if (button == Button3)
      return kMouseButtonRight;
    return kMouseButtonNone;
  }

  static KeyCode translateKeyCode(KeySym keysym) {
    switch (keysym) {
    case XK_a: return KeyCode::A;
    case XK_b: return KeyCode::B;
    case XK_c: return KeyCode::C;
    case XK_d: return KeyCode::D;
    case XK_e: return KeyCode::E;
    case XK_f: return KeyCode::F;
    case XK_g: return KeyCode::G;
    case XK_h: return KeyCode::H;
    case XK_i: return KeyCode::I;
    case XK_j: return KeyCode::J;
    case XK_k: return KeyCode::K;
    case XK_l: return KeyCode::L;
    case XK_m: return KeyCode::M;
    case XK_n: return KeyCode::N;
    case XK_o: return KeyCode::O;
    case XK_p: return KeyCode::P;
    case XK_q: return KeyCode::Q;
    case XK_r: return KeyCode::R;
    case XK_s: return KeyCode::S;
    case XK_t: return KeyCode::T;
    case XK_u: return KeyCode::U;
    case XK_v: return KeyCode::V;
    case XK_w: return KeyCode::W;
    case XK_x: return KeyCode::X;
    case XK_y: return KeyCode::Y;
    case XK_z: return KeyCode::Z;
    case XK_A: return KeyCode::A;
    case XK_B: return KeyCode::B;
    case XK_C: return KeyCode::C;
    case XK_D: return KeyCode::D;
    case XK_E: return KeyCode::E;
    case XK_F: return KeyCode::F;
    case XK_G: return KeyCode::G;
    case XK_H: return KeyCode::H;
    case XK_I: return KeyCode::I;
    case XK_J: return KeyCode::J;
    case XK_K: return KeyCode::K;
    case XK_L: return KeyCode::L;
    case XK_M: return KeyCode::M;
    case XK_N: return KeyCode::N;
    case XK_O: return KeyCode::O;
    case XK_P: return KeyCode::P;
    case XK_Q: return KeyCode::Q;
    case XK_R: return KeyCode::R;
    case XK_S: return KeyCode::S;
    case XK_T: return KeyCode::T;
    case XK_U: return KeyCode::U;
    case XK_V: return KeyCode::V;
    case XK_W: return KeyCode::W;
    case XK_X: return KeyCode::X;
    case XK_Y: return KeyCode::Y;
    case XK_Z: return KeyCode::Z;
    case XK_1: return KeyCode::Number1;
    case XK_2: return KeyCode::Number2;
    case XK_3: return KeyCode::Number3;
    case XK_4: return KeyCode::Number4;
    case XK_5: return KeyCode::Number5;
    case XK_6: return KeyCode::Number6;
    case XK_7: return KeyCode::Number7;
    case XK_8: return KeyCode::Number8;
    case XK_9: return KeyCode::Number9;
    case XK_0: return KeyCode::Number0;
    case XK_Return: return KeyCode::Return;
    case XK_Escape: return KeyCode::Escape;
    case XK_BackSpace: return KeyCode::Backspace;
    case XK_Tab: return KeyCode::Tab;
    case XK_space: return KeyCode::Space;
    case XK_minus: return KeyCode::Minus;
    case XK_equal: return KeyCode::Equals;
    case XK_bracketleft: return KeyCode::LeftBracket;
    case XK_bracketright: return KeyCode::RightBracket;
    case XK_backslash: return KeyCode::Backslash;
    case XK_semicolon: return KeyCode::Semicolon;
    case XK_apostrophe: return KeyCode::Apostrophe;
    case XK_grave: return KeyCode::Grave;
    case XK_comma: return KeyCode::Comma;
    case XK_period: return KeyCode::Period;
    case XK_slash: return KeyCode::Slash;
    case XK_Caps_Lock: return KeyCode::CapsLock;
    case XK_F1: return KeyCode::F1;
    case XK_F2: return KeyCode::F2;
    case XK_F3: return KeyCode::F3;
    case XK_F4: return KeyCode::F4;
    case XK_F5: return KeyCode::F5;
    case XK_F6: return KeyCode::F6;
    case XK_F7: return KeyCode::F7;
    case XK_F8: return KeyCode::F8;
    case XK_F9: return KeyCode::F9;
    case XK_F10: return KeyCode::F10;
    case XK_F11: return KeyCode::F11;
    case XK_F12: return KeyCode::F12;
    case XK_Print: return KeyCode::PrintScreen;
    case XK_Scroll_Lock: return KeyCode::ScrollLock;
    case XK_Pause: return KeyCode::Pause;
    case XK_Insert: return KeyCode::Insert;
    case XK_Home: return KeyCode::Home;
    case XK_Page_Up: return KeyCode::PageUp;
    case XK_Delete: return KeyCode::Delete;
    case XK_End: return KeyCode::End;
    case XK_Page_Down: return KeyCode::PageDown;
    case XK_Right: return KeyCode::Right;
    case XK_Left: return KeyCode::Left;
    case XK_Down: return KeyCode::Down;
    case XK_Up: return KeyCode::Up;
    case XK_Num_Lock: return KeyCode::NumLock;
    case XK_KP_Divide: return KeyCode::KPDivide;
    case XK_KP_Multiply: return KeyCode::KPMultiply;
    case XK_KP_Subtract: return KeyCode::KPMinus;
    case XK_KP_Add: return KeyCode::KPPlus;
    case XK_KP_Enter: return KeyCode::KPEnter;
    case XK_KP_1: return KeyCode::KP1;
    case XK_KP_2: return KeyCode::KP2;
    case XK_KP_3: return KeyCode::KP3;
    case XK_KP_4: return KeyCode::KP4;
    case XK_KP_5: return KeyCode::KP5;
    case XK_KP_6: return KeyCode::KP6;
    case XK_KP_7: return KeyCode::KP7;
    case XK_KP_8: return KeyCode::KP8;
    case XK_KP_9: return KeyCode::KP9;
    case XK_KP_0: return KeyCode::KP0;
    case XK_KP_Decimal: return KeyCode::KPPeriod;
    default: return KeyCode::Unknown;
    }
  }

  ::Window WindowX11::windowUnderCursor(::Window inside) {
    ::Window root_return, child_return;
    int root_x, root_y, win_x, win_y;
    unsigned int mask_return;
    if (XQueryPointer(x11_->display(), inside, &root_return, &child_return, &root_x, &root_y,
                      &win_x, &win_y, &mask_return)) {
      if (child_return != None)
        return windowUnderCursor(child_return);
    }

    return inside;
  }

  ::Window WindowX11::windowUnderCursor() {
    return windowUnderCursor(x11_->rootWindow());
  }

  void WindowX11::sendDragDropEnter(::Window source, ::Window target) const {
    XClientMessageEvent message;
    memset(&message, 0, sizeof(message));
    message.type = ClientMessage;
    message.display = x11_->display();
    message.window = target;
    message.message_type = x11_->dndEnter();
    message.format = 32;
    message.data.l[0] = source;
    message.data.l[1] = 5 << 24;
    message.data.l[2] = x11_->dndUriList();
    message.data.l[3] = None;
    message.data.l[4] = None;

    XSendEvent(x11_->display(), target, False, 0, reinterpret_cast<XEvent*>(&message));
    XFlush(x11_->display());
  }

  void WindowX11::sendDragDropLeave(::Window source, ::Window target) const {
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = x11_->display();
    message.xclient.window = target;
    message.xclient.message_type = x11_->dndLeave();
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;

    XSendEvent(x11_->display(), target, False, 0, &message);
    XFlush(x11_->display());
  }

  void WindowX11::sendDragDropPosition(::Window source, ::Window target, int x, int y,
                                       unsigned long time) const {
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = x11_->display();
    message.xclient.window = target;
    message.xclient.message_type = x11_->dndPosition();
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    message.xclient.data.l[2] = (x << 16) | (y & 0xffff);
    message.xclient.data.l[3] = time;
    message.xclient.data.l[4] = x11_->dndActionCopy();

    XSendEvent(x11_->display(), drag_drop_out_state_.target, False, 0, &message);
    XFlush(x11_->display());
  }

  void WindowX11::sendDragDropStatus(::Window source, ::Window target, bool accept_drag) const {
    // Per XDND spec, XdndStatus replies go directly to the window that sent
    // XdndPosition; XdndProxy indirection only applies to the source->target
    // direction. Looking it up here bounced the reply back to our own window
    // whenever our embed-time XdndProxy property was present on the source.
    ::Window receiver = target;

    XEvent message { 0 };
    message.xclient.type = ClientMessage;
    message.xclient.display = x11_->display();
    message.xclient.window = receiver;
    message.xclient.message_type = x11_->dndStatus();
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    message.xclient.data.l[2] = 1;
    message.xclient.data.l[3] = 0;
    if (accept_drag) {
      message.xclient.data.l[1] = 1;
      message.xclient.data.l[4] = x11_->dndActionCopy();
    }
    else {
      message.xclient.data.l[1] = 0;
      message.xclient.data.l[4] = x11_->dndActionNone();
    }

    XSendEvent(x11_->display(), receiver, False, NoEventMask, &message);
  }

  void WindowX11::sendDragDropDrop(::Window source, ::Window target) const {
    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = x11_->display();
    message.xclient.window = target;
    message.xclient.message_type = x11_->dndDrop();
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    message.xclient.data.l[2] = CurrentTime;

    XSendEvent(x11_->display(), target, False, 0, &message);
  }

  void WindowX11::sendDragDropSelectionNotify(XSelectionRequestEvent* request) const {
    X11Connection::DisplayLock lock(x11_);

    XSelectionEvent result = { 0 };
    result.type = SelectionNotify;
    result.display = request->display;
    result.requestor = request->requestor;
    result.selection = request->selection;
    result.time = request->time;
    result.target = request->target;
    result.property = None;

    if (request->target == x11_->targets()) {
      Atom supported_types[] = { x11_->dndUriList() };
      XChangeProperty(request->display, request->requestor, request->property, XA_ATOM, 32,
                      PropModeReplace, (unsigned char*)supported_types, 1);
      result.property = request->property;
    }
    else if (request->target == x11_->dndUriList()) {
      std::string selection_data;
      for (const auto& drag_drop_file : drag_drop_files_)
        selection_data += "file://" + drag_drop_file + "\r\n";

      XChangeProperty(request->display, request->requestor, request->property, request->target, 8,
                      PropModeReplace, (unsigned char*)selection_data.c_str(), selection_data.length());
      result.property = request->property;
    }

    if (result.property != None)
      XSendEvent(request->display, result.requestor, 0, NoEventMask, (XEvent*)&result);
  }

  void WindowX11::sendDragDropFinished(::Window source, ::Window target, bool accepted_drag) const {
    // Same reply routing as sendDragDropStatus: straight to the XdndDrop sender.
    ::Window receiver = target;

    XEvent message;
    memset(&message, 0, sizeof(message));
    message.xclient.type = ClientMessage;
    message.xclient.display = x11_->display();
    message.xclient.window = receiver;
    message.xclient.message_type = x11_->dndFinished();
    message.xclient.format = 32;
    message.xclient.data.l[0] = source;
    if (accepted_drag) {
      message.xclient.data.l[1] = 1;
      message.xclient.data.l[2] = x11_->dndActionCopy();
    }
    else {
      message.xclient.data.l[1] = 0;
      message.xclient.data.l[2] = x11_->dndActionNone();
    }

    XSendEvent(x11_->display(), receiver, False, NoEventMask, &message);
  }

  void WindowX11::processPluginFdEvents() {
    bool timer_fired = false;
    XEvent event;
    while (XPending(x11_->display())) {
      XNextEvent(x11_->display(), &event);

      if (event.type == GenericEvent) {
        processXInput2GenericEvent(event);
        continue;
      }

      if (event.xany.window == parent_handle_ && event.type == ConfigureNotify) {
        X11Connection::DisplayLock lock(x11_);
        XWindowAttributes attributes;
        XGetWindowAttributes(x11_->display(), parent_handle_, &attributes);
        setNativeWindowSize(attributes.width, attributes.height);
        if (parent_configure_recreate_pending_) {
          parent_configure_recreate_pending_ = false;
          notifyWindowMapped();
        }
      }
      else if (event.xany.window == window_handle_ && event.type == MapNotify) {
        notifyWindowMapped();
      }
      else if (event.xany.window == window_handle_ && event.type == ClientMessage &&
               event.xclient.message_type == x11_->timerEvent()) {
        if (!timer_fired) {
          timer_fired = true;
          long long microseconds = time::microseconds() - start_draw_microseconds_;
          drawCallback(microseconds / 1000000.0);
        }
      }
      else if (event.xany.window == window_handle_ || event.xany.window == parent_handle_)
        processEvent(event);
    }
  }

  void WindowX11::processMessageWindowEvent(XEvent& event) {
    switch (event.type) {
    case SelectionRequest: {
      X11Connection* x11 = X11Connection::globalInstance();
      X11Connection::DisplayLock lock(x11);
      XSelectionRequestEvent* request = &event.xselectionrequest;

      XSelectionEvent result = { 0 };
      result.type = SelectionNotify;
      result.display = request->display;
      result.requestor = request->requestor;
      result.selection = request->selection;
      result.time = request->time;
      result.target = request->target;
      result.property = None;

      if (request->target == x11->targets()) {
        Atom supported_types[] = { x11->utf8String(), XA_STRING };
        XChangeProperty(request->display, request->requestor, request->property, XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)supported_types, 1);
        result.property = request->property;
      }
      else if (request->target == x11->utf8String() || request->target == XA_STRING) {
        XChangeProperty(request->display, request->requestor, request->property, request->target, 8,
                        PropModeReplace, (unsigned char*)_clipboard_text.c_str(),
                        _clipboard_text.length());
        result.property = request->property;
      }

      if (result.property != None)
        XSendEvent(request->display, result.requestor, 0, NoEventMask, (XEvent*)&result);
      break;
    }
    }
  }

  // Core pointer delivery and XInput2 delivery both funnel through these handlers so the two
  // event sources cannot drift apart. They are mutually exclusive for a given event: the server
  // delivers a device event as XInput2 when a client selected that event on the window and as a
  // core event otherwise.
  void WindowX11::processPointerMotion(const PointerEvent& pointer) {
    X11Connection::DisplayLock lock(x11_);
    last_active_window_ = this;

    if (window_operation_ & kMoveWindow) {
      int x_offset = pointer.root_x - dragging_window_position_.x;
      int y_offset = pointer.root_y - dragging_window_position_.y;
      XMoveWindow(x11_->display(), window_handle_, x_offset, y_offset);
    }
    else if (window_operation_) {
      ::Window root;
      int window_x, window_y;
      unsigned int window_width, window_height, border, depth;
      XGetGeometry(x11_->display(), window_handle_, &root, &window_x, &window_y, &window_width,
                   &window_height, &border, &depth);
      int window_right = window_x + window_width;
      int window_bottom = window_y + window_height;

      if (window_operation_ & kResizeLeft)
        window_x = std::min(window_right - kMinWidth, pointer.root_x);
      else if (window_operation_ & kResizeRight)
        window_right = std::max(window_x + kMinHeight, pointer.root_x);
      if (window_operation_ & kResizeTop)
        window_y = std::min(window_bottom - kMinHeight, pointer.root_y);
      else if (window_operation_ & kResizeBottom)
        window_bottom = std::max(window_y + kMinHeight, pointer.root_y);

      XMoveResizeWindow(x11_->display(), window_handle_, window_x, window_y, window_right - window_x,
                        window_bottom - window_y);
    }

    int last_hover_operation = hover_window_operation_;
    hover_window_operation_ = resizeOperationForPosition(pointer.x, pointer.y);
    if (hover_window_operation_)
      setCursorStyle(windowResizeCursor(hover_window_operation_));
    else if (last_hover_operation)
      setCursorStyle(MouseCursor::Arrow);

    if (drag_drop_out_state_.dragging) {
      ::Window last_target = drag_drop_out_state_.target;
      if (pointer.x >= 0 && pointer.x < clientWidth() && pointer.y >= 0 &&
          pointer.y < clientHeight()) {
        drag_drop_out_state_.target = 0;
        handleFileDrag(pointer.x, pointer.y, drag_drop_files_);
      }
      else {
        if (drag_drop_out_state_.target == 0)
          handleFileDragLeave();

        drag_drop_out_state_.target = windowUnderCursor();
      }
      if (last_target != drag_drop_out_state_.target) {
        if (last_target)
          sendDragDropLeave(window_handle_, last_target);
        if (drag_drop_out_state_.target)
          sendDragDropEnter(window_handle_, drag_drop_out_state_.target);
      }
      if (drag_drop_out_state_.target)
        sendDragDropPosition(window_handle_, drag_drop_out_state_.target, pointer.root_x,
                             pointer.root_y, pointer.time);
      return;
    }
    if (mouseRelativeMode() && mouse_down_position_ == IPoint(pointer.x, pointer.y))
      return;

    if (window_operation_ == 0) {
      handleMouseMove(pointer.x, pointer.y, mouseButtonState(), modifierState());
      if (mouseRelativeMode())
        setNativeCursorPosition(mouse_down_position_);
    }
  }

  void WindowX11::processPointerButtonPress(const PointerEvent& pointer) {
    MouseButton button = buttonFrom(pointer.button);
    if (button == kMouseButtonNone)
      passButtonToParent(true, pointer);
    else
      handleMouseDown(button, pointer.x, pointer.y, mouseButtonState(), modifierState());

    HitTestResult hit_test = handleHitTest(pointer.x, pointer.y);

    window_operation_ = resizeOperationForPosition(pointer.x, pointer.y);
    if (hit_test == HitTestResult::TitleBar && window_operation_ == 0) {
      window_operation_ = kMoveWindow;
      ::Window root;
      int window_x, window_y;
      unsigned int window_width, window_height, border, depth;
      XGetGeometry(x11_->display(), window_handle_, &root, &window_x, &window_y, &window_width,
                   &window_height, &border, &depth);
      dragging_window_position_ = { pointer.root_x - window_x, pointer.root_y - window_y };
    }

    mouse_down_position_ = { pointer.x, pointer.y };

    drag_drop_out_state_.dragging = isDragDropSource();
    if (drag_drop_out_state_.dragging) {
      XSetSelectionOwner(x11_->display(), x11_->dndSelection(), window_handle_, pointer.time);

      setCursorStyle(MouseCursor::MultiDirectionalResize);
      drag_drop_files_.clear();
      drag_drop_files_.push_back(startDragDropSource());
    }
  }

  void WindowX11::processPointerButtonRelease(const PointerEvent& pointer) {
    window_operation_ = 0;
    MouseButton button = buttonFrom(pointer.button);
    HitTestResult hit_test = currentHitTest();
    if (drag_drop_out_state_.dragging && button == kMouseButtonLeft) {
      if (pointer.x >= 0 && pointer.x < clientWidth() && pointer.y >= 0 &&
          pointer.y < clientHeight()) {
        if (drag_drop_out_state_.target)
          sendDragDropLeave(window_handle_, drag_drop_out_state_.target);
        handleFileDrop(pointer.x, pointer.y, drag_drop_files_);
      }
      else if (drag_drop_out_state_.target)
        sendDragDropDrop(window_handle_, drag_drop_out_state_.target);

      cleanupDragDropSource();
      drag_drop_out_state_.dragging = false;
      setCursorStyle(MouseCursor::Arrow);
    }
    if (button == kMouseButtonNone)
      passButtonToParent(false, pointer);
    else
      handleMouseUp(button, pointer.x, pointer.y, mouseButtonState(), modifierState());

    if (button == kMouseButtonLeft) {
      if (hit_test == HitTestResult::CloseButton &&
          handleHitTest(pointer.x, pointer.y) == HitTestResult::CloseButton) {
        close();
      }
      else if (hit_test == HitTestResult::MaximizeButton &&
               handleHitTest(pointer.x, pointer.y) == HitTestResult::MaximizeButton) {
        showMaximized();
      }
      else if (hit_test == HitTestResult::MinimizeButton &&
               handleHitTest(pointer.x, pointer.y) == HitTestResult::MinimizeButton) {
        Atom wm_change_state = XInternAtom(x11_->display(), "WM_CHANGE_STATE", False);
        XEvent min_event = {};
        min_event.xclient.type = ClientMessage;
        min_event.xclient.message_type = wm_change_state;
        min_event.xclient.display = x11_->display();
        min_event.xclient.window = window_handle_;
        min_event.xclient.format = 32;
        min_event.xclient.data.l[0] = IconicState;

        XSendEvent(x11_->display(), x11_->rootWindow(), False,
                   SubstructureRedirectMask | SubstructureNotifyMask, &min_event);
        XFlush(x11_->display());
      }
    }
  }

  void WindowX11::processEvent(XEvent& event) {
    switch (event.type) {
    case ClientMessage: {
      X11Connection::DisplayLock lock(x11_);
      if (event.xclient.message_type == x11_->dndEnter()) {
        drag_drop_files_.clear();
        drag_drop_files_requested_ = false;
        drag_drop_files_received_ = false;
        drag_drop_pending_ = false;
        sendDragDropStatus(event.xclient.window, event.xclient.data.l[0], false);
      }
      else if (event.xclient.message_type == x11_->dndLeave()) {
        drag_drop_files_requested_ = false;
        drag_drop_files_received_ = false;
        drag_drop_pending_ = false;
        handleFileDragLeave();
      }
      else if (event.xclient.message_type == x11_->dndDrop()) {
        if (drag_drop_files_received_) {
          bool success = handleFileDrop(drag_drop_target_x_, drag_drop_target_y_, drag_drop_files_);
          sendDragDropFinished(event.xclient.window, event.xclient.data.l[0], success);
        }
        else {
          // Data not fetched yet: request it with the timestamp from XdndDrop
          // (the spec-compliant time) and finish the drop when it arrives.
          drag_drop_pending_ = true;
          drag_drop_message_window_ = event.xclient.window;
          drag_drop_source_window_ = event.xclient.data.l[0];
          XConvertSelection(x11_->display(), x11_->dndSelection(), x11_->dndUriList(),
                            x11_->dndUriList(), window_handle_, event.xclient.data.l[2]);
          XFlush(x11_->display());
        }
      }
      else if (event.xclient.message_type == x11_->dndPosition()) {
        int win_x = 0;
        int win_y = 0;
        ::Window child_return;
        XTranslateCoordinates(x11_->display(), window_handle_, x11_->rootWindow(), 0, 0, &win_x,
                              &win_y, &child_return);

        drag_drop_target_x_ = (event.xclient.data.l[2] >> 16) - win_x;
        drag_drop_target_y_ = (event.xclient.data.l[2] & 0xffff) - win_y;
        bool accepts = handleFileDrag(drag_drop_target_x_, drag_drop_target_y_, drag_drop_files_);
        sendDragDropStatus(event.xclient.window, event.xclient.data.l[0], accepts);

        // Prefetch the file list using the timestamp the source sent in
        // XdndPosition. CurrentTime is rejected by strict sources (Qt refuses
        // any XdndSelection conversion whose time isn't the drag's own
        // timestamp), which previously left drag_drop_files_ empty forever.
        if (!drag_drop_files_received_ && !drag_drop_files_requested_) {
          drag_drop_files_requested_ = true;
          XConvertSelection(x11_->display(), x11_->dndSelection(), x11_->dndUriList(),
                            x11_->dndUriList(), window_handle_, event.xclient.data.l[3]);
          XFlush(x11_->display());
        }
      }
      else if (event.xclient.message_type == x11_->dndStatus()) {
      }
      else if (event.xclient.message_type == x11_->dndFinished()) {
        drag_drop_out_state_.target = 0;
        drag_drop_out_state_.dragging = false;
        setCursorStyle(MouseCursor::Arrow);
      }
      break;
    }
    case SelectionRequest: {
      X11Connection::DisplayLock lock(x11_);
      XSelectionRequestEvent* request = &event.xselectionrequest;
      if (request->selection == x11_->dndSelection())
        sendDragDropSelectionNotify(request);
      break;
    }
    case SelectionNotify: {
      X11Connection::DisplayLock lock(x11_);

      if (event.xselection.selection == x11_->dndSelection()) {
        if (!event.xselection.property) {
          // Conversion refused; allow a retry with a fresh timestamp.
          drag_drop_files_requested_ = false;
        }
        else {
          drag_drop_files_received_ = true;
        }
        if (event.xselection.property) {

          // file:// URIs in XDND uri-lists are percent-encoded (RFC 2396);
          // decode or paths with spaces/special chars fail to open.
          static auto percentDecode = [](const std::string& encoded) {
            auto hexVal = [](char c) -> int {
              if (c >= '0' && c <= '9') return c - '0';
              if (c >= 'a' && c <= 'f') return c - 'a' + 10;
              if (c >= 'A' && c <= 'F') return c - 'A' + 10;
              return -1;
            };
            std::string decoded;
            decoded.reserve(encoded.size());
            for (size_t i = 0; i < encoded.size(); ++i) {
              int hi = i + 2 < encoded.size() ? hexVal(encoded[i + 1]) : -1;
              int lo = hi >= 0 ? hexVal(encoded[i + 2]) : -1;
              if (encoded[i] == '%' && hi >= 0 && lo >= 0) {
                decoded += static_cast<char>(hi * 16 + lo);
                i += 2;
              }
              else
                decoded += encoded[i];
            }
            return decoded;
          };
          const std::string kFilePrefix = "file://";

          Atom actual_type;
          int actual_format;
          unsigned long num_items = 0, bytes_after = 0;
          unsigned char* files_string = nullptr;

          XGetWindowProperty(x11_->display(), event.xany.window, event.xselection.property, 0, ~0,
                             False, AnyPropertyType, &actual_type, &actual_format, &num_items,
                             &bytes_after, &files_string);

          if (files_string) {
            std::vector<std::string> result;
            if (num_items && files_string[num_items - 1] == 0)
              num_items--;

            std::stringstream stream(std::string((char*)files_string, num_items));
            std::string line;

            drag_drop_files_.clear();
            while (std::getline(stream, line)) {
              std::string trimmed = String(line).trim().toUtf8();
              if (trimmed.substr(0, kFilePrefix.size()) == kFilePrefix)
                drag_drop_files_.push_back(percentDecode(trimmed.substr(kFilePrefix.size())));
              else
                drag_drop_files_.push_back(trimmed);
            }

            XFree(files_string);
          }
        }
        if (drag_drop_pending_) {
          drag_drop_pending_ = false;
          bool success = handleFileDrop(drag_drop_target_x_, drag_drop_target_y_, drag_drop_files_);
          sendDragDropFinished(drag_drop_message_window_, drag_drop_source_window_, success);
        }
      }
      break;
    }
    case MotionNotify: {
      processPointerMotion({ event.xmotion.x, event.xmotion.y, event.xmotion.x_root,
                             event.xmotion.y_root, 0, event.xmotion.time });
      break;
    }
    case ButtonPress: {
      PointerEvent pointer { event.xbutton.x, event.xbutton.y, event.xbutton.x_root,
                             event.xbutton.y_root, static_cast<int>(event.xbutton.button),
                             event.xbutton.time };
      if (pointer.button >= 4 && pointer.button <= 7) {
        float y = (pointer.button == 4 ? 1.0f : 0.0f) - (pointer.button == 5 ? 1.0f : 0.0f);
        float x = (pointer.button == 7 ? 1.0f : 0.0f) - (pointer.button == 6 ? 1.0f : 0.0f);
        handleMouseWheel(x, y, pointer.x, pointer.y, mouseButtonState(), modifierState());
      }
      else
        processPointerButtonPress(pointer);
      break;
    }
    case ButtonRelease: {
      processPointerButtonRelease({ event.xbutton.x, event.xbutton.y, event.xbutton.x_root,
                                    event.xbutton.y_root, static_cast<int>(event.xbutton.button),
                                    event.xbutton.time });
      break;
    }
    case EnterNotify: {
      if (event.xcrossing.detail != XINotifyInferior)
        reloadInputDeviceStates();
      handleMouseEnter(event.xcrossing.x, event.xcrossing.y);
      break;
    }
    case LeaveNotify: {
      handleMouseLeave(mouseButtonState(), modifierState());
      break;
    }
    case KeyPress: {
      static constexpr int kMaxCharacters = 32;
      int modifier_state = modifierState();
      char buffer[kMaxCharacters] {};
      KeySym keysym;
      int length = XLookupString(&event.xkey, buffer, sizeof(buffer), &keysym, nullptr);
      if ((modifier_state & kModifierAlt) == 0 && length && buffer[0] < 127)
        handleTextInput(std::string(buffer, length));

      bool repeat = pressed_[keysym];
      pressed_[keysym] = true;
      KeyCode key_code = translateKeyCode(keysym);
      bool used = false;
      if (key_code != KeyCode::Unknown)
        used = handleKeyDown(key_code, modifier_state, repeat);

      if (!used)
        passEventToParent(event);
      break;
    }
    case KeyRelease: {
      KeySym keysym = XLookupKeysym(&event.xkey, 0);
      pressed_[keysym] = false;
      KeyCode key_code = translateKeyCode(keysym);
      bool used = false;
      if (key_code != KeyCode::Unknown)
        used = handleKeyUp(key_code, modifierState());

      if (!used)
        passEventToParent(event);
      break;
    }
    case ConfigureNotify: {
      IPoint dimensions = retrieveWindowDimensions();
      if (dimensions.x != clientWidth() || dimensions.y != clientHeight()) {
        handleResized(dimensions.x, dimensions.y);
        long long us_time = time::microseconds() - start_microseconds_;
        drawCallback(us_time / 1000000.0);
      }
      break;
    }
    }
  }

  void WindowX11::runEventLoop() {
    Display* display = x11_->display();

    timeval timeout {};
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    fd_set read_fds;
    unsigned int fd = ConnectionNumber(display);

    start_microseconds_ = time::microseconds();
    long long last_timer_microseconds = start_microseconds_;

    XEvent event;
    bool running = true;
    while (running) {
      FD_ZERO(&read_fds);
      FD_SET(fd, &read_fds);

      timeout.tv_sec = 0;
      long long us_to_timer = timer_microseconds_ - (time::microseconds() - last_timer_microseconds);
      int result = 0;
      if (us_to_timer > 0) {
        timeout.tv_usec = us_to_timer;
        result = select(fd + 1, &read_fds, nullptr, nullptr, &timeout);
      }

      if (result == -1)
        running = false;

      while (running && XPending(x11_->display())) {
        XNextEvent(x11_->display(), &event);
        if (event.type == GenericEvent) {
          // The window an extension event was delivered to is part of the event payload.
          processXInput2GenericEvent(event);
          continue;
        }

        WindowX11* window = NativeWindowLookup::instance().findWindow(event.xany.window);
        if (window == nullptr)
          continue;

        if (event.type == Expose && decoration_ != Decoration::Native) {
          int height = clientHeight();
          window->handleResized(clientWidth(), height + 1);
          window->handleResized(clientWidth(), height);
        }

        if (event.type == DestroyNotify ||
            (event.type == ClientMessage && event.xclient.data.l[0] == x11_->deleteMessage())) {
          if (window->handleCloseRequested()) {
            NativeWindowLookup::instance().removeWindow(window);
            window->hide();
            if (!NativeWindowLookup::instance().anyWindowOpen())
              running = false;
          }
        }
        else
          window->processEvent(event);
      }

      if (timer_microseconds_ - (time::microseconds() - last_timer_microseconds) <= 0) {
        last_timer_microseconds = time::microseconds();
        long long us_time = last_timer_microseconds - start_microseconds_;
        drawCallback(us_time / 1000000.0);
      }
    }
  }

  void WindowX11::windowContentsResized(int width, int height) {
    XResizeWindow(x11_->display(), window_handle_, width, height);
  }

  void WindowX11::show() {
    X11Connection::DisplayLock lock(x11_);

    XSizeHints* size_hints = XAllocSizeHints();
    ::Window handle = parent_handle_ ? parent_handle_ : window_handle_;
    long supplied_return;
    XGetWMNormalHints(x11_->display(), handle, size_hints, &supplied_return);
    int min_width = kMinWidth;
    int min_height = kMinHeight;
    handleAdjustResize(&min_width, &min_height, true, true);
    size_hints->min_width = min_width;
    size_hints->min_height = min_height;
    auto max_dimension = maxWindowDimensions();
    size_hints->max_width = max_dimension.x;
    size_hints->max_height = max_dimension.y;
    XSetWMNormalHints(x11_->display(), handle, size_hints);
    XFree(size_hints);

    XMapWindow(x11_->display(), window_handle_);
    XSetWMProtocols(x11_->display(), window_handle_, x11_->deleteMessageRef(), 1);
    XFlush(x11_->display());
    handleWindowShown();
  }

  void WindowX11::showMaximized() {
    show();
    Atom wm_state = XInternAtom(x11_->display(), "_NET_WM_STATE", False);
    Atom max_horizontal = XInternAtom(x11_->display(), "_NET_WM_STATE_MAXIMIZED_HORZ", False);
    Atom max_vertical = XInternAtom(x11_->display(), "_NET_WM_STATE_MAXIMIZED_VERT", False);

    XEvent event = {};
    event.type = ClientMessage;
    event.xclient.window = window_handle_;
    event.xclient.message_type = wm_state;
    event.xclient.format = 32;
    event.xclient.data.l[0] = 2;
    event.xclient.data.l[1] = max_horizontal;
    event.xclient.data.l[2] = max_vertical;
    event.xclient.data.l[3] = 0;
    event.xclient.data.l[4] = 0;

    XSendEvent(x11_->display(), x11_->rootWindow(), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &event);

    for (int retries = 0; retries < 10; ++retries) {
      Thread::sleep(10);
      while (XPending(x11_->display())) {
        XEvent e;
        XNextEvent(x11_->display(), &e);

        if (e.type == ConfigureNotify && e.xconfigure.window == window_handle_) {
          visage::IPoint dimensions = retrieveWindowDimensions();
          handleResized(dimensions.x, dimensions.y);
          return;
        }
      }
    }
  }

  void WindowX11::hide() {
    X11Connection::DisplayLock lock(x11_);
    ::Display* display = x11_->display();
    XUnmapWindow(display, window_handle_);
    XFlush(display);
    handleWindowHidden();
  }

  void WindowX11::close() {
    XEvent close_event = {};
    close_event.xclient.type = ClientMessage;
    close_event.xclient.window = window_handle_;
    close_event.xclient.message_type = XInternAtom(x11_->display(), "WM_PROTOCOLS", True);
    close_event.xclient.format = 32;
    close_event.xclient.data.l[0] = x11_->deleteMessage();
    close_event.xclient.data.l[1] = CurrentTime;

    XSendEvent(x11_->display(), window_handle_, False, NoEventMask, &close_event);
    XFlush(x11_->display());
  }

  bool WindowX11::isShowing() const {
    X11Connection::DisplayLock lock(x11_);
    XWindowAttributes attributes;
    if (XGetWindowAttributes(x11_->display(), window_handle_, &attributes) == 0)
      return false;
    return attributes.map_state == IsViewable;
  }

  void WindowX11::setWindowTitle(const std::string& title) {
    X11Connection::DisplayLock lock(x11_);
    XStoreName(x11_->display(), window_handle_, title.c_str());
  }

  IPoint WindowX11::maxWindowDimensions() const {
    MonitorInfo monitor_info = activeMonitorInfo();

    int display_width = monitor_info.bounds.width();
    int display_height = monitor_info.bounds.height();
    float aspect_ratio = clientWidth() * 1.0f / clientHeight();
    return { std::min<int>(display_width, display_height * aspect_ratio),
             std::min<int>(display_height, display_width / aspect_ratio) };
  }
}
#endif
