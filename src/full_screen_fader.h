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

#ifndef FULL_SCREEN_FADER_H
#define FULL_SCREEN_FADER_H

#include "common.h"
#include "precompiled.h"

namespace fpl {

namespace pie_noon {

// Renders a full screen overlay that transitions to opaque then back to
// transparent.
class FullScreenFader {
 public:
  FullScreenFader(fplbase::Renderer* renderer);
  ~FullScreenFader() {}

  // Starts the fade.
  void Start(const WorldTime& time, const WorldTime& fade_time,
             const mathfu::vec4& color, const bool fade_in);

  // Renders the fade overlay, returning true on the frame the overlay
  // is opaque.
  bool Render(const WorldTime& time);

  // Returns true when the fade is complete (overlay is transparent).
  bool Finished(const WorldTime& time) const {
    return !fade_in_ && CalculateOffset(time) >= 1.0f;
  }

  void set_material(fplbase::Material* material) { material_ = material; }
  fplbase::Material* material() const { return material_; }
  void set_shader(fplbase::Shader* shader) { shader_ = shader; }
  fplbase::Shader* shader() const { return shader_; }
  void set_ortho_mat(const mathfu::mat4& ortho_mat) { ortho_mat_ = ortho_mat; }
  const mathfu::mat4& ortho_mat() const { return ortho_mat_; }
  void set_extents(const mathfu::vec2i& extents) { extents_ = extents; }
  const mathfu::vec2i& extents() const { return extents_; }

 private:
  // Calculate offset from the fade start time scaled by fade time.
  inline float CalculateOffset(const WorldTime& time) const {
    return static_cast<float>(time - start_time_) /
           static_cast<float>(half_fade_time_);
  }

  // Time when the fade started.
  WorldTime start_time_;
  // Half the complete fade time.
  WorldTime half_fade_time_;
  // Whether the fader is fading in, false indicates it's fading out.
  bool fade_in_;
  // Color of the overlay (the alpha component is ignored).
  mathfu::vec4 color_;
  // Projection matrix.
  mathfu::mat4 ortho_mat_;
  // Extents of the fade region.
  mathfu::vec2i extents_;
  fplbase::Renderer* renderer_;
  // Material used to render the overlay.
  fplbase::Material* material_;
  // Shader used to render the overlay material.
  fplbase::Shader* shader_;
};

}  // namespace pie_noon
}  // namespace fpl

#endif  // FULL_SCREEN_FADER_H
