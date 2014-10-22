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

#include "full_screen_fader.h"

#include "material.h"
#include "mesh.h"
#include "renderer.h"
#include "shader.h"

namespace fpl {
namespace splat {

FullScreenFader::FullScreenFader(Renderer* renderer) :
  start_time_(0), half_fade_time_(0), fade_in_(false), renderer_(renderer),
  material_(NULL), shader_(NULL) {}

// Starts the fade.
void FullScreenFader::Start(const WorldTime& time, const WorldTime& fade_time,
                            const vec4& color, const bool fade_in) {
  assert(material_ && shader_);
  fade_in_ = true;
  half_fade_time_ = fade_time / 2;
  // If we're fading out, move the start time into the past so that the fader
  // renders at least one opaque frame before fading out.
  start_time_ = fade_in ? time : time - half_fade_time_;
  color_ = color;
}

// Renders the fade overlay, returning true on the frame the overlay
// is opaque.
bool FullScreenFader::Render(const WorldTime& time) {
  // The alpha is calculated using this mini-state machine so that there
  // is always at least one frame where the overlay is complete opaque.
  const float offset = CalculateOffset(time);
  const float alpha = fade_in_ ? std::min(offset, 1.0f) :
      std::max(1.0f - offset, 0.0f);
  const bool opaque = fade_in_ && alpha == 1.0f;
  if (opaque) {
    // At the mid point, start fading out.
    start_time_ = time;
    fade_in_ = false;
  }

  // Render the overlay in front on the screen.
  renderer_->model_view_projection() =
    ortho_mat_ * mat4::FromTranslationVector(vec3(0.0f, 0.0f, 0.1f));
  renderer_->color() = vec4(0.0f, 0.0f, 0.0f, alpha);
  material_->Set(*renderer_);
  shader_->Set(*renderer_);
  Mesh::RenderAAQuadAlongX(
      vec3(0.0f, static_cast<float>(extents_.y()), 0.0f),
      vec3(static_cast<float>(extents_.x()), 0.0f, 0.0f),
      vec2(0.0f, 1.0f), vec2(1.0f, 0.0f));
  return opaque;
}

}  // namespace splat
}  // namespace fpl
