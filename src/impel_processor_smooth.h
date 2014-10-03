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

#ifndef IMPEL_PROCESSOR_SMOOTH_H_
#define IMPEL_PROCESSOR_SMOOTH_H_

#include "impel_processor_base_classes.h"
#include "bezier.h"


namespace impel {


typedef fpl::BezierCurve<float, float> BezierCurve1f;


struct SmoothImpelInit : public ImpelInitWithVelocity {
  IMPEL_INIT_REGISTER();

  SmoothImpelInit() : ImpelInitWithVelocity(kType), time_per_dist(0.0f) {}

  float time_per_dist;
};


struct SmoothImpelData : public ImpelDataWithVelocity {
  // When the current or target state is overridden, the polynomial should be
  // re-calculated. We only want to calculate once per frame, though, so we do
  // it lazily.
  bool curve_valid;

  // Current time into 'curve'.
  float time;

  // Time at which 'curve' has reached the target.
  float end_time;

  // Polynomial with the curve of our motion over time.
  BezierCurve1f curve;

  // Keep a local copy of the init params.
  SmoothImpelInit init;

  virtual void Initialize(const ImpelInit& init_param) {
    ImpelDataWithVelocity::Initialize(init_param);
    curve_valid = false;
    time = 0.0f;
    curve = BezierCurve1f();
    init = static_cast<const SmoothImpelInit&>(init_param);
  }
};


class SmoothImpelProcessor :
    public ImpelProcessorWithVelocity<SmoothImpelData, SmoothImpelInit> {

 public:
  IMPEL_PROCESSOR_REGISTER(SmoothImpelProcessor, SmoothImpelInit);
  virtual ~SmoothImpelProcessor() {}

  virtual void AdvanceFrame(ImpelTime delta_time);
  virtual void SetVelocity(ImpelId id, const float& velocity) {
    ImpelProcessorWithVelocity::SetVelocity(id, velocity);
    Data(id).curve_valid = false;
  }
  virtual void SetTargetValue(ImpelId id, const float& target_value) {
    ImpelProcessorWithVelocity::SetTargetValue(id, target_value);
    Data(id).curve_valid = false;
  }
  virtual void SetTargetTime(ImpelId id, float target_time) {
    SmoothImpelData& data = Data(id);
    data.curve_valid = false;
    data.end_time = target_time;
  }

 protected:
  void CalculateCurve(SmoothImpelData* d) const;
};

} // namespace impel

#endif // IMPEL_PROCESSOR_SMOOTH_H_

