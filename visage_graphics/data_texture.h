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
  class ShaderDataTexture {
  public:
    enum class Format {
      RGBA8,
      Float32,
    };

    enum class Filter {
      Point,
      Linear,
    };

    enum class Wrap {
      Clamp,
      Repeat,
    };

    explicit ShaderDataTexture(Format format = Format::RGBA8, Filter filter = Filter::Linear,
                               Wrap wrap = Wrap::Clamp);
    ~ShaderDataTexture();

    ShaderDataTexture(const ShaderDataTexture&) = delete;
    ShaderDataTexture& operator=(const ShaderDataTexture&) = delete;
    ShaderDataTexture(ShaderDataTexture&&) noexcept;
    ShaderDataTexture& operator=(ShaderDataTexture&&) noexcept;

    void reset();
    void resize(int width, int height);
    void update(const void* data, int width, int height);

    bool hasHandle() const;
    int width() const;
    int height() const;
    Format format() const;
    Filter filter() const;
    Wrap wrap() const;

    const bgfx::TextureHandle& textureHandle() const;

  private:
    struct State;
    std::unique_ptr<State> state_;
  };
}
