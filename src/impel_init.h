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

enum MatrixOperationType {
  kInvalidMatrixOperation,
  kRotateAboutX,
  kRotateAboutY,
  kRotateAboutZ,
  kTranslateX,
  kTranslateY,
  kTranslateZ,
  kScaleX,
  kScaleY,
  kScaleZ,
  kScaleUniformly,
};

// ImpelProcessors that derive from ImpelProcessorWithVelocity should have an
// ImpelInit that derives from this struct (ImpelInitWithVelocity).
class ModularImpelInit : public ImpelInit {
 public:
  // The derived type must call this constructor with it's ImpelType identifier.
  explicit ModularImpelInit(ImpellerType type)
      : ImpelInit(type), min_(0.0f), max_(0.0f), modular_(false) {}

  // Ensure position 'x' is within the valid constraint range.
  float Normalize(float x) const {
    // For non-modular values, do nothing.
    if (!modular_) return x;

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
  // Minimum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float min_;

  // Maximum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float max_;

  // A modular value wraps around from min to max. For example, an angle
  // is modular, where -pi is equivalent to +pi. Setting this to true ensures
  // that arithmetic wraps around instead of clamping to min/max.
  bool modular_;
};

class OvershootImpelInit : public ModularImpelInit {
 public:
  IMPEL_INTERFACE();

  OvershootImpelInit()
      : ModularImpelInit(kType),
        max_velocity_(0.0f),
        accel_per_difference_(0.0f),
        wrong_direction_multiplier_(0.0f),
        max_delta_time_(0) {}

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
  float wrong_direction_multiplier() const {
    return wrong_direction_multiplier_;
  }
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

struct MatrixOperationInit {
  // Matrix operation never changes. Always use 'const_value'.
  MatrixOperationInit(MatrixOperationType type, float const_value)
      : init(nullptr),
        type(type),
        has_initial_value(true),
        initial_value(const_value) {}

  // Matrix operation is driven by Impeller defined by 'init'.
  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init)
      : init(&init),
        type(type),
        has_initial_value(false),
        initial_value(0.0f) {}

  // Matrix operation is driven by Impeller defined by 'init'. Specify initial
  // value as well.
  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init,
                      float initial_value)
      : init(&init),
        type(type),
        has_initial_value(true),
        initial_value(initial_value) {}

  const ImpelInit* init;
  MatrixOperationType type;
  bool has_initial_value;
  float initial_value;
};

// Initialize an ImpellerMatrix4f with these initialization parameters to
// create an impeller that generates a 4x4 matrix from a series of basic
// matrix operations. The basic matrix operations are driven by 1 dimensional
// impellers.
//
// The series of operations can transform an object from the coordinate space
// in which it was authored, to world (or local) space. For example, if you
// have a penguin that is authored at (0,0,0) facing up the x-axis, you can
// move it to it's target position with four operations:
//
//      kScaleUniformly --> to make penguin the correct size
//      kRotateAboutY --> to make penguin face the correct direction
//      kTranslateX } --> to move penguin along to ground to target position
//      kTranslateZ }
class MatrixImpelInit : public ImpelInit {
 public:
  IMPEL_INTERFACE();
  typedef typename std::vector<MatrixOperationInit> OpVector;

  // By default expect a relatively high number of ops. Cost for allocating
  // a bit too much temporary memory is small compared to cost of reallocating
  // that memory.
  explicit MatrixImpelInit(int expected_num_ops = 8) : ImpelInit(kType) {
    ops_.reserve(expected_num_ops);
  }

  void Clear() { ops_.clear(); }

  // Operation is constant. For example, use to put something flat on the
  // ground, with 'type' = kRotateAboutX and 'const_value' = pi/2.
  void AddOp(MatrixOperationType type, float const_value) {
    ops_.push_back(MatrixOperationInit(type, const_value));
  }

  // Operation is driven by a 1-dimensional impeller. For example, you can
  // control the face angle of a standing object with 'type' = kRotateAboutY
  // and 'init' a curve specified by SmoothImpelInit.
  void AddOp(MatrixOperationType type, const ImpelInit& init) {
    ops_.push_back(MatrixOperationInit(type, init));
  }

  // Operation is driven by a 1-dimensional impeller, and initial value
  // is specified.
  void AddOp(MatrixOperationType type, const ImpelInit& init,
             float initial_value) {
    ops_.push_back(MatrixOperationInit(type, init, initial_value));
  }

  const OpVector& ops() const { return ops_; }

 private:
  OpVector ops_;
};

}  // namespace impel

#endif  // IMPEL_INIT_H_
