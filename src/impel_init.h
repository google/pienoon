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
#include "range.h"

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
      : ImpelInit(type),
        range_(-std::numeric_limits<float>::infinity(),
               std::numeric_limits<float>::infinity()),
        modular_(false) {}
  ModularImpelInit(ImpellerType type, const fpl::Range& range, bool modular)
      : ImpelInit(type), range_(range), modular_(modular) {}

  // Ensure position 'x' is within the valid constraint range.
  // 'x' must be within (max_ - min_) of the range. This is a reasonable
  // restriction in most cases (such as after an arithmetic operation).
  // For cases where 'x' may be wildly outside the range, use
  // NormalizeWildValue() instead.
  float Normalize(float x) const { return modular_ ? range_.Normalize(x) : x; }

  float NormalizeWildValue(float x) const {
    return modular_ ? range_.NormalizeWildValue(x) : x;
  }

  // Ensure the impeller value is within the specified range.
  float ClampValue(float x) const { return range_.Clamp(x); }
  float Min() const { return range_.start(); }
  float Max() const { return range_.end(); }

  const fpl::Range& range() const { return range_; }
  void set_range(const fpl::Range& r) { range_ = r; }

  bool modular() const { return modular_; }
  void set_modular(bool modular) { modular_ = modular; }

 private:
  // Minimum and maximum values for Impeller::Value().
  // Clamp (if modular_ is false) or wrap-around (if modular_ is true) when
  // we reach these boundaries.
  fpl::Range range_;

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
  SmoothImpelInit(const fpl::Range& range, bool modular)
      : ModularImpelInit(kType, range, modular) {}
};

struct MatrixOperationInit {
  enum UnionType {
    kUnionEmpty,
    kUnionInitialValue,
    kUnionTarget,
    kUnionWaypoints
  };

  // Matrix operation never changes. Always use 'const_value'.
  MatrixOperationInit(MatrixOperationType type, float const_value)
      : init(nullptr),
        type(type),
        union_type(kUnionInitialValue),
        initial_value(const_value) {}

  // Matrix operation is driven by Impeller defined by 'init'.
  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init)
      : init(&init),
        type(type),
        union_type(kUnionEmpty) {}

  // Matrix operation is driven by Impeller defined by 'init'. Specify initial
  // value as well.
  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init,
                      float initial_value)
      : init(&init),
        type(type),
        union_type(kUnionInitialValue),
        initial_value(initial_value) {}

  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init,
                      const ImpelTarget1f& target)
      : init(&init),
        type(type),
        union_type(kUnionTarget),
        target(&target) {}

  MatrixOperationInit(MatrixOperationType type, const ImpelInit& init,
                      const fpl::CompactSpline& spline, float start_time)
      : init(&init),
        type(type),
        union_type(kUnionWaypoints) {
    waypoints.spline = &spline;
    waypoints.start_time = start_time;
  }

  struct Waypoints {
    const fpl::CompactSpline* spline;
    float start_time;
  };

  const ImpelInit* init;
  MatrixOperationType type;
  UnionType union_type;
  union {
    float initial_value;
    const ImpelTarget1f* target;
    Waypoints waypoints;
  };
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

  void AddOp(MatrixOperationType type, const ImpelInit& init,
             const ImpelTarget1f& target) {
    ops_.push_back(MatrixOperationInit(type, init, target));
  }

  void AddOp(MatrixOperationType type, const ImpelInit& init,
             const fpl::CompactSpline& spline, float start_time = 0.0f) {
    ops_.push_back(MatrixOperationInit(type, init, spline, start_time));
  }

  const OpVector& ops() const { return ops_; }

 private:
  OpVector ops_;
};

}  // namespace impel

#endif  // IMPEL_INIT_H_
