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

#include "impel_engine.h"
#include "impel_processor_overshoot.h"

namespace impel {

IMPEL_INIT_INSTANTIATE(OvershootImpelInit);

float OvershootImpelProcessor::CalculateVelocity(ImpelTime delta_time,
                                                 const ImpelData& d) const {
  // Increment our current face angle velocity.
  // If we're moving in the wrong direction (i.e. away from the target),
  // increase the acceleration. This results in us moving towards the target
  // for longer time than we move away from the target, or equivalently,
  // aggressively initiating our movement towards the target, which feels good.
  const float diff = d.init.Normalize(d.target_value - d.value);
  const bool wrong_direction = d.velocity * diff < 0.0f;
  const float wrong_direction_multiplier = wrong_direction ?
      d.init.wrong_direction_multiplier : 1.0f;
  const float acceleration = diff * d.init.accel_per_difference *
                             wrong_direction_multiplier;
  const float velocity_unclamped = d.velocity + delta_time * acceleration;

  // Always ensure the velocity remains within the valid limits.
  const float velocity = d.init.ClampVelocity(velocity_unclamped);

  // If we're far from facing the target, use the velocity calculated above.
  const bool should_snap = d.init.AtTarget(diff, velocity);
  if (should_snap) {
    const float max_velocity = d.init.at_target.max_velocity;
    return mathfu::Clamp(diff / delta_time, -max_velocity, max_velocity);
  }

  return velocity;
}

} // namespace impel

