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
#include "shader.h"
#include "renderer.h"

#ifdef _WIN32
#define snprintf(buffer, count, format, ...)\
  _snprintf_s(buffer, count, count, format, __VA_ARGS__)
#endif  // _WIN32

namespace fpl {

void Shader::InitializeUniforms() {
  // Look up variables that are standard, but still optionally present in a
  // shader.
  uniform_model_view_projection_ = glGetUniformLocation(
                                        program_, "model_view_projection");
  uniform_model_ = glGetUniformLocation(program_, "model");

  uniform_color_ = glGetUniformLocation(program_, "color");

  uniform_light_pos_ = glGetUniformLocation(program_, "light_pos");
  uniform_camera_pos_ = glGetUniformLocation(program_, "camera_pos");

  // Set up the uniforms the shader uses for texture access.
  char texture_unit_name[] = "texture_unit_#####";
  for (int i = 0; i < kMaxTexturesPerShader; i++) {
    snprintf(texture_unit_name, sizeof(texture_unit_name),
        "texture_unit_%d", i);
    auto loc = glGetUniformLocation(program_, texture_unit_name);
    if (loc >= 0) GL_CALL(glUniform1i(loc, i));
  }
}

void Shader::Set(const Renderer &renderer) const {
  GL_CALL(glUseProgram(program_));

  if (uniform_model_view_projection_ >= 0)
    GL_CALL(glUniformMatrix4fv(uniform_model_view_projection_, 1, false,
                               &renderer.model_view_projection()[0]));
  if (uniform_model_ >= 0)
    GL_CALL(glUniformMatrix4fv(uniform_model_, 1, false, &renderer.model()[0]));
  if (uniform_color_ >= 0)
    GL_CALL(glUniform4fv(uniform_color_, 1, &renderer.color()[0]));
  if (uniform_light_pos_ >= 0)
    GL_CALL(glUniform3fv(uniform_light_pos_, 1, &renderer.light_pos()[0]));
  if (uniform_camera_pos_ >= 0)
    GL_CALL(glUniform3fv(uniform_camera_pos_, 1, &renderer.camera_pos()[0]));
}

}  // namespace fpl
