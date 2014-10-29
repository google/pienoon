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

class TouchscreenButton
{
 public:
  TouchscreenButton();

  void AdvanceFrame(WorldTime delta_time,
                    InputSystem* input, vec2 window_size);

  //bool HandlePointer(Pointer pointer, vec2 window_size);
  void Render(Renderer& renderer);
  void AdvanceFrame(WorldTime delta_time);
  ButtonId GetId();
  bool WillCapturePointer(const Pointer& pointer, vec2 window_size);
  bool IsTriggered();

  Button& button() { return button_; }


  const std::vector<Material*> &up_materials() const { return up_materials_; }
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
  void set_down_material(Material* down_material) { down_material_ = down_material; }

  mathfu::vec2 up_offset() const { return up_offset_; }
  void set_up_offset(mathfu::vec2 up_offset) { up_offset_ = up_offset; }

  mathfu::vec2 down_offset() const { return down_offset_; }
  void set_down_offset(mathfu::vec2 down_offset) { down_offset_ = down_offset; }

  const ButtonDef* button_def() const { return button_def_; }
  void set_button_def(const ButtonDef* button_def) { button_def_ = button_def; }

  Shader* shader() const { return shader_; }
  void set_shader(Shader* shader) { shader_ = shader; }

  bool is_active() const { return is_active_; }
  void set_is_active(bool is_active) { is_active_ = is_active; }

  bool is_highlighted() const { return is_highlighted_; }
  void set_is_highlighted(bool is_highlighted) {
    is_highlighted_ = is_highlighted;
  }

 private:
  Button button_;
  WorldTime elapsed_time_;

  const ButtonDef* button_def_;
  Shader* shader_;

  // Textures to draw for the up/down states:
  std::vector<Material*> up_materials_;
  size_t up_current_;

  Material* down_material_;

  // Offsets to draw the textures at,
  mathfu::vec2 up_offset_;
  mathfu::vec2 down_offset_;

  bool is_active_;
  bool is_highlighted_;
};

class StaticImage
{
 public:
  StaticImage() : image_def_(nullptr), material_(nullptr), shader_(nullptr) {}
  void Initialize(const StaticImageDef& image_def, Material* material,
                  Shader* shader);
  void Render(Renderer& renderer);
  bool Valid() const {
    return image_def_ != nullptr && material_ != nullptr && shader_ != nullptr;
  }
  const StaticImageDef* image_def() const { return image_def_; }

 private:
  const StaticImageDef* image_def_;
  Material* material_;
  Shader* shader_;
};

}  // pie_noon
}  // fpl

#endif // TOUCHSCREEN_BUTTON_H
