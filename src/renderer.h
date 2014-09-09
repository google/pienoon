// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FPL_RENDERER_H
#define FPL_RENDERER_H

#include "shader.h"
#include "material.h"
#include "mesh.h"

namespace fpl {

// The core of the rendering system. Deals with setting up and shutting down
// the window + OpenGL context (based on SDL), and creating/using resources
// such as shaders, textures, and geometry.
class Renderer {
 public:
  // Creates the window + OpenGL context.
  // A descriptive error is in last_error() if it returns false.
  bool Initialize(const vec2i &window_size = vec2i(800, 600),
                  const char *window_title = "");

  // Swaps frames. Call this once per frame inside your main loop.
  void AdvanceFrame(bool minimized);

  // Cleans up whatever Initialize creates.
  void ShutDown();

  // Clears the framebuffer. Call this after AdvanceFrame if desired.
  void ClearFrameBuffer(const vec4 &color);

  // Create a shader object from two strings containing glsl code.
  // Returns nullptr upon error, with a descriptive message in glsl_error().
  // Attribute names in the vertex shader should be aPosition, aNormal,
  // aTexCoord and aColor to match whatever attributes your vertex data has.
  Shader *CompileAndLinkShader(const char *vs_source, const char *ps_source);

  // Create a texture from a memory buffer containing xsize * ysize RGBA pixels.
  // Does not fail.
  Texture *CreateTexture(const uint8_t *buffer, const vec2i &size);

  // Create a texture from a memory buffer containing a TGA format file.
  // May only be uncompressed RGB or RGBA data. Returns nullptr if the format is
  // not understood.
  Texture *CreateTextureFromTGAMemory(const void *tga_buf);

  // Create a texture from a memory buffer containing a Webp format file.
  // Returns nullptr if the format is not understood.
  Texture *CreateTextureFromWebpMemory(const void *webp_buf, size_t size);

  // Set alpha test (cull pixels with alpha below amount) vs alpha blend
  // (blend with framebuffer pixel regardedless).
  // blend_mode: see materials.fbs for valid enum values.
  void SetBlendMode(BlendMode blend_mode, float amount = 0.5f);

  // Set to compare fragment against Z-buffer before writing, or not.
  void DepthTest(bool on);

  Renderer() : model_view_projection_(mat4::Identity()),
               model_(mat4::Identity()), color_(mathfu::kOnes4f),
               light_pos_(mathfu::kZeros3f), camera_pos_(mathfu::kZeros3f),
               window_size_(mathfu::kZeros2i), window_(nullptr),
               context_(nullptr) {}
  ~Renderer() { ShutDown(); }

  mat4 &model_view_projection() { return model_view_projection_; }
  const mat4 &model_view_projection() const { return model_view_projection_; }

  mat4 &model() { return model_; }
  const mat4 &model() const { return model_; }

  vec4 &color() { return color_; }
  const vec4 &color() const { return color_; }

  vec3 &light_pos() { return light_pos_; }
  const vec3 &light_pos() const { return light_pos_; }

  vec3 &camera_pos() { return camera_pos_; }
  const vec3 &camera_pos() const { return camera_pos_; }

  std::string &last_error() { return last_error_; }
  const std::string &last_error() const { return last_error_; }

  vec2i &window_size() { return window_size_; }
  const vec2i &window_size() const { return window_size_; }

 private:
  GLuint CompileShader(GLenum stage, GLuint program, const GLchar *source);

  // The mvp. Use the Ortho() and Perspective() methods in mathfu::Matrix
  // to conveniently change the camera.
  mat4 model_view_projection_;
  mat4 model_;
  vec4 color_;
  vec3 light_pos_;
  vec3 camera_pos_;

  vec2i window_size_;

  std::string last_error_;

  SDL_Window *window_;
  SDL_GLContext context_;

  BlendMode blend_mode_;
};

}  // namespace fpl

#endif  // FPL_RENDERER_H
