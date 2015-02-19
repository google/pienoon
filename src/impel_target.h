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

namespace impel {

// Impellers have a subset of this state. Not all impellers types have all
// of this state. For example, some impellers may be simple physics driven
// movement, and not support waypoints.
enum ImpelTarget1fItem {
  kValue,
  kVelocity,
  kTargetValue,
  kTargetVelocity,
  kTargetTime,
};

// Bitfields of the types above. Used in ImpelTarget1f::valid
// to specify which fields are being set in ImpelProcessor::SetState().
enum ImpelTarget1fValidity {
  kValueValid = 1 << kValue,
  kVelocityValid = 1 << kVelocity,
  kTargetValueValid = 1 << kTargetValue,
  kTargetVelocityValid = 1 << kTargetVelocity,
  kTargetTimeValid = 1 << kTargetTime,
};

// Override the current and target state for a one-dimensional Impeller.
// It is valid to set a subset of the parameters here. For example, if you
// want to continually adjust the target value of an Impeller every frame,
// you can call Impeller1f::SetTarget() with an ImpelTarget1f that has only
// the target value set.
//
// If the current value and current velocity are not specified, their current
// values in the impeller are used.
//
// If the target value and target velocity are not specified, 0 is used.
//
// If target time is not specified, a default constant is used. Note that not
// all Impeller types respect target time.
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
  ImpelTarget1f() : valid_items_(0) {}

  // Convenience constructors. To avoid having to call SetValue() and other
  // such calls on a separate line.
  explicit ImpelTarget1f(float value) { SetConstant(value); }

  ImpelTarget1f(float value, float velocity) : valid_items_(0) {
    SetValue(value);
    SetVelocity(velocity);
  }

  ImpelTarget1f(float value, float velocity, float target_value)
      : valid_items_(0) {
    SetValue(value);
    SetVelocity(velocity);
    SetTargetValue(target_value);
  }

  ImpelTarget1f(float value, float velocity, float target_value,
                float target_velocity)
      : valid_items_(0) {
    SetValue(value);
    SetVelocity(velocity);
    SetTargetValue(target_value);
    SetTargetVelocity(target_velocity);
  }

  ImpelTarget1f(float value, float velocity, float target_value,
                float target_velocity, float target_time)
      : valid_items_(0) {
    SetValue(value);
    SetVelocity(velocity);
    SetTargetValue(target_value);
    SetTargetVelocity(target_velocity);
    SetTargetTime(target_time);
  }

  void Reset() { valid_items_ = 0; }

  void SetValue(float value) {
    value_ = value;
    valid_items_ |= kValueValid;
  }

  void SetVelocity(float velocity) {
    velocity_ = velocity;
    valid_items_ |= kVelocityValid;
  }

  void SetTargetValue(float target_value) {
    target_value_ = target_value;
    valid_items_ |= kTargetValueValid;
  }

  void SetTargetVelocity(float target_velocity) {
    target_velocity_ = target_velocity;
    valid_items_ |= kTargetVelocityValid;
  }

  void SetTargetTime(float target_time) {
    target_time_ = target_time;
    valid_items_ |= kTargetTimeValid;
  }

  // Set the impeller to a constant state, where the current and target
  // values equal 'value', and the current and target velocities are 0.
  void SetConstant(float value) {
    value_ = value;
    velocity_ = 0.0f;
    target_value_ = value_;
    target_velocity_ = velocity_;
    target_time_ = 0.0f;
    valid_items_ |= kValueValid | kVelocityValid | kTargetValueValid |
                    kTargetVelocityValid | kTargetTimeValid;
  }

  // Return true if the specified item has been specified in a 'Set()' call.
  bool Valid(const ImpelTarget1fItem item) const {
    return (1 << item) & valid_items_;
  }

  // Return values that have been specified in a 'Set()' call.
  float Value() const {
    assert(Valid(kValue));
    return value_;
  }

  float Velocity() const {
    assert(Valid(kVelocity));
    return velocity_;
  }

  float TargetValue() const {
    assert(Valid(kTargetValue));
    return target_value_;
  }

  float TargetVelocity() const {
    assert(Valid(kTargetVelocity));
    return target_velocity_;
  }

  float TargetTime() const {
    assert(Valid(kTargetTime));
    return target_time_;
  }

  uint32_t valid_items() const { return valid_items_; }

 private:
  uint32_t valid_items_;  // bitfield. See ImpelTarget1fValidity.
  float target_time_;
  float value_;
  float velocity_;
  float target_value_;
  float target_velocity_;
};

}  // namespace impel

#endif  // IMPEL_TARGET_H
