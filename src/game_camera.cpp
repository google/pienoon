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
#include "config_generated.h"
#include "common.h"
#include "impel_flatbuffers.h"
#include "impel_processor_overshoot.h"
#include "impel_processor_smooth.h"
#include "game_camera.h"
#include "utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace splat {

vec3 GameCamera::Position() const {
  return percent_.Valid() ?
         vec3::Lerp(start_.position, end_.position, percent_.Value()) :
         start_.position;
}

vec3 GameCamera::Target() const {
  return percent_.Valid() ?
         vec3::Lerp(start_.target, end_.target, percent_.Value()) :
         start_.target;
}

void GameCamera::Initialize(const GameCameraState& state,
                            impel::ImpelEngine* engine) {
  engine_ = engine;
  start_ = state;
  end_ = state;
  percent_.Invalidate();
  movements_ = std::queue<GameCameraMovement>();
  AdvanceFrame(0);
}

void GameCamera::AdvanceFrame(WorldTime /*delta_time*/) {
  // Update the directional vectors.
  GameCameraState current = CurrentState();
  forward_ = (current.target - current.position).Normalized();
  side_ = vec3::CrossProduct(mathfu::kAxisY3f, forward_);

  // If the camera has finished zooming in, transition to zoom out.
  // Transition to next movement that's been queued.
  if (!movements_.empty() && (!percent_.Valid() || percent_.Value() == 1.0f)) {
    ExecuteMovement(movements_.front());
    movements_.pop();
  }
}

void GameCamera::ExecuteMovement(const GameCameraMovement& movement) {
  // We interpolate between start_ and end_, so start_ should be the current
  // values.
  start_ = CurrentState();
  end_ = movement.end;

  // Initialize the Impeller.
  percent_.Initialize(movement.init, engine_);
  percent_.SetValue(0.0f);
  percent_.SetVelocity(movement.start_velocity);
  percent_.SetTargetValue(1.0f);
  percent_.SetTargetTime(movement.time);
}

void GameCamera::TerminateMovements() {
  GameCameraState state = CurrentState();
  start_ = state;
  end_ = state;
  if (percent_.Valid()) {
    percent_.SetValue(1.0f);
    percent_.SetTargetValue(1.0f);
    percent_.SetVelocity(0.0f);
  }
  movements_ = std::queue<GameCameraMovement>();
}

// Used for debugging. Haults animation and sets the camera position.
void GameCamera::OverridePosition(const vec3& position) {
  TerminateMovements();
  GameCameraState current = CurrentState();
  const vec3 delta = position - current.position;
  start_ = GameCameraState(current.position, current.target + delta);
  end_ = start_;
}

// Used for debugging. Haults animation and sets the camera target.
void GameCamera::OverrideTarget(const vec3& target) {
  TerminateMovements();
  start_.target = target;
  end_.target = target;
}

} // splat
} // fpl
