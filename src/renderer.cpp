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

#include "precompiled.h"
#include "renderer.h"

#include "webp/decode.h"

namespace fpl {

bool Renderer::Initialize(const vec2i &window_size, const char *window_title) {
  // Basic SDL initialization, does not actually initialize a Window or OpenGL,
  // typically should not fail.
  SDL_SetMainReady();
  if (SDL_Init(SDL_INIT_VIDEO)) {
    last_error_ = std::string("SDL_Init fail: ") + SDL_GetError();
    return false;
  }

  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);

  // Force OpenGL ES 2 on mobile.
  #ifdef PLATFORM_MOBILE
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  #else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
  #endif

  // Always double buffer.
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  // Create the window:
  window_ = SDL_CreateWindow(
    window_title,
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    window_size.x(), window_size.y(),
    SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN |
      #ifdef PLATFORM_MOBILE
        SDL_WINDOW_BORDERLESS
      #else
        SDL_WINDOW_RESIZABLE
        #ifndef _DEBUG
          //| SDL_WINDOW_FULLSCREEN_DESKTOP
        #endif
      #endif
    );
  if (!window_) {
    last_error_ = std::string("SDL_CreateWindow fail: ") + SDL_GetError();
    return false;
  }
  // Get the size we actually got, which typically is native res for
  // any fullscreen display:
  SDL_GetWindowSize(window_, &window_size_.x(), &window_size_.y());

  // Create the OpenGL context:
  context_ = SDL_GL_CreateContext(window_);
  if (!context_) {
    last_error_ = std::string("SDL_GL_CreateContext fail: ") + SDL_GetError();
    return false;
  }

  // Enable Vsync on desktop
  #ifndef PLATFORM_MOBILE
    SDL_GL_SetSwapInterval(1);
  #endif

  #ifndef PLATFORM_MOBILE
  auto exts = (char *)glGetString(GL_EXTENSIONS);

  if (!strstr(exts, "GL_ARB_vertex_buffer_object") ||
      !strstr(exts, "GL_ARB_multitexture") ||
      !strstr(exts, "GL_ARB_vertex_program") ||
      !strstr(exts, "GL_ARB_fragment_program")) {
    last_error_ = "missing GL extensions";
    return false;
  }

  #endif

  #if !defined(PLATFORM_MOBILE) && !defined(__APPLE__)
  #define GLEXT(type, name) \
    union { \
      void* data; \
      type function; \
    } data_function_union_##name; \
    data_function_union_##name.data = SDL_GL_GetProcAddress(#name); \
    if (!data_function_union_##name.data) { \
      last_error_ = "could not retrieve GL function pointer " #name; \
      return false; \
    } \
    name = data_function_union_##name.function;
      GLBASEEXTS GLEXTS
  #undef GLEXT
  #endif

  blend_mode_ = kBlendModeOff;
  return true;
}

void Renderer::AdvanceFrame(bool minimized) {
  if (minimized) {
    // Save some cpu / battery:
    SDL_Delay(10);
  } else {
    SDL_GL_SwapWindow(window_);
  }
  GL_CALL(glViewport(0, 0, window_size_.x(), window_size_.y()));
  GL_CALL(glEnable(GL_DEPTH_TEST));
}

void Renderer::ShutDown() {
  if (context_) {
    SDL_GL_DeleteContext(context_);
    context_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
}

void Renderer::ClearFrameBuffer(const vec4 &color) {
    GL_CALL(glClearColor(color.x(), color.y(), color.z(), color.w()));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

GLuint Renderer::CompileShader(GLenum stage, GLuint program,
                               const GLchar *source) {
  std::string platform_source =
  #ifdef PLATFORM_MOBILE
    "#ifdef GL_ES\nprecision highp float;\n#endif\n";
  #else
    "#version 120\n";
  #endif
  platform_source += source;
  const char *platform_source_ptr = platform_source.c_str();
  auto shader_obj = glCreateShader(stage);
  GL_CALL(glShaderSource(shader_obj, 1, &platform_source_ptr, nullptr));
  GL_CALL(glCompileShader(shader_obj));
  GLint success;
  GL_CALL(glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success));
  if (success) {
    GL_CALL(glAttachShader(program, shader_obj));
    return shader_obj;
  } else {
    GLint length = 0;
    GL_CALL(glGetShaderiv(shader_obj, GL_INFO_LOG_LENGTH, &length));
    last_error_.assign(length, '\0');
    GL_CALL(glGetShaderInfoLog(shader_obj, length, &length, &last_error_[0]));
    GL_CALL(glDeleteShader(shader_obj));
    return 0;
  }
}

Shader *Renderer::CompileAndLinkShader(const char *vs_source,
                                       const char *ps_source) {
  auto program = glCreateProgram();
  auto vs = CompileShader(GL_VERTEX_SHADER, program, vs_source);
  if (vs) {
    auto ps = CompileShader(GL_FRAGMENT_SHADER, program, ps_source);
    if (ps) {
      GL_CALL(glBindAttribLocation(program, Mesh::kAttributePosition,
                                   "aPosition"));
      GL_CALL(glBindAttribLocation(program, Mesh::kAttributeNormal,
                                   "aNormal"));
      GL_CALL(glBindAttribLocation(program, Mesh::kAttributeTangent,
                                   "aTangent"));
      GL_CALL(glBindAttribLocation(program, Mesh::kAttributeTexCoord,
                                   "aTexCoord"));
      GL_CALL(glBindAttribLocation(program, Mesh::kAttributeColor,
                                   "aColor"));
      GL_CALL(glLinkProgram(program));
      GLint status;
      GL_CALL(glGetProgramiv(program, GL_LINK_STATUS, &status));
      if (status == GL_TRUE) {
        auto shader = new Shader(program, vs, ps);
        GL_CALL(glUseProgram(program));
        shader->InitializeUniforms();
        return shader;
      }
      GLint length = 0;
      GL_CALL(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
      last_error_.assign(length, '\0');
      GL_CALL(glGetProgramInfoLog(program, length, &length, &last_error_[0]));
      GL_CALL(glDeleteShader(ps));
    }
    GL_CALL(glDeleteShader(vs));
  }
  GL_CALL(glDeleteProgram(program));
  return nullptr;
}

Texture *Renderer::CreateTexture(const uint8_t *buffer, const vec2i &size) {
  // TODO: support default args for mipmap/wrap
  GLuint texture_id;
  GL_CALL(glGenTextures(1, &texture_id));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, texture_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x(), size.y(), 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, buffer));
  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  return new Texture(texture_id, size);
}

