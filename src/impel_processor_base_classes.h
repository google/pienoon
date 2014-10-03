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

#ifndef IMPEL_PROCESSOR_BASE_CLASSES_H_
#define IMPEL_PROCESSOR_BASE_CLASSES_H_

#include "impel_id_map.h"
#include "impel_util.h"
#include "impeller.h"

namespace impel {

// ImpelProcessors that derive from ImpelProcessorWithVelocity should have an
// ImpelInit that derives from this struct (ImpelInitWithVelocity).
struct ImpelInitWithVelocity : public ImpelInit {
  // A modular value wraps around from min to max. For example, an angle
  // is modular, where -pi is equivalent to +pi. Setting this to true ensures
  // that arithmetic wraps around instead of clamping to min/max.
  bool modular;

  // Minimum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float min;

  // Maximum value for Impeller::Value(). Clamped or wrapped-around when we
  // reach this boundary.
  float max;

  // Maximum speed at which the value can change. That is, maximum value for
  // the Impeller::Velocity(). In units/tick.
  // For example, if the value is an angle, then this is the max angular
  // velocity, and the units are radians/tick.
  float max_velocity;

  // Maximum that Impeller::Value() can be altered on a single call to
  // ImpelEngine::AdvanceFrame(), regardless of velocity or delta_time.
  float max_delta;

  // Cutoff to determine if the Impeller's current state has settled on the
  // target. Once it has settled, Value() is set to TargetValue() and Velocity()
  // is set to zero.
  Settled1f at_target;

  // The derived type must call this constructor with it's ImpelType identifier.
  explicit ImpelInitWithVelocity(ImpellerType type) :
      ImpelInit(type),
      modular(false),
      min(0.0f),
      max(0.0f),
      max_velocity(0.0f),
      max_delta(0.0f) {
  }

  // Ensure position 'x' is within the valid constraint range.
  float Normalize(float x) const {
    // For non-modular values, do nothing.
    if (!modular)
      return x;

    // For modular values, ensures 'x' is in the constraints (min, max].
    // That is, exclusive of min, inclusive of max.
    const float width = max - min;
    const float above_min = x <= min ? x + width : x;
    const float normalized = above_min > max ? above_min - width : above_min;
    assert(min < normalized && normalized <= max);
    return normalized;
  }

  // Ensure the Impeller's 'value' doesn't increment by more than 'max_delta'.
  // This is different from ClampVelocity because it is independent of time.
  // No matter how big the timestep, the delta will not be too great.
  float ClampDelta(float delta) const {
    return mathfu::Clamp(delta, -max_delta, max_delta);
  }

  // Ensure velocity is within the reasonable limits.
  float ClampVelocity(float velocity) const {
    return mathfu::Clamp(velocity, -max_velocity, max_velocity);
  }

  // Ensure the impeller value is within the specified range.
  float ClampValue(float value) const {
    return mathfu::Clamp(value, min, max);
  }

  // Return true if we're close to the target and almost stopped.
  // The definition of "close to" and "almost stopped" are given by the
  // "at_target" member.
  bool AtTarget(float dist, float velocity) const {
    return at_target.Settled(dist, velocity);
  }
};


// For impeller types that derive from ImpelInitWithVelocity, each Impeller
// gets a copy of this data. The data is held centrally, in the ImpelProcessor.
struct ImpelDataWithVelocity {
  // What we are animating. Returned when Impeller::Value() called.
  float value;

  // The rate of change of value. Returned when Impeller::Velocity() called.
  float velocity;

  // What we are striving to hit. Returned when Impeller::TargetValue() called.
  float target_value;

  virtual void Initialize(const ImpelInit& /*init_param*/) {
    value = 0.0f;
    velocity = 0.0f;
    target_value = 0.0f;
  }
};


// InitType must derive from ImpelInitWithVelocity.
// ImpelData must derive from ImpelDataWithVelocity. It should have a member
// called 'init' of type InitType.
template<class ImpelData, class InitType>
class ImpelProcessorWithVelocity : public ImpelProcessor<float> {
 public:
  virtual ~ImpelProcessorWithVelocity() {
    assert(map_.Count() == 0);
  }

  virtual ImpelId InitializeImpeller(const ImpelInit& init,
                                     ImpelEngine* /*engine*/) {
    assert(init.type == InitType::kType);

    // Allocate an external id, and map it to an index into data_.
    ImpelId id = map_.Allocate();

    // Initialize the newly allocated item in data_.
    Data(id).Initialize(static_cast<const InitType&>(init));
    return id;
  }

  virtual void RemoveImpeller(ImpelId id) { map_.Free(id); }
  virtual ImpellerType Type() const { return InitType::kType; }

  // Accessors to allow the user to get and set simluation values.
  virtual float Value(ImpelId id) const { return Data(id).value; }
  virtual float Velocity(ImpelId id) const { return Data(id).velocity; }
  virtual float TargetValue(ImpelId id) const { return Data(id).target_value; }
  virtual void SetValue(ImpelId id, const float& value) {
    Data(id).value = value;
  }
  virtual void SetVelocity(ImpelId id, const float& velocity) {
    Data(id).velocity = velocity;
  }
  virtual void SetTargetValue(ImpelId id, const float& target_value) {
    Data(id).target_value = target_value;
  }
  virtual void SetTargetTime(ImpelId /*id*/, float /*target_time*/) {}
  virtual float Difference(ImpelId id) const {
    const ImpelData& d = Data(id);
    return d.init.Normalize(d.target_value - d.value);
  }

 protected:
  ImpelData& Data(ImpelId id) { return map_.Data(id); }
  const ImpelData& Data(ImpelId id) const { return map_.Data(id); }

  IdMap<ImpelData> map_;
};


} // namespace impel

#endif // IMPEL_PROCESSOR_BASE_CLASSES_H_

