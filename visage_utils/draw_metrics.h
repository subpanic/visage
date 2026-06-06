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

#include <cstdint>
#include <string>
#include <vector>

namespace visage {

  struct FrameDrawMetrics {
    std::string className;
    std::string frameName;
    float cpuDrawMs = 0.0f;
    std::vector<FrameDrawMetrics> children;
  };

  struct DrawMetricsSnapshot {
    float wallClockTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    uint32_t numDrawCalls = 0;
    FrameDrawMetrics rootHierarchy;
  };

#if VISAGE_DRAW_METRICS
  class DrawMetricsCollector {
  public:
    static DrawMetricsCollector& instance();

    void beginFrame();
    void beginFrameDraw(const std::string& className, const std::string& frameName);
    void endFrameDraw();
    void endFrame(float gpuTimeMs, uint32_t numDrawCalls);

    bool getSnapshot(DrawMetricsSnapshot& snapshot);

  private:
    static constexpr size_t kWindowSize = 30;

    struct FrameEntry {
      std::string className;
      std::string frameName;
      float cpuDrawMs = 0.0f;
      std::vector<FrameEntry> children;
      int parentIndex = -1;
    };

    struct FrameWindow {
      float wallClockTimeMs = 0.0f;
      float gpuTimeMs = 0.0f;
      uint32_t numDrawCalls = 0;
      FrameEntry root;
    };

    DrawMetricsCollector() = default;

    void averageFrames(DrawMetricsSnapshot& snapshot);
    static void accumulateEntry(FrameEntry& dst, const FrameEntry& src);
    static void finalizeEntry(FrameDrawMetrics& dst, const FrameEntry& src, uint32_t count);

    std::vector<FrameWindow> frames_;
    size_t currentIndex_ = 0;
    size_t frameCount_ = 0;

    FrameWindow currentFrame_;
    std::vector<FrameEntry*> drawStack_;
    double frameStartTime_ = 0.0;
    double drawStartTime_ = 0.0;
  };
#endif

}
