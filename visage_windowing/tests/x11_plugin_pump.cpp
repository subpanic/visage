#include "visage_windowing/windowing.h"

#include <catch2/catch_test_macros.hpp>
#include <X11/Xlib.h>
#include <poll.h>

#include <chrono>
#include <thread>

namespace {
  using namespace visage;

  struct DragReceiver : visage::Window::EventHandler {
    std::vector<int> positions;
    int presses = 0;
    int releases = 0;
    int released_at = -1;
    bool absolute_drag = false;
    int position_at_wheel = -1;
    int position_at_release = -1;

    bool canCoalesceMouseDrag() const override {
      return absolute_drag && presses > releases && !positions.empty();
    }

    void handleMouseDown(MouseButton button, int, int, int, int, int) override {
      if (button == kMouseButtonMiddle)
        ++presses;
    }
    void handleMouseMove(int x, int, int, int) override {
      positions.push_back(x);
      // Model expensive UI work without depending on graph size or machine speed.
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    void handleMouseUp(MouseButton button, int x, int, int, int, int) override {
      if (button == kMouseButtonMiddle) {
        ++releases;
        position_at_release = positions.empty() ? -1 : positions.back();
        released_at = x;
      }
    }

    HitTestResult handleHitTest(int, int) override { return HitTestResult::Client; }
    HitTestResult currentHitTest() const override { return HitTestResult::Client; }
    void handleMouseEnter(int, int) override { }
    void handleMouseLeave(int, int, int, int) override { }
    void handleMouseWheel(float, float, float, float, int, int, int, int, bool, bool) override {
      position_at_wheel = positions.empty() ? -1 : positions.back();
    }
    void handleMouseMagnify(float, int, int, int, int) override { }
    bool handleKeyDown(visage::KeyCode, int, bool) override { return false; }
    bool handleKeyUp(visage::KeyCode, int) override { return false; }
    bool handleTextInput(const std::string&) override { return false; }
    bool hasActiveTextEntry() override { return false; }
    void handleFocusLost() override { }
    void handleFocusGained() override { }
    void handleResized(int, int) override { }
    void handleWindowShown() override { }
    void handleWindowHidden() override { }
    bool handleCloseRequested() override { return false; }
    bool handleFileDrag(int, int, const std::vector<std::string>&) override { return false; }
    void handleFileDragLeave() override { }
    bool handleFileDrop(int, int, const std::vector<std::string>&) override { return false; }
    bool isDragDropSource() override { return false; }
    std::string startDragDropSource() override { return {}; }
    void cleanupDragDropSource() override { }
  };
}

TEST_CASE("Embedded X11 drag dispatch under queued motion", "[x11][input]") {
  std::unique_ptr<Display, decltype(&XCloseDisplay)> host(XOpenDisplay(nullptr), XCloseDisplay);
  if (!host)
    SKIP("Requires an X11 or Xwayland display");

  // Never map the parent or move the user's pointer. Closing the display destroys the parent.
  auto parent = XCreateSimpleWindow(host.get(), DefaultRootWindow(host.get()), 0, 0, 100, 100, 0, 0, 0);
  XSync(host.get(), False);
  DragReceiver receiver;
  SECTION("Path-sensitive drag") { receiver.absolute_drag = false; }
  SECTION("Absolute pan") { receiver.absolute_drag = true; }
  bool drew_during_drag = false;
  auto child = visage::createPluginWindow(100, 100, reinterpret_cast<void*>(parent));
  child->setEventHandler(&receiver);
  child->setDrawCallback([&](double) {
    if (!receiver.positions.empty() && !receiver.releases)
      drew_during_drag = true;
  });

  auto xid = reinterpret_cast<::Window>(child->nativeHandle());
  auto send_button = [&](int type, int button, int x) {
    XEvent e {};
    e.xbutton.type = type;
    e.xbutton.window = xid;
    e.xbutton.button = button;
    e.xbutton.x = x;
    XSendEvent(host.get(), xid, False, NoEventMask, &e);
  };
  constexpr int kMoves = 32;
  send_button(ButtonPress, Button2, 0);
  for (int x = 1; x <= kMoves; ++x) {
    XEvent e {};
    e.xmotion.type = MotionNotify;
    e.xmotion.window = xid;
    e.xmotion.x = x;
    e.xmotion.state = Button2Mask;
    XSendEvent(host.get(), xid, False, NoEventMask, &e);
    if (x == kMoves / 2) {
      send_button(ButtonPress, Button4, x);
      send_button(ButtonRelease, Button4, x);
    }
  }
  send_button(ButtonRelease, Button2, kMoves);
  XSync(host.get(), False);

  pollfd fd { child->posixFd(), POLLIN, 0 };
  bool yielded_during_drag = false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  while (!receiver.releases && std::chrono::steady_clock::now() < deadline) {
    if (poll(&fd, 1, 100) > 0 && (fd.revents & POLLIN)) {
      child->processPluginFdEvents();
      yielded_during_drag |= !receiver.positions.empty() && !receiver.releases;
    }
  }
  REQUIRE(receiver.position_at_wheel == kMoves / 2);
  REQUIRE(receiver.position_at_release == kMoves);
  REQUIRE(receiver.presses == 1);
  REQUIRE(receiver.releases == 1);
  REQUIRE(receiver.released_at == kMoves);
  if (receiver.absolute_drag) {
    REQUIRE(receiver.positions.size() < kMoves);
    REQUIRE(receiver.positions.front() == 1);
  }
  else {
    REQUIRE(yielded_during_drag);
    REQUIRE(drew_during_drag);
    REQUIRE(receiver.positions.size() == kMoves);
    for (int x = 1; x <= kMoves; ++x)
      REQUIRE(receiver.positions[x - 1] == x);
  }
}
