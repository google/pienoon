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
#include "bezier.h"
#include "impel_engine.h"
#include "impel_init.h"


namespace impel {

typedef fpl::BezierCurve<float, float> BezierCurve1f;


struct SmoothImpelData  {
  void Initialize(const SmoothImpelInit& init_param) {
    value = 0.0f;
    velocity = 0.0f;
    target_value = 0.0f;
    time = 0.0f;
    curve_valid = false;
    init = init_param;
  }

  // What we are animating. Returned when Impeller::Value() called.
  float value;

  // The rate of change of value. Returned when Impeller::Velocity() called.
  float velocity;

  // What we are striving to hit. Returned when Impeller::TargetValue() called.
  float target_value;

  // Current time into 'curve'.
  float time;

  // Time at which 'curve' has reached the target.
  float end_time;

  // When the current or target state is overridden, the polynomial should be
  // re-calculated. We only want to calculate once per frame, though, so we do
  // it lazily.
  bool curve_valid;

  // Polynomial with the curve of our motion over time.
  BezierCurve1f curve;

  // Keep a local copy of the init params.
  SmoothImpelInit init;
};


class SmoothImpelProcessor : public ImpelProcessor<float> {
 public:
  virtual ~SmoothImpelProcessor() {}

  virtual ImpellerType Type() const { return SmoothImpelInit::kType; }

  // Accessors to allow the user to get and set simluation values.
  virtual float Value(ImpelIndex index) const { return Data(index).value; }
  virtual float Velocity(ImpelIndex index) const {
    return Data(index).velocity;
  }
  virtual float TargetValue(ImpelIndex index) const {
    return Data(index).target_value;
  }
  virtual float Difference(ImpelIndex index) const {
    const SmoothImpelData& d = Data(index);
    return d.init.Normalize(d.target_value - d.value);
  }

  virtual void AdvanceFrame(ImpelTime delta_time) {
    Defragment();

    // Loop through every impeller one at a time.
    // TODO OPT: reorder data and then optimize with SIMD to process in groups
    // of 4 floating-point or 8 fixed-point values.
    for (auto it = data_.begin(); it < data_.end(); ++it) {
      SmoothImpelData* d = &(*it);

      // If the current or target parameters have changed, we need to recalculate
      // the curve that we're following. We do this lazily to avoid recalculating
      // more than once when both the current value and target value are set.
      if (!d->curve_valid) {
        CalculateCurve(d);
      }

      // Update the current simulation time. A time of 0 is the start of the
      // curve, and a time of end_time is the end of the curve.
      d->time += delta_time;

      // We've we're at the end of the curve then we're already at the target.
      const bool at_target = d->time >= d->end_time;
      if (at_target) {
        // Optimize the case when we're already at the target.
        d->value = d->target_value;
        d->velocity = 0.0f;
      } else {
        // Evaluate the polynomial at the current time.
        d->value = d->init.Normalize(d->curve.Evaluate(d->time));
        d->velocity = d->curve.Derivative(d->time);
      }
    }
  }

  virtual void SetValue(ImpelIndex index, const float& value) {
    SmoothImpelData& d = Data(index);
    d.value = value;
    d.curve_valid = false;
  }
  virtual void SetVelocity(ImpelIndex index, const float& velocity) {
    SmoothImpelData& d = Data(index);
    d.velocity = velocity;
    d.curve_valid = false;
  }
  virtual void SetTargetValue(ImpelIndex index, const float& target_value) {
    SmoothImpelData& d = Data(index);
    d.target_value = target_value;
    d.curve_valid = false;
  }
  virtual void SetTargetTime(ImpelIndex index, float target_time) {
    SmoothImpelData& d = Data(index);
    d.end_time = target_time;
    d.curve_valid = false;
  }

 protected:
  virtual void InitializeIndex(const ImpelInit& init, ImpelIndex index,
                               ImpelEngine* engine) {
    (void)engine;
    Data(index).Initialize(static_cast<const SmoothImpelInit&>(init));
  }

  virtual void RemoveIndex(ImpelIndex index) {
    Data(index).Initialize(SmoothImpelInit());
  }

  virtual void MoveIndex(ImpelIndex old_index, ImpelIndex new_index) {
    data_[new_index] = data_[old_index];
  }

  virtual void SetNumIndices(ImpelIndex num_indices) {
    data_.resize(num_indices);
  }


  const SmoothImpelData& Data(ImpelIndex index) const {
    assert(ValidIndex(index));
    return data_[index];
  }

  SmoothImpelData& Data(ImpelIndex index) {
    assert(ValidIndex(index));
    return data_[index];
  }

  void CalculateCurve(SmoothImpelData* d) const {
    if (d->end_time > 0.0f) {
      d->curve.Initialize(d->value, d->velocity, d->target_value, 0.0f, 0.0f,
                          d->end_time);
    } else {
      d->curve = BezierCurve1f();
    }
    d->time = 0.0f;
    d->curve_valid = true;
  }

  std::vector<SmoothImpelData> data_;
};

IMPEL_INSTANCE(SmoothImpelInit, SmoothImpelProcessor);


} // namespace impel


