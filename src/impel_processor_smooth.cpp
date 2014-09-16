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
#include "impel_processor_smooth.h"

namespace impel {

IMPEL_INIT_INSTANTIATE(SmoothImpelInit);

float SmoothImpelProcessor::CalculateVelocity(ImpelTime delta_time,
                                              const ImpelData& d) const {
  const float dist = d.init.Normalize(d.target_value - d.value);
  const bool at_target = d.init.AtTarget(dist, d.velocity);
  if (at_target)
    return 0.0f;

  const float abs_dist = fabs(dist);
  const float decel_time = abs_dist / d.init.decel;
  const float decel_dist = 0.5f * abs_dist * decel_time;
  const bool should_decel = decel_dist >= dist;
  const float accel = should_decel ? -d.init.decel : d.init.accel;
  const float velocity_unclamped = d.velocity + accel * delta_time;
  const float velocity = mathfu::Clamp(
      velocity_unclamped, -d.init.max_velocity, d.init.max_velocity);
  return velocity;
}

} // namespace impel

