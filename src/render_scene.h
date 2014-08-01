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

#ifndef SPLAT_RENDER_SCENE_H
#define SPLAT_RENDER_SCENE_H

#include "mathfu/glsl_mappings.h"
#include <vector>

namespace fpl {

class Renderable {
 public:
  Renderable(uint16_t id, const mathfu::mat4& world_matrix)
    : id_(id),
      world_matrix_(world_matrix)
  {}

  uint16_t id() const { return id_; }
  void set_id(uint16_t id) { id_ = id; }

  const mathfu::mat4& world_matrix() const { return world_matrix_; }
  void set_world_matrix(const mathfu::mat4& mat) { world_matrix_ = mat; }

 private:
  // Unique identifier for item to be rendered.
  // See renderable_id in timeline_generated.h.
  uint16_t id_;

  // Position and orientation of item.
  mathfu::mat4 world_matrix_;
};

class RenderScene {
 public:
  const mathfu::mat4& camera() const { return camera_; }
  void set_camera(const mathfu::mat4& camera) { camera_ = camera; }

  std::vector<Renderable>& renderables() { return renderables_; }
  const std::vector<Renderable>& renderables() const { return renderables_; }

  std::vector<mathfu::vec3>& lights() { return lights_; }
  const std::vector<mathfu::vec3>& lights() const { return lights_; }

 private:
  // The camera position, orientation, fov.
  mathfu::mat4 camera_;

  // Array of items to be rendered and their positions.
  std::vector<Renderable> renderables_;

  // Array of positions for where to place point lights.
  std::vector<mathfu::vec3> lights_;
};

} // namespace fpl

#endif // SPLAT_RENDER_SCENE_H
