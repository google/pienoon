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


#ifndef TOUCHSCREEN_BUTTON_H
#define TOUCHSCREEN_BUTTON_H

#include "precompiled.h"
#include "common.h"
#include "config_generated.h"
#include "input.h"
#include "material.h"
#include "renderer.h"
#include "splat_common_generated.h"

namespace fpl {
namespace splat {

class TouchscreenButton
{
 public:
  TouchscreenButton();

  bool HandlePointer(Pointer pointer, vec2 window_size);
  void Render(Renderer& renderer);

  bool button_state() const { return button_state_; }
  void set_button_state(bool button_state) { button_state_ = button_state; }

  vec2 position() const { return position_; }
  void set_position(vec2 position) { position_ = position; }

  Material* up_material() const { return up_material_; }
  void set_up_material(Material* up_material) { up_material_ = up_material; }

  Material* down_material() const { return down_material_; }
  void set_down_material(Material* down_material) { down_material_ = down_material; }

  mathfu::vec2 up_offset() const { return up_offset_; }
  void set_up_offset(mathfu::vec2 up_offset) { up_offset_ = up_offset; }

  mathfu::vec2 down_offset() const { return down_offset_; }
  void set_down_offset(mathfu::vec2 down_offset) { down_offset_ = down_offset; }

  const ButtonDef* button_def() const { return button_def_; }
  void set_button_def(const ButtonDef* button_def) { button_def_ = button_def; }

  Shader* shader() const { return shader_; }
  void set_shader(Shader* shader) { shader_ = shader; }

 private:
  bool button_state_;

  vec2 position_;

  const ButtonDef* button_def_;
  Shader* shader_;

  // Textures to draw for the up/down states:
  Material* up_material_;
  Material* down_material_;

  // Offsets to draw the textures at,
  mathfu::vec2 up_offset_;
  mathfu::vec2 down_offset_;
};

}  // splat
}  // fpl

#endif // TOUCHSCREEN_BUTTON_H
