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

#ifndef FPL_SHADER_H
#define FPL_SHADER_H

namespace fpl {

using mathfu::vec2;
using mathfu::vec2i;
using mathfu::vec3;
using mathfu::vec3i;
using mathfu::vec4;
using mathfu::vec4i;
using mathfu::mat4;

class Renderer;

static const int kMaxTexturesPerShader = 8;

// Represents a shader consisting of a vertex and pixel shader. Also stores
// ids of standard uniforms. Use the Renderer class below to create these.
class Shader {
 public:
  Shader(GLuint program, GLuint vs, GLuint ps)
      : program_(program),
        vs_(vs),
        ps_(ps),
        uniform_model_view_projection_(-1),
        uniform_model_(-1),
        uniform_color_(-1),
        uniform_light_pos_(-1),
        uniform_camera_pos_(-1) {}

  ~Shader() {
    if (vs_) GL_CALL(glDeleteShader(vs_));
    if (ps_) GL_CALL(glDeleteShader(ps_));
    if (program_) GL_CALL(glDeleteProgram(program_));
  }

  // Will make this shader active for any subsequent draw calls, and sets
  // all standard uniforms (e.g. mvp matrix) based on current values in
  // Renderer, if this shader refers to them.
  void Set(const Renderer &renderer) const;

  // Find a non-standard uniform by name, -1 means not found.
  GLint FindUniform(const char *uniform_name) {
    GL_CALL(glUseProgram(program_));
    return glGetUniformLocation(program_, uniform_name);
  }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)  // conditional expression is constant
#endif                           // _MSC_VER
  // Set an non-standard uniform to a vec2/3/4 value.
  // Call this after Set() or FindUniform().
  template <int N>
  void SetUniform(GLint uniform_loc, const mathfu::Vector<float, N> &value) {
    // This should amount to a compile-time if-then.
    if (N == 2) {
      GL_CALL(glUniform2fv(uniform_loc, 1, &value[0]));
    } else if (N == 3) {
      GL_CALL(glUniform3fv(uniform_loc, 1, &value[0]));
    } else if (N == 4) {
      GL_CALL(glUniform4fv(uniform_loc, 1, &value[0]));
    } else {
      assert(0);
    }
  }
#ifdef _MSC_VER
#pragma warning(pop)
#endif  // _MSC_VER

  // Convenience call that does a Lookup and a Set if found.
  // Call this after Set().
  template <int N>
  bool SetUniform(const char *uniform_name,
                  const mathfu::Vector<float, N> &value) {
    auto loc = FindUniform(uniform_name);
    if (loc < 0) return false;
    SetUniform(loc, value);
    return true;
  }

  bool SetUniform(const char *uniform_name, const float &value) {
    auto loc = FindUniform(uniform_name);
    if (loc < 0) return false;
    GL_CALL(glUniform1f(loc, value));
    return true;
  }

  void InitializeUniforms();

 private:
  GLuint program_, vs_, ps_;

  GLint uniform_model_view_projection_;
  GLint uniform_model_;
  GLint uniform_color_;
  GLint uniform_light_pos_;
  GLint uniform_camera_pos_;
};

}  // namespace fpl

#endif  // FPL_SHADER_H
