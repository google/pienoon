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

#ifndef IMPEL_INIT_H_
#define IMPEL_INIT_H_

#include "impel_util.h"


namespace impel {

// ImpelProcessors that derive from ImpelProcessorWithVelocity should have an
// ImpelInit that derives from this struct (ImpelInitWithVelocity).
class ModularImpelInit : public ImpelInit {
 public:
  // The derived type must call this constructor with it's ImpelType identifier.
  explicit ModularImpelInit(ImpellerType type) :
      ImpelInit(type),
      modular_(false),
      min_(0.0f),
      max_(0.0f) {
  }

  // Ensure position 'x' is within the valid constraint range.
  float Normalize(float x) const {
    // For non-modular values, do nothing.
    if (!modular_)
      return x;

    // For modular values, ensures 'x' is in the constraints (min, max].
    // That is, exclusive of min, inclusive of max.
    const float width = max_ - min_;
    const float above_min = x <= min_ ? x + width : x;
    const float normalized = above_min > max_ ? above_min - width : above_min;
    assert(min_ < normalized && normalized <= max_);
    return normalized;
  }

  // Ensure the impeller value is within the specified range.
  float ClampValue(float value) const {
    return mathfu::Clamp(value, min_, max_);
  }

  bool modular() const { return modular_; }
  float min() const { return min_; }
  float max() const { return max_; }
  void set_modular(bool modular) { modular_ = modular; }
  void set_min(float min) { min_ = min; }
  void set_max(float max) { max_ = max; }

 private:
  // A modular value wraps around from min to max. For example, an angle
  // is modular, where -pi is equivalent to +pi. Setting this to true ensures
  // that arithmetic wraps around instead of clamping to min/max.
  bool modular_;

  // Minimum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float min_;

  // Maximum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float max_;
};


class OvershootImpelInit : public ModularImpelInit {
 public:
  IMPEL_INTERFACE();

  OvershootImpelInit() :
      ModularImpelInit(kType),
      max_velocity_(0.0f),
      accel_per_difference_(0.0f),
      wrong_direction_multiplier_(0.0f),
      max_delta_time_(0)
  {}

  // Ensure velocity is within the reasonable limits.
  float ClampVelocity(float velocity) const {
    return mathfu::Clamp(velocity, -max_velocity_, max_velocity_);
  }

  // Ensure the Impeller's 'value' doesn't increment by more than 'max_delta'.
  // This is different from ClampVelocity because it is independent of time.
  // No matter how big the timestep, the delta will not be too great.
  float ClampDelta(float delta) const {
    return mathfu::Clamp(delta, -max_delta_, max_delta_);
  }

  // Return true if we're close to the target and almost stopped.
  // The definition of "close to" and "almost stopped" are given by the
  // "at_target" member.
  bool AtTarget(float dist, float velocity) const {
    return at_target_.Settled(dist, velocity);
  }

  float max_velocity() const { return max_velocity_; }
  float max_delta() const { return max_delta_; }
  const Settled1f& at_target() const { return at_target_; }
  Settled1f& at_target() { return at_target_; }
  float accel_per_difference() const { return accel_per_difference_; }
  float wrong_direction_multiplier() const { return wrong_direction_multiplier_; }
  ImpelTime max_delta_time() const { return max_delta_time_; }

  void set_max_velocity(float max_velocity) { max_velocity_ = max_velocity; }
  void set_max_delta(float max_delta) { max_delta_ = max_delta; }
  void set_at_target(const Settled1f& at_target) { at_target_ = at_target; }
  void set_accel_per_difference(float accel_per_difference) {
    accel_per_difference_ = accel_per_difference;
  }
  void set_wrong_direction_multiplier(float wrong_direction_multiplier) {
    wrong_direction_multiplier_ = wrong_direction_multiplier;
  }
  void set_max_delta_time(ImpelTime max_delta_time) {
    max_delta_time_ = max_delta_time;
  }

 private:
  // Maximum speed at which the value can change. That is, maximum value for
  // the Impeller::Velocity(). In units/tick.
  // For example, if the value is an angle, then this is the max angular
  // velocity, and the units are radians/tick.
  float max_velocity_;

  // Maximum that Impeller::Value() can be altered on a single call to
  // ImpelEngine::AdvanceFrame(), regardless of velocity or delta_time.
  float max_delta_;

  // Cutoff to determine if the Impeller's current state has settled on the
  // target. Once it has settled, Value() is set to TargetValue() and Velocity()
  // is set to zero.
  Settled1f at_target_;

  // Acceleration is a multiple of abs('state_.position' - 'target_.position').
  // Bigger differences cause faster acceleration.
  float accel_per_difference_;

  // When accelerating away from the target, we multiply our acceleration by
  // this amount. We need counter-acceleration to be stronger so that the
  // amplitude eventually dies down; otherwise, we'd just have a pendulum.
  float wrong_direction_multiplier_;

  // The algorithm is iterative. When the iteration step gets too big, the
  // behavior becomes erratic. This value clamps the iteration step.
  ImpelTime max_delta_time_;
};


class SmoothImpelInit : public ModularImpelInit {
 public:
  IMPEL_INTERFACE();

  SmoothImpelInit() : ModularImpelInit(kType) {}
};


} // namespace impel

#endif // IMPEL_INIT_H_

