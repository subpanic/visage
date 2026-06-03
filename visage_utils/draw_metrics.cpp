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

#include "draw_metrics.h"

#include "time_utils.h"

namespace visage {

#if VISAGE_DRAW_METRICS
  DrawMetricsCollector& DrawMetricsCollector::instance() {
    static DrawMetricsCollector collector;
    return collector;
  }

  void DrawMetricsCollector::beginFrame() {
    currentFrame_ = {};
    drawStack_.clear();
    frameStartTime_ = time::microseconds();
  }

  void DrawMetricsCollector::beginFrameDraw(const std::string& className,
                                            const std::string& frameName) {
    FrameEntry entry;
    entry.className = className;
    entry.frameName = frameName;
    entry.parentIndex = static_cast<int>(drawStack_.size()) - 1;

    FrameEntry* parent = nullptr;
    if (!drawStack_.empty())
      parent = drawStack_.back();

    FrameEntry* target = nullptr;
    if (parent) {
      parent->children.push_back(std::move(entry));
      target = &parent->children.back();
    }
    else {
      currentFrame_.root = std::move(entry);
      target = &currentFrame_.root;
    }

    drawStack_.push_back(target);
    drawStartTime_ = time::microseconds();
  }

  void DrawMetricsCollector::endFrameDraw() {
    double endTime = time::microseconds();
    if (!drawStack_.empty()) {
      drawStack_.back()->cpuDrawMs = static_cast<float>((endTime - drawStartTime_) / 1000.0);
      drawStack_.pop_back();
    }
  }

  void DrawMetricsCollector::endFrame(float gpuTimeMs, uint32_t numDrawCalls) {
    double endTime = time::microseconds();
    currentFrame_.wallClockTimeMs = static_cast<float>((endTime - frameStartTime_) / 1000.0);
    currentFrame_.gpuTimeMs = gpuTimeMs;
    currentFrame_.numDrawCalls = numDrawCalls;

    if (frames_.size() < kWindowSize)
      frames_.push_back(currentFrame_);
    else
      frames_[currentIndex_] = currentFrame_;

    currentIndex_ = (currentIndex_ + 1) % kWindowSize;
    frameCount_++;
  }

  bool DrawMetricsCollector::getSnapshot(DrawMetricsSnapshot& snapshot) {
    if (frameCount_ == 0)
      return false;

    averageFrames(snapshot);
    return true;
  }

  void DrawMetricsCollector::averageFrames(DrawMetricsSnapshot& snapshot) {
    size_t count = std::min(frameCount_, frames_.size());
    if (count == 0)
      return;

    FrameWindow averaged;
    for (size_t i = 0; i < count; ++i) {
      averaged.wallClockTimeMs += frames_[i].wallClockTimeMs;
      averaged.gpuTimeMs += frames_[i].gpuTimeMs;
      averaged.numDrawCalls += frames_[i].numDrawCalls;
      if (i == 0)
        averaged.root = frames_[i].root;
      else
        accumulateEntry(averaged.root, frames_[i].root);
    }

    snapshot.wallClockTimeMs = averaged.wallClockTimeMs / count;
    snapshot.gpuTimeMs = averaged.gpuTimeMs / count;
    snapshot.numDrawCalls = averaged.numDrawCalls / static_cast<uint32_t>(count);
    finalizeEntry(snapshot.rootHierarchy, averaged.root, static_cast<uint32_t>(count));
  }

  void DrawMetricsCollector::accumulateEntry(FrameEntry& dst, const FrameEntry& src) {
    dst.cpuDrawMs += src.cpuDrawMs;

    for (size_t i = 0; i < src.children.size(); ++i) {
      if (i < dst.children.size()) {
        accumulateEntry(dst.children[i], src.children[i]);
      }
      else {
        dst.children.push_back(src.children[i]);
      }
    }
  }

  void DrawMetricsCollector::finalizeEntry(FrameDrawMetrics& dst, const FrameEntry& src,
                                           uint32_t count) {
    dst.className = src.className;
    dst.frameName = src.frameName;
    dst.cpuDrawMs = src.cpuDrawMs / count;
    dst.children.resize(src.children.size());
    for (size_t i = 0; i < src.children.size(); ++i)
      finalizeEntry(dst.children[i], src.children[i], count);
  }
#endif

}
