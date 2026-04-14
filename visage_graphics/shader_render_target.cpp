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

#include "shader_render_target.h"

#include <array>

#include <bgfx/bgfx.h>

namespace visage {
  namespace {
    bgfx::TextureFormat::Enum bgfxTextureFormat(ShaderRenderTarget::Format format) {
      switch (format) {
      case ShaderRenderTarget::Format::RGBA8:
        return bgfx::TextureFormat::RGBA8;
      case ShaderRenderTarget::Format::RGB10A2:
        return bgfx::TextureFormat::RGB10A2;
      case ShaderRenderTarget::Format::RGBA16F:
        return bgfx::TextureFormat::RGBA16F;
      }

      VISAGE_ASSERT(false);
      return bgfx::TextureFormat::RGBA8;
    }

    uint64_t samplerFlags(ShaderRenderTarget::Filter filter, ShaderRenderTarget::Wrap wrap) {
      uint64_t flags = BGFX_TEXTURE_RT;
      if (wrap == ShaderRenderTarget::Wrap::Clamp)
        flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
      if (filter == ShaderRenderTarget::Filter::Point)
        flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
      return flags;
    }
  }

  struct ShaderRenderTarget::State {
    std::array<bgfx::FrameBufferHandle, 2> frame_buffers = {{
      BGFX_INVALID_HANDLE,
      BGFX_INVALID_HANDLE,
    }};
  };

  ShaderRenderTarget::ShaderRenderTarget(Format format, Filter filter, Wrap wrap, bool ping_pong) :
      state_(std::make_unique<State>()), format_(format), filter_(filter), wrap_(wrap),
      ping_pong_(ping_pong) { }

  ShaderRenderTarget::~ShaderRenderTarget() {
    reset();
  }

  ShaderRenderTarget::ShaderRenderTarget(ShaderRenderTarget&&) noexcept = default;
  ShaderRenderTarget& ShaderRenderTarget::operator=(ShaderRenderTarget&&) noexcept = default;

  void ShaderRenderTarget::reset() {
    destroyBuffers();
    width_ = 0;
    height_ = 0;
    read_index_ = 0;
  }

  void ShaderRenderTarget::ensureSize(int width, int height) {
    width = std::max(1, width);
    height = std::max(1, height);
    if (width_ == width && height_ == height && hasHandle())
      return;

    destroyBuffers();
    width_ = width;
    height_ = height;
    read_index_ = 0;

    const bgfx::TextureFormat::Enum format = bgfxTextureFormat(format_);
    const uint64_t flags = samplerFlags(filter_, wrap_);
    const int buffer_count = ping_pong_ ? 2 : 1;
    for (int i = 0; i < buffer_count; ++i)
      state_->frame_buffers[i] = bgfx::createFrameBuffer(width_, height_, format, flags);
  }

  void ShaderRenderTarget::swap() {
    if (ping_pong_)
      read_index_ = 1 - read_index_;
  }

  bool ShaderRenderTarget::hasHandle(Buffer buffer) const {
    return bgfx::isValid(frameBufferHandle(buffer));
  }

  bgfx::FrameBufferHandle ShaderRenderTarget::frameBufferHandle(Buffer buffer) const {
    return state_->frame_buffers[bufferIndex(buffer)];
  }

  bgfx::TextureHandle ShaderRenderTarget::textureHandle(Buffer buffer) const {
    bgfx::FrameBufferHandle frame_buffer = frameBufferHandle(buffer);
    if (!bgfx::isValid(frame_buffer))
      return BGFX_INVALID_HANDLE;
    return bgfx::getTexture(frame_buffer);
  }

  int ShaderRenderTarget::bufferIndex(Buffer buffer) const {
    if (!ping_pong_)
      return 0;

    switch (buffer) {
    case Buffer::Current:
    case Buffer::Read:
      return read_index_;
    case Buffer::Write:
      return 1 - read_index_;
    }

    VISAGE_ASSERT(false);
    return read_index_;
  }

  void ShaderRenderTarget::destroyBuffers() {
    for (auto& frame_buffer : state_->frame_buffers) {
      if (bgfx::isValid(frame_buffer))
        bgfx::destroy(frame_buffer);
      frame_buffer = BGFX_INVALID_HANDLE;
    }
  }
}
