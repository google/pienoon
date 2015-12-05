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

#include "common.h"
#include "config_generated.h"
#include "pie_noon_common_generated.h"
#include "precompiled.h"

namespace fpl {
namespace pie_noon {

class TouchscreenButton {
 public:
  TouchscreenButton();

  void AdvanceFrame(WorldTime delta_time, fplbase::InputSystem* input,
                    vec2 window_size);

  // bool HandlePointer(Pointer pointer, vec2 window_size);
  void Render(fplbase::Renderer& renderer);
  void AdvanceFrame(WorldTime delta_time);
  ButtonId GetId() const;
  bool WillCapturePointer(const fplbase::InputPointer& pointer,
                          vec2 window_size);
  bool IsTriggered();

  fplbase::Button& button() { return button_; }

  const std::vector<fplbase::Material*>& up_materials() const {
    return up_materials_;
  }
  void set_up_material(size_t i, fplbase::Material* up_material) {
    assert(up_material);
    if (i >= up_materials_.size()) up_materials_.resize(i + 1);
    up_materials_[i] = up_material;
  }
  void set_current_up_material(size_t which) {
    assert(which < up_materials_.size());
    up_current_ = which;
  }

  fplbase::Material* down_material() const { return down_material_; }
  void set_down_material(fplbase::Material* down_material) {
    down_material_ = down_material;
  }

  mathfu::vec2 up_offset() const { return mathfu::vec2(up_offset_); }
  void set_up_offset(mathfu::vec2 up_offset) { up_offset_ = up_offset; }

  mathfu::vec2 down_offset() const { return mathfu::vec2(down_offset_); }
  void set_down_offset(mathfu::vec2 down_offset) { down_offset_ = down_offset; }

  const ButtonDef* button_def() const { return button_def_; }
  void set_button_def(const ButtonDef* button_def) { button_def_ = button_def; }

  fplbase::Shader* inactive_shader() const { return inactive_shader_; }
  void set_inactive_shader(fplbase::Shader* inactive_shader) {
    inactive_shader_ = inactive_shader;
  }

  fplbase::Shader* shader() const { return shader_; }
  void set_shader(fplbase::Shader* shader) { shader_ = shader; }
  fplbase::Shader* debug_shader() const { return debug_shader_; }
  void set_debug_shader(fplbase::Shader* shader) { debug_shader_ = shader; }
  void DebugRender(const vec3& position, const vec3& texture_size,
                   fplbase::Renderer& renderer) const;
  void set_draw_bounds(bool enable) { draw_bounds_ = enable; };

  bool is_active() const { return is_active_; }
  void set_is_active(bool is_active) { is_active_ = is_active; }

  bool is_visible() const { return is_visible_; }
  void set_is_visible(bool is_visible) { is_visible_ = is_visible; }

  bool is_highlighted() const { return is_highlighted_; }
  void set_is_highlighted(bool is_highlighted) {
    is_highlighted_ = is_highlighted;
  }

  void set_color(const mathfu::vec4& color) { color_ = color; }
  mathfu::vec4 color() { return mathfu::vec4(color_); }

  void SetCannonicalWindowHeight(int height) {
    one_over_cannonical_window_height_ = 1.0f / static_cast<float>(height);
  }

 private:
  fplbase::Button button_;
  WorldTime elapsed_time_;

  const ButtonDef* button_def_;
  fplbase::Shader* shader_;
  fplbase::Shader* inactive_shader_;
  fplbase::Shader* debug_shader_;

  // Textures to draw for the up/down states:
  std::vector<fplbase::Material*> up_materials_;
  size_t up_current_;

  fplbase::Material* down_material_;

  // Allow overriding the default color in code.
  mathfu::vec4_packed color_;

  // Offsets to draw the textures at,
  mathfu::vec2_packed up_offset_;
  mathfu::vec2_packed down_offset_;

  bool is_active_;
  bool is_visible_;
  bool is_highlighted_;
  bool draw_bounds_;

  // Scale the textures by the y-axis so that they are (proportionally)
  // the same height on every platform.
  float one_over_cannonical_window_height_;
};

class StaticImage {
 public:
  StaticImage();
  void Initialize(const StaticImageDef& image_def,
                  std::vector<fplbase::Material*> materials,
                  fplbase::Shader* shader,
                  int cannonical_window_height);
  void Render(fplbase::Renderer& renderer);
  bool Valid() const;
  ButtonId GetId() const {
    return image_def_ == nullptr ? ButtonId_Undefined : image_def_->ID();
  }
  const StaticImageDef* image_def() const { return image_def_; }
  mathfu::vec2 scale() const { return mathfu::vec2(scale_); }
  void set_scale(const mathfu::vec2& scale) { scale_ = scale; }
  void set_current_material_index(int i) { current_material_index_ = i; }

  void set_is_visible(bool b) { is_visible_ = b; }
  bool is_visible() { return is_visible_; }

  void set_color(const mathfu::vec4& color) { color_ = color; }
  mathfu::vec4 color() { return mathfu::vec4(color_); }

  // Set the image position on screen, expressed as a fraction of the screen
  // dimensions to place the center point.
  void set_texture_position(const mathfu::vec2& position) {
    texture_position_ = position;
  }
  mathfu::vec2 texture_position() { return mathfu::vec2(texture_position_); }

 private:
  // Flatbuffer's definition of this image.
  const StaticImageDef* image_def_;

  // A list of materials that can be drawn. Choose current material with
  // set_current_material_index.
  std::vector<fplbase::Material*> materials_;

  // The material that is currently being displayed.
  int current_material_index_;

  // The shader used to render the material.
  fplbase::Shader* shader_;

  // Draw image bigger or smaller. (1.0f, 1.0f) means no scaling.
  mathfu::vec2_packed scale_;

  // Where to display the texture on screen.
  mathfu::vec2_packed texture_position_;

  // Allow overriding the default color in code.
  mathfu::vec4_packed color_;

  // Scale the textures by the y-axis so that they are (proportionally)
  // the same height on every platform.
  float one_over_cannonical_window_height_;

  bool is_visible_;
};

}  // pie_noon
}  // fpl

#endif  // TOUCHSCREEN_BUTTON_H
