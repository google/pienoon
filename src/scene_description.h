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

#ifndef PIE_NOON_SCENE_DESCRIPTION_H
#define PIE_NOON_SCENE_DESCRIPTION_H

#include "mathfu/glsl_mappings.h"
#include <memory>
#include <vector>

namespace fpl {

class Renderable {
 public:
  Renderable(uint16_t id, uint16_t variant, const mathfu::mat4& world_matrix,
             const mathfu::vec4& color = mathfu::vec4(1, 1, 1, 1))
      : id_(id), variant_(variant), world_matrix_(world_matrix), color_(color)
  {}

  uint16_t id() const { return id_; }
  void set_id(uint16_t id) { id_ = id; }

  uint16_t variant() const { return variant_; }
  void set_variant(uint16_t variant) { variant_ = variant; }

  const mathfu::mat4& world_matrix() const { return world_matrix_; }
  void set_world_matrix(const mathfu::mat4& mat) { world_matrix_ = mat; }

  const mathfu::vec4& color() const { return color_; }
  void set_color(const mathfu::vec4& c) { color_ = c; }

 private:
  // Unique identifier for item to be rendered.
  // See renderable_id in timeline_generated.h.
  uint16_t id_;

  // Variation of the renderable_id `id_` to be rendered.
  // Could be an alternate color, for example.
  uint16_t variant_;

  // Position and orientation of item.
  mathfu::mat4 world_matrix_;

  mathfu::vec4 color_;
};

class SceneDescription {
 public:
  const mathfu::mat4& camera() const { return camera_; }
  void set_camera(const mathfu::mat4& camera) { camera_ = camera; }

  std::vector<std::unique_ptr<Renderable>>& renderables() {
    return renderables_;
  }
  const std::vector<std::unique_ptr<Renderable>>& renderables() const {
    return renderables_;
  }

  std::vector<std::unique_ptr<mathfu::vec3>>& lights() { return lights_; }
  const std::vector<std::unique_ptr<mathfu::vec3>>& lights() const {
    return lights_;
  }

  // Clear out the render list. Should be called once per frame.
  void Clear() {
    renderables_.clear();
    lights_.clear();
  }

 private:
  // The camera position, orientation, fov.
  mathfu::mat4 camera_;

  // Array of items to be rendered and their positions.
  std::vector<std::unique_ptr<Renderable>> renderables_;

  // Array of positions for where to place point lights.
  std::vector<std::unique_ptr<mathfu::vec3>> lights_;
};

}  // namespace fpl

#endif  // PIE_NOON_SCENE_DESCRIPTION_H
