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

#include "graphics_utils.h"

#include <memory>

namespace visage {
  class ShaderRenderTarget {
  public:
    enum class Format {
      RGBA8,
      RGB10A2,
      RGBA16F,
    };

    enum class Filter {
      Linear,
      Point,
    };

    enum class Wrap {
      Clamp,
      Repeat,
    };

    enum class Buffer {
      Current,
      Read,
      Write,
    };

    ShaderRenderTarget(Format format = Format::RGBA8,
                       Filter filter = Filter::Linear,
                       Wrap wrap = Wrap::Clamp,
                       bool ping_pong = false);
    ~ShaderRenderTarget();

    ShaderRenderTarget(const ShaderRenderTarget&) = delete;
    ShaderRenderTarget& operator=(const ShaderRenderTarget&) = delete;
    ShaderRenderTarget(ShaderRenderTarget&&) noexcept;
    ShaderRenderTarget& operator=(ShaderRenderTarget&&) noexcept;

    void reset();
    void ensureSize(int width, int height);
    void swap();

    bool hasHandle(Buffer buffer = Buffer::Current) const;
    bool pingPong() const { return ping_pong_; }
    int width() const { return width_; }
    int height() const { return height_; }
    Format format() const { return format_; }
    Filter filter() const { return filter_; }
    Wrap wrap() const { return wrap_; }

    bgfx::FrameBufferHandle frameBufferHandle(Buffer buffer = Buffer::Current) const;
    bgfx::TextureHandle textureHandle(Buffer buffer = Buffer::Current) const;

  private:
    struct State;

    int bufferIndex(Buffer buffer) const;
    void destroyBuffers();

    std::unique_ptr<State> state_;
    int width_ = 0;
    int height_ = 0;
    Format format_ = Format::RGBA8;
    Filter filter_ = Filter::Linear;
    Wrap wrap_ = Wrap::Clamp;
    bool ping_pong_ = false;
    int read_index_ = 0;
  };
}
