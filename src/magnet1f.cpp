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

#include "magnet1f.h"

namespace fpl {

static bool IsSettled1f(float diff, float velocity,
                        const MagnetSettled1f& settled) {
  return fabs(velocity) <= settled.max_velocity() &&
         fabs(diff) <= settled.max_difference();
}

// Ensure position 'x' is within the valid constraint range.
float Magnet1f::Normalize(float x) const {
  const float min = constraints_->min();
  const float max = constraints_->max();

  if (constraints_->modular()) {
    // For modular values, ensures 'x' is in the constraints (min, max].
    // That is, exclusive of min, inclusive of max.
    const float width = max - min;
    const float above_min = x <= min ? x + width : x;
    const float normalized = above_min > max ? above_min - width : above_min;
    assert(min < normalized && normalized <= max);
    return normalized;

  } else {
    // For non-modular values, clamp 'x' to the valid range.
    return mathfu::Clamp(x, min, max);
  }
}

float Magnet1f::CalculateDifference() const {
  return Normalize(target_.position() - state_.position);
}

float Magnet1f::CalculatePosition(WorldTime delta_time, float velocity) const {
  const float delta = mathfu::Clamp(delta_time * velocity,
                                    -constraints_->max_delta(),
                                    constraints_->max_delta());
  const float position = Normalize(state_.position + delta);
  return position;
}

float Magnet1f::ClampVelocity(float velocity) const {
  return mathfu::Clamp(velocity, -constraints_->max_velocity(),
                       constraints_->max_velocity());
}

float OvershootMagnet1f::CalculateVelocity(WorldTime delta_time) const {
  // Increment our current face angle velocity.
  // If we're moving in the wrong direction (i.e. away from the target),
  // increase the acceleration. This results in us moving towards the target
  // for longer time than we move away from the target, or equivalently,
  // aggressively initiating our movement towards the target, which feels good.
  const float diff = CalculateDifference();
  const bool wrong_direction = state_.velocity * diff < 0.0f;
  const float wrong_direction_multiplier = wrong_direction ?
      def_->wrong_direction_acceleration_multiplier() : 1.0f;
  const float acceleration = diff * def_->acceleration_per_difference() *
                             wrong_direction_multiplier;
  const float velocity_unclamped = state_.velocity + delta_time * acceleration;
  const float velocity = ClampVelocity(velocity_unclamped);

  // If we're far from facing the target, use the velocity calculated above.
  const bool should_snap = IsSettled1f(diff, velocity, *def_->snap_settled());
  if (should_snap) {
    const float max_snap_velocity = def_->snap_settled()->max_velocity();
    const float snap_velocity = mathfu::Clamp(
        diff / delta_time, -max_snap_velocity, max_snap_velocity);
    return snap_velocity;
  }

  return velocity;
}

float OvershootMagnet1f::CalculateTwitchVelocity(MagnetTwitch twitch) const {
  if (twitch == kMagnetTwitchNone)
    return state_.velocity;

  // If we're close to being settled, give a boost to the velocity if a twich
  // is requested. Twitches are useful for responding to inputs without actually
  // changing the target.
  const float diff = CalculateDifference();
  const bool should_twitch = IsSettled1f(diff, state_.velocity,
                                         *def_->twitch_settled());
  if (!should_twitch)
    return state_.velocity;

  const float twitch_velocity = def_->twitch_velocity_boost();
  return twitch == kMagnetTwitchPositive ? twitch_velocity : -twitch_velocity;
}

void OvershootMagnet1f::AdvanceFrame(WorldTime delta_time) {
  state_.velocity = CalculateVelocity(delta_time);
  state_.position = CalculatePosition(delta_time, state_.velocity);
}

void OvershootMagnet1f::Twitch(MagnetTwitch twitch) {
  state_.velocity = CalculateTwitchVelocity(twitch);
}

bool OvershootMagnet1f::Settled() const {
  const float diff = CalculateDifference();
  return IsSettled1f(diff, state_.velocity, *def_->snap_settled());
}

} // namespace fpl
