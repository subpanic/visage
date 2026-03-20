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

#include "data_texture.h"

#include <bgfx/bgfx.h>

namespace visage {
  namespace {
    bgfx::TextureFormat::Enum bgfxTextureFormat(ShaderDataTexture::Format format) {
      switch (format) {
      case ShaderDataTexture::Format::RGBA8:
        return bgfx::TextureFormat::RGBA8;
      case ShaderDataTexture::Format::Float32:
        return bgfx::TextureFormat::R32F;
      }

      VISAGE_ASSERT(false);
      return bgfx::TextureFormat::RGBA8;
    }

    int bytesPerPixel(ShaderDataTexture::Format format) {
      switch (format) {
      case ShaderDataTexture::Format::RGBA8:
        return 4;
      case ShaderDataTexture::Format::Float32:
        return 4;
      }

      VISAGE_ASSERT(false);
      return 4;
    }

    uint64_t samplerFlags(ShaderDataTexture::Filter filter, ShaderDataTexture::Wrap wrap) {
      uint64_t flags = 0;

      if (wrap == ShaderDataTexture::Wrap::Clamp)
        flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

      if (filter == ShaderDataTexture::Filter::Point)
        flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;

      return flags;
    }
  }

  struct ShaderDataTexture::State {
    State(Format texture_format, Filter texture_filter, Wrap texture_wrap) :
        format(texture_format), filter(texture_filter), wrap(texture_wrap) { }

    ~State() { destroyHandle(); }

    void destroyHandle() {
      if (bgfx::isValid(handle))
        bgfx::destroy(handle);

      handle = BGFX_INVALID_HANDLE;
    }

    void ensureHandle() {
      if (width <= 0 || height <= 0)
        return;

      if (!bgfx::isValid(handle)) {
        handle = bgfx::createTexture2D(static_cast<uint16_t>(width), static_cast<uint16_t>(height),
                                       false, 1, bgfxTextureFormat(format),
                                       samplerFlags(filter, wrap));
      }
    }

    Format format = Format::RGBA8;
    Filter filter = Filter::Linear;
    Wrap wrap = Wrap::Clamp;
    int width = 0;
    int height = 0;
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
  };

  ShaderDataTexture::ShaderDataTexture(Format format, Filter filter, Wrap wrap) :
      state_(std::make_unique<State>(format, filter, wrap)) { }

  ShaderDataTexture::~ShaderDataTexture() = default;

  ShaderDataTexture::ShaderDataTexture(ShaderDataTexture&&) noexcept = default;
  ShaderDataTexture& ShaderDataTexture::operator=(ShaderDataTexture&&) noexcept = default;

  void ShaderDataTexture::reset() {
    state_->destroyHandle();
    state_->width = 0;
    state_->height = 0;
  }

  void ShaderDataTexture::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
      reset();
      return;
    }

    if (state_->width == width && state_->height == height)
      return;

    state_->destroyHandle();
    state_->width = width;
    state_->height = height;
  }

  void ShaderDataTexture::update(const void* data, int width, int height) {
    if (data == nullptr || width <= 0 || height <= 0) {
      VISAGE_ASSERT(false);
      return;
    }

    resize(width, height);
    state_->ensureHandle();
    VISAGE_ASSERT(bgfx::isValid(state_->handle));

    const uint32_t size = static_cast<uint32_t>(width * height * bytesPerPixel(state_->format));
    bgfx::updateTexture2D(state_->handle, 0, 0, 0, 0, static_cast<uint16_t>(width),
                          static_cast<uint16_t>(height), bgfx::copy(data, size));
  }

  bool ShaderDataTexture::hasHandle() const { return bgfx::isValid(state_->handle); }
  int ShaderDataTexture::width() const { return state_->width; }
  int ShaderDataTexture::height() const { return state_->height; }
  ShaderDataTexture::Format ShaderDataTexture::format() const { return state_->format; }
  ShaderDataTexture::Filter ShaderDataTexture::filter() const { return state_->filter; }
  ShaderDataTexture::Wrap ShaderDataTexture::wrap() const { return state_->wrap; }

  const bgfx::TextureHandle& ShaderDataTexture::textureHandle() const {
    VISAGE_ASSERT(bgfx::isValid(state_->handle));
    return state_->handle;
  }
}
