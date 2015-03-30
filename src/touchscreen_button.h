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
#include "pie_noon_common_generated.h"

namespace fpl {
namespace pie_noon {

class TouchscreenButton {
 public:
  TouchscreenButton();

  void AdvanceFrame(WorldTime delta_time, InputSystem* input, vec2 window_size);

  // bool HandlePointer(Pointer pointer, vec2 window_size);
  void Render(Renderer& renderer);
  void AdvanceFrame(WorldTime delta_time);
  ButtonId GetId() const;
  bool WillCapturePointer(const Pointer& pointer, vec2 window_size);
  bool IsTriggered();

  Button& button() { return button_; }

  const std::vector<Material*>& up_materials() const { return up_materials_; }
  void set_up_material(size_t i, Material* up_material) {
    assert(up_material);
    if (i >= up_materials_.size()) up_materials_.resize(i + 1);
    up_materials_[i] = up_material;
  }
  void set_current_up_material(size_t which) {
    assert(which < up_materials_.size());
    up_current_ = which;
  }

  Material* down_material() const { return down_material_; }
  void set_down_material(Material* down_material) {
    down_material_ = down_material;
  }

  mathfu::vec2 up_offset() const { return up_offset_; }
  void set_up_offset(mathfu::vec2 up_offset) { up_offset_ = up_offset; }

  mathfu::vec2 down_offset() const { return down_offset_; }
  void set_down_offset(mathfu::vec2 down_offset) { down_offset_ = down_offset; }

  const ButtonDef* button_def() const { return button_def_; }
  void set_button_def(const ButtonDef* button_def) { button_def_ = button_def; }

  Shader* inactive_shader() const { return inactive_shader_; }
  void set_inactive_shader(Shader* inactive_shader) {
    inactive_shader_ = inactive_shader;
  }

  Shader* shader() const { return shader_; }
  void set_shader(Shader* shader) { shader_ = shader; }

  bool is_active() const { return is_active_; }
  void set_is_active(bool is_active) { is_active_ = is_active; }

  bool is_visible() const { return is_visible_; }
  void set_is_visible(bool is_visible) { is_visible_ = is_visible; }

  bool is_highlighted() const { return is_highlighted_; }
  void set_is_highlighted(bool is_highlighted) {
    is_highlighted_ = is_highlighted;
  }

  void set_color(const mathfu::vec4& color) { color_ = color; }
  const mathfu::vec4& color() { return color_; }

  void SetCannonicalWindowHeight(int height) {
    one_over_cannonical_window_height_ = 1.0f / static_cast<float>(height);
  }

 private:
  Button button_;
  WorldTime elapsed_time_;

  const ButtonDef* button_def_;
  Shader* shader_;
  Shader* inactive_shader_;

  // Textures to draw for the up/down states:
  std::vector<Material*> up_materials_;
  size_t up_current_;

  Material* down_material_;

  // Allow overriding the default color in code.
  mathfu::vec4 color_;

  // Offsets to draw the textures at,
  mathfu::vec2 up_offset_;
  mathfu::vec2 down_offset_;

  bool is_active_;
  bool is_visible_;
  bool is_highlighted_;

  // Scale the textures by the y-axis so that they are (proportionally)
  // the same height on every platform.
  float one_over_cannonical_window_height_;
};

class StaticImage {
 public:
  StaticImage();
  void Initialize(const StaticImageDef& image_def,
                  std::vector<Material*> materials, Shader* shader,
                  int cannonical_window_height);
  void Render(Renderer& renderer);
  bool Valid() const;
  ButtonId GetId() const {
    return image_def_ == nullptr ? ButtonId_Undefined : image_def_->ID();
  }
  const StaticImageDef* image_def() const { return image_def_; }
  const mathfu::vec2& scale() const { return scale_; }
  void set_scale(const mathfu::vec2& scale) { scale_ = scale; }
  void set_current_material_index(int i) { current_material_index_ = i; }

  void set_is_visible(bool b) { is_visible_ = b; }
  bool is_visible() { return is_visible_; }

  void set_color(const mathfu::vec4& color) { color_ = color; }
  const mathfu::vec4& color() { return color_; }

 private:
  // Flatbuffer's definition of this image.
  const StaticImageDef* image_def_;

  // A list of materials that can be drawn. Choose current material with
  // set_current_material_index.
  std::vector<Material*> materials_;

  // The material that is currently being displayed.
  int current_material_index_;

  // The shader used to render the material.
  Shader* shader_;

  // Draw image bigger or smaller. (1.0f, 1.0f) means no scaling.
  mathfu::vec2 scale_;

  // Allow overriding the default color in code.
  mathfu::vec4 color_;

  // Scale the textures by the y-axis so that they are (proportionally)
  // the same height on every platform.
  float one_over_cannonical_window_height_;

  bool is_visible_;
};

}  // pie_noon
}  // fpl

#endif  // TOUCHSCREEN_BUTTON_H
