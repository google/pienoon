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

#ifndef MAGNET_BASE_H_
#define MAGNET_BASE_H_

#include "common.h"

namespace fpl {

// See MagnetBase::Twich for a description of what twitch is.
enum MagnetTwitch {
  kMagnetTwitchNone,        // Do nothing.
  kMagnetTwitchPositive,    // Give the velocity a positive boost.
  kMagnetTwitchNegative     // Give the velocity a negative boost.
};

enum MagnetDirection {
  kMagnetDirectionClosest,  // Turn in the closest direction possible.
  kMagnetDirectionPositive, // Turn such that the velocity is positive.
  kMagnetDirectionNegative  // Turn such that the velocity is negative.
};

enum MagnetTargetFields {
  kMagnetTargetPosition,
  kMagnetTargetDirection
};

enum {
  kMagnetTargetPositionFlag = 1 << kMagnetTargetPosition,
  kMagnetTargetDirectionFlag = 1 << kMagnetTargetDirection
};


// Magnets define a position and velocity in n-dimensional space. For the 1D
// case, the position is simply the value of the magnet, and the velocity is
// its rate of change.
template<class T>
struct MagnetState {
  T position;
  T velocity;
  MagnetState() : position(static_cast<T>(0)), velocity(static_cast<T>(0)) {}
  MagnetState(const T& position, const T& velocity)
    : position(position), velocity(velocity) {
  }
};


// Increase number of bits when we get more than 8 fields.
typedef uint8_t MagnetTargetFieldFlags;

// Magnets are always moving towards their target. The target can change at
// every update. The purpose of the magnet is to smoothly achieve its target,
// no matter how eratically the target is changed.
//
// Not every field in MagnetTarget needs to be specified, and not every field
// in MagnetTarget is used by every Magnet. It's the caller's responsibility
// to ensure that all required fields are populated.
//
// Why have one MagnetTarget class for every kind of magnet? Why not have a
// separate target class for each magnet? Because there is a lot of overlap.
// Most magnets require the same target values. And when all target values
// are in a shared 'MagnetTarget' class, it's easy to swap out one kind of
// magnet for another.
template<class T>
class MagnetTarget {
 public:
  MagnetTarget() { Reset(); }

  void Reset() {
    // Set default values on fields that have default values.
    direction_ = kMagnetDirectionClosest;
    valid_fields_ = kMagnetTargetDirectionFlag;
  }

  MagnetTargetFieldFlags valid_fields() const { return valid_fields_; }

  // Check validity of position field before returning.
  const T& position() const {
    assert(valid_fields_ & kMagnetTargetPositionFlag);
    return position_;
  }
  void set_position(const T& position) {
    position_ = position;
    valid_fields_ |= kMagnetTargetPositionFlag;
  }

  // direction field has a default value, so it is always valid.
  MagnetDirection direction() const { return direction_; }
  void set_direction(MagnetDirection direction) { direction_ = direction; }

 private:
  // Bitmap specifying which member have valid data.
  MagnetTargetFieldFlags valid_fields_;

  // Position in n-dimensional space. For the 1D case (e.g. T is a float), this
  // is simply the current value of the simulation. For the 3D case (e.g. T is
  // mathfu::vec3) the position is a location in 3D space.
  T position_;

  // Requested turn direction. Has meaning when there is more than one way to
  // go from 'current_' to 'target_'. For example, in a 1D modular space
  // of angles, where current is 160 degrees and target is 170 degrees,
  // a 'direction_' of 'kDirectionPositive' means increase 10 degrees
  // from 160 --> 170. However, 'kDirectionNegative' means decrease 350 degrees
  // to wrap all the way around: 160 --> -180 = 180 --> 170.
  // By default, we use 'kDirectionClosest'.
  MagnetDirection direction_;
};


// Core interface that magnets of all dimensions and precisions implement.
template<class T, class Constraints>
class MagnetBase {
 public:
  // Move the 'state_' closer to the 'target_' by advancing the simulation by
  // 'delta_time'. Derived classes must implement this function.
  virtual void AdvanceFrame(WorldTime delta_time) = 0;

  // Update 'state_.velocity' so that the simulation is no longer settled on
  // its target. Useful for faking a response to user input when 'target_'
  // should not be changed. Derived classes can optionally implement this
  // funciton; if they do not, nothing happens.
  virtual void Twitch(MagnetTwitch twitch) { (void)twitch; };

  // Returns true if 'current_' has reached 'target_'. The definition of settled
  // depends on the derived class and (most likely) user data.
  virtual bool Settled() const = 0;

  // Convenience functions to access common values.
  const T& Position() const { return state_.position; }
  const T& Velocity() const { return state_.velocity; }
  void SetPosition(const T& position) { state_.position = position; }
  void SetVelocity(const T& velocity) { state_.velocity = velocity; }
  void SetTargetPosition(const T& position) { target_.set_position(position); }
  void SetTargetDirection(MagnetDirection direction) {
    target_.set_direction(direction);
  }

  // Accessors for members.
  const MagnetState<T>& state() const { return state_; }
  const MagnetTarget<T>& target() const { return target_; }
  const Constraints& constraints() const { return *constraints_; }
  void set_target(MagnetTarget<T>& target) { target_ = target; }

 protected:
  // The current value of our simluation.
  MagnetState<T> state_;

  // The value we are trying to achieve with our simulation.
  MagnetTarget<T> target_;

  // Bounds on our simulation.
  const Constraints* constraints_;
};

} // namespace fpl

#endif // MAGNET_BASE_H_
