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
#include "impel_init.h"

namespace impel {


struct OvershootImpelData {
  void Initialize(const OvershootImpelInit& init_param) {
    value = 0.0f;
    velocity = 0.0f;
    target_value = 0.0f;
    init = init_param;
  }

  // What we are animating. Returned when Impeller::Value() called.
  float value;

  // The rate of change of value. Returned when Impeller::Velocity() called.
  float velocity;

  // What we are striving to hit. Returned when Impeller::TargetValue() called.
  float target_value;

  // Keep a local copy of the init params.
  OvershootImpelInit init;
};

class OvershootImpelProcessor : public ImpelProcessor<float> {
 public:
  virtual ~OvershootImpelProcessor() {}

  virtual void AdvanceFrame(ImpelTime delta_time) {
    Defragment();

    // Loop through every impeller one at a time.
    // TODO: change this to a closed-form equation.
    // TODO OPT: reorder data and then optimize with SIMD to process in groups
    // of 4 floating-point or 8 fixed-point values.
    for (auto d = data_.begin(); d < data_.end(); ++d) {
      for (ImpelTime time_remaining = delta_time; time_remaining > 0;) {
        ImpelTime dt = std::min(time_remaining, d->init.max_delta_time());

        d->velocity = CalculateVelocity(dt, *d);
        d->value = CalculateValue(dt, *d);

        time_remaining -= dt;
      }
    }
  }

  virtual ImpellerType Type() const { return OvershootImpelInit::kType; }

  // Accessors to allow the user to get and set simluation values.
  virtual float Value(ImpelIndex index) const { return Data(index).value; }
  virtual float Velocity(ImpelIndex index) const {
    return Data(index).velocity;
  }
  virtual float TargetValue(ImpelIndex index) const {
    return Data(index).target_value;
  }
  virtual float Difference(ImpelIndex index) const {
    const OvershootImpelData& d = Data(index);
    return d.init.Normalize(d.target_value - d.value);
  }
  virtual void SetState(ImpelIndex index, const ImpellerState& state) {
    OvershootImpelData& data = Data(index);
    if (state.valid & kValueValid) {
      data.value = state.value;
    }
    if (state.valid & kVelocityValid) {
      data.velocity = state.velocity;
    }
    if (state.valid & kTargetValueValid) {
      data.target_value = state.target_value;
    }
  }

  virtual int Priority() const { return 1; }

 protected:
  virtual void InitializeIndex(const ImpelInit& init, ImpelIndex index,
                               ImpelEngine* engine) {
    (void)engine;
    Data(index).Initialize(static_cast<const OvershootImpelInit&>(init));
  }

  virtual void RemoveIndex(ImpelIndex index) {
    Data(index).Initialize(OvershootImpelInit());
  }

  virtual void MoveIndex(ImpelIndex old_index, ImpelIndex new_index) {
    data_[new_index] = data_[old_index];
  }

  virtual void SetNumIndices(ImpelIndex num_indices) {
    data_.resize(num_indices);
  }

  const OvershootImpelData& Data(ImpelIndex index) const {
    assert(ValidIndex(index));
    return data_[index];
  }

  OvershootImpelData& Data(ImpelIndex index) {
    assert(ValidIndex(index));
    return data_[index];
  }

  float CalculateVelocity(ImpelTime delta_time,
                          const OvershootImpelData& d) const {
    // Increment our current face angle velocity.
    // If we're moving in the wrong direction (i.e. away from the target),
    // increase the acceleration. This results in us moving towards the target
    // for longer time than we move away from the target, or equivalently,
    // aggressively initiating our movement towards the target, which feels good.
    const float diff = d.init.Normalize(d.target_value - d.value);
    const bool wrong_direction = d.velocity * diff < 0.0f;
    const float wrong_direction_multiplier = wrong_direction ?
        d.init.wrong_direction_multiplier() : 1.0f;
    const float acceleration = diff * d.init.accel_per_difference() *
                               wrong_direction_multiplier;
    const float velocity_unclamped = d.velocity + delta_time * acceleration;

    // Always ensure the velocity remains within the valid limits.
    const float velocity = d.init.ClampVelocity(velocity_unclamped);

    // If we're far from facing the target, use the velocity calculated above.
    const bool should_snap = d.init.AtTarget(diff, velocity);
    if (should_snap)
      return 0.0f;

    return velocity;
  }

  float CalculateValue(ImpelTime delta_time,
                       const OvershootImpelData& d) const {
    // Snap to the target value when we've stopped moving.
    if (d.velocity == 0.0f)
      return d.target_value;

    const float delta = d.init.ClampDelta(delta_time * d.velocity);
    const float value_unclamped = d.init.Normalize(d.value + delta);
    const float value = d.init.ClampValue(value_unclamped);
    return value;
  }

  std::vector<OvershootImpelData> data_;
};

IMPEL_INSTANCE(OvershootImpelInit, OvershootImpelProcessor);


} // namespace impel

