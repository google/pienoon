// Copyright 2015 Google Inc. All rights reserved.
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

#ifndef IMPEL_TARGET_H
#define IMPEL_TARGET_H

#include "range.h"

namespace impel {

struct ImpelNode1f {
  float value;
  float velocity;
  ImpelTime time;
  fpl::ModularDirection direction;

  ImpelNode1f() : value(0.0f), velocity(0.0f), time(0),
                  direction(fpl::kDirectionClosest) {}
  ImpelNode1f(float value, float velocity, ImpelTime time,
              fpl::ModularDirection direction = fpl::kDirectionClosest)
      : value(value), velocity(velocity), time(time), direction(direction) {}
};

// Override the current and/or target state for a one-dimensional Impeller.
// It is valid to set a subset of the parameters here. For example, if you
// want to continually adjust the target value of an Impeller every frame,
// you can call Impeller1f::SetTarget() with an ImpelTarget1f that has only
// the target value set.
//
// If the current value and current velocity are not specified, their current
// values in the impeller are used.
//
// An Impeller's target is set in bulk via the SetTarget call. All the state
// is set in one call because SetTarget will generally involve a lot of
// initialization work. We don't want that initialization to happen twice on
// one frame if we set both 'value' and 'velocity', for instance, so we
// aggregate all the target values into this class, and have only one
// SetTarget() function.
//
class ImpelTarget1f {
 public:
  static const int kMaxNodes = 3;

  ImpelTarget1f() : num_nodes_(0) {}

  explicit ImpelTarget1f(const ImpelNode1f& n0)
      : num_nodes_(1) {
    nodes_[0] = n0;
  }

  ImpelTarget1f(const ImpelNode1f& n0, const ImpelNode1f& n1)
      : num_nodes_(2) {
    assert(0 <= n0.time && n0.time < n1.time);
    nodes_[0] = n0;
    nodes_[1] = n1;
  }

  ImpelTarget1f(const ImpelNode1f& n0, const ImpelNode1f& n1,
                const ImpelNode1f& n2)
      : num_nodes_(3) {
    assert(0 <= n0.time && n0.time < n1.time && n1.time < n2.time);
    nodes_[0] = n0;
    nodes_[1] = n1;
    nodes_[2] = n2;
  }

  // Empty the target of all nodes.
  void Reset() { num_nodes_ = 0; }

  const ImpelNode1f& Node(int node_index) const {
    assert(0 <= node_index && node_index < num_nodes_);
    return nodes_[node_index];
  }

  fpl::Range ValueRange(float start_value) const {
    assert(num_nodes_ > 0);
    float min = start_value;
    float max = start_value;
    for (int i = 0; i < num_nodes_; ++i) {
      min = std::min(nodes_[i].value, min);
      max = std::max(nodes_[i].value, max);
    }
    return fpl::Range(min, max);
  }

  ImpelTime EndTime() const {
    assert(num_nodes_ > 0);
    return nodes_[num_nodes_ - 1].time;
  }

  int num_nodes() const { return num_nodes_; }

 private:
  int num_nodes_;
  ImpelNode1f nodes_[kMaxNodes];
};


// Set the Impeller's current values. Target values are reset to be the same
// as the new current values.
inline ImpelTarget1f Current1f(float current_value,
                               float current_velocity = 0.0f) {
  return ImpelTarget1f(ImpelNode1f(current_value, current_velocity, 0));
}

// Keep the Impeller's current values, but set the Impeller's target values.
// If Impeller uses modular arithmetic, traverse from the current to the target
// according to 'direction'.
inline ImpelTarget1f Target1f(
    float target_value, float target_velocity, ImpelTime target_time,
    fpl::ModularDirection direction = fpl::kDirectionClosest) {
  assert(target_time > 0);
  return ImpelTarget1f(ImpelNode1f(target_value, target_velocity, target_time,
                                   direction));
}

// Set both the current and target values for an Impeller.
inline ImpelTarget1f CurrentToTarget1f(
    float current_value, float current_velocity,
    float target_value, float target_velocity, ImpelTime target_time,
    fpl::ModularDirection direction = fpl::kDirectionClosest) {
  return ImpelTarget1f(ImpelNode1f(current_value, current_velocity, 0),
                       ImpelNode1f(target_value, target_velocity, target_time,
                                   direction));
}

// Move from the current value to the target value at a constant speed.
inline ImpelTarget1f CurrentToTargetConstVelocity1f(
    float current_value, float target_value, ImpelTime target_time) {
  assert(target_time > 0);
  const float velocity = (target_value - current_value) / target_time;
  return ImpelTarget1f(ImpelNode1f(current_value, velocity, 0),
                       ImpelNode1f(target_value, velocity, target_time,
                                   fpl::kDirectionDirect));
}

// Keep the Impeller's current values, but set two targets for the Impeller.
// After the first target, go on to the next.
inline ImpelTarget1f TargetToTarget1f(
    float target_value, float target_velocity, ImpelTime target_time,
    float third_value, float third_velocity, ImpelTime third_time) {
  return ImpelTarget1f(
             ImpelNode1f(target_value, target_velocity, target_time),
             ImpelNode1f(third_value, third_velocity, third_time));
}

// Set the Impeller's current values, and two targets afterwards.
inline ImpelTarget1f CurrentToTargetToTarget1f(
    float current_value, float current_velocity,
    float target_value, float target_velocity, ImpelTime target_time,
    float third_value, float third_velocity, ImpelTime third_time) {
  return ImpelTarget1f(
             ImpelNode1f(current_value, current_velocity, 0),
             ImpelNode1f(target_value, target_velocity, target_time),
             ImpelNode1f(third_value, third_velocity, third_time));
}

}  // namespace impel

#endif  // IMPEL_TARGET_H