Texture *Renderer::CreateTextureFromTGAMemory(const void *tga_buf) {
  struct TGA {
    unsigned char id_len, color_map_type, image_type, color_map_data[5];
    unsigned short x_origin, y_origin, width, height;
    unsigned char bpp, image_descriptor;
  };
  static_assert(sizeof(TGA) == 18,
    "Members of struct TGA need to be packed with no padding.");
  int little_endian = 1;
  if (!*reinterpret_cast<char *>(&little_endian)) {
    return nullptr; // TODO: endian swap the shorts instead
  }
  auto header = reinterpret_cast<const TGA *>(tga_buf);
  if (header->color_map_type != 0 // no color map
   || header->image_type != 2 // RGB or RGBA only
   || (header->bpp != 32 && header->bpp != 24))
    return nullptr;
  auto pixels = reinterpret_cast<const unsigned char *>(header + 1);
  pixels += header->id_len;
  int size = header->width * header->height;
  auto rgba = new unsigned char[size * 4];
  int start_y, end_y, y_direction;
  if (header->image_descriptor & 0x20)  // y is flipped
  {
      start_y = header->height - 1;
      end_y = -1;
      y_direction = -1;
  } else {    // y is not flipped.
      start_y = 0;
      end_y = header->height;
      y_direction = 1;
  }
  for (int y = start_y; y != end_y; y += y_direction) {
    for (int x = 0; x < header->width; x++) {
      auto p = rgba + (y * header->width + x) * 4;
      p[2] = *pixels++;    // BGR -> RGB
      p[1] = *pixels++;
      p[0] = *pixels++;
      p[3] = header->bpp == 32 ? *pixels++ : 255;
    }
  }
  auto tex = CreateTexture(rgba, vec2i(header->width, header->height));
  delete[] rgba;
  return tex;
}

Texture *Renderer::CreateTextureFromWebpMemory(const void *webp_buf,
                                               size_t size) {
  int width = 0, height = 0;
  auto data = WebPDecodeRGBA(static_cast<const uint8_t *>(webp_buf), size,
                             &width, &height);
  return data ? CreateTexture(data, vec2i(width, height)) : nullptr;
}

void Renderer::DepthTest(bool on) {
  if (on) {
    GL_CALL(glEnable(GL_DEPTH_TEST));
  } else {
    GL_CALL(glDisable(GL_DEPTH_TEST));
  }
}

void Renderer::SetBlendMode(BlendMode blend_mode, float amount) {
  if (blend_mode == blend_mode_)
    return;

  // Disable current blend mode.
  switch (blend_mode_) {
    case kBlendModeOff:
      break;
    case kBlendModeTest:
#     ifndef PLATFORM_MOBILE  // Alpha test not supported in ES 2.
      GL_CALL(glDisable(GL_ALPHA_TEST));
      break;
#     endif
    case kBlendModeAlpha:
      GL_CALL(glDisable(GL_BLEND));
      break;
    default:
      assert(false); // Not yet implemented
      break;
  }

  // Enable new blend mode.
  switch (blend_mode) {
    case kBlendModeOff:
      break;
    case kBlendModeTest:
#     ifndef PLATFORM_MOBILE
      GL_CALL(glEnable(GL_ALPHA_TEST));
      GL_CALL(glAlphaFunc(GL_GREATER, amount));
      break;
#     endif
    case kBlendModeAlpha:
      GL_CALL(glEnable(GL_BLEND));
      GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
      break;
    default:
      assert(false); // Not yet implemented
      break;
  }

  // Remember new mode as the current mode.
  blend_mode_ = blend_mode;
}

}  // namespace fpl

void LogGLError(const char *file, int line, const char *call) {
  auto err = glGetError();
  if (err == GL_NO_ERROR) return;
  const char *err_str = "<unknown error enum>";
  switch (err) {
    case GL_INVALID_ENUM: err_str = "GL_INVALID_ENUM"; break;
    case GL_INVALID_VALUE: err_str = "GL_INVALID_VALUE"; break;
    case GL_INVALID_OPERATION: err_str = "GL_INVALID_OPERATION"; break;
    case GL_INVALID_FRAMEBUFFER_OPERATION:
      err_str = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
    case GL_OUT_OF_MEMORY: err_str = "GL_OUT_OF_MEMORY"; break;
  }
  SDL_LogError(SDL_LOG_CATEGORY_ERROR,
               "%s(%d): OpenGL Error: %s from %s", file, line, err_str, call);
  assert(0);
}

#if !defined(PLATFORM_MOBILE) && !defined(__APPLE__)
  #define GLEXT(type, name) type name = nullptr;
    GLBASEEXTS GLEXTS
  #undef GLEXT
#endif

