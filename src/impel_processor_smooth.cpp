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
#include "bulk_spline_evaluator.h"
#include "impel_engine.h"
#include "impel_init.h"


namespace impel {

using fpl::CompactSpline;
using fpl::BulkSplineEvaluator;


struct SmoothImpelData  {
  void Initialize(const SmoothImpelInit& init_param) {
    local_spline.Clear();
    init = init_param;
  }

  CompactSpline local_spline;

  // Keep a local copy of the init params.
  SmoothImpelInit init;
};


class SmoothImpelProcessor : public ImpelProcessor<float> {
 public:
  virtual ~SmoothImpelProcessor() {}

  virtual ImpellerType Type() const { return SmoothImpelInit::kType; }

  // Accessors to allow the user to get and set simluation values.
  virtual float Value(ImpelIndex index) const { return interpolator_.Y(index); }
  virtual float Velocity(ImpelIndex index) const {
    return interpolator_.Derivative(index);
  }
  virtual float TargetValue(ImpelIndex index) const {
    return interpolator_.EndY(index);
  }
  virtual float Difference(ImpelIndex index) const {
    const SmoothImpelData& d = Data(index);
    return d.init.Normalize(interpolator_.EndY(index) -
                            interpolator_.Y(index));
  }

  virtual void AdvanceFrame(ImpelTime delta_time) {
    Defragment();
    interpolator_.AdvanceFrame(delta_time);
  }

  virtual void SetState(ImpelIndex index, const ImpellerState& s) {
    SmoothImpelData& d = Data(index);

    if (s.valid & kTargetWaypointsValid) {
      // Initialize spline to follow way points.
      // Snaps the current value and velocity to the way point's start value
      // and velocity.
      d.local_spline.Clear();
      interpolator_.SetSpline(index, *s.waypoints, s.waypoints_start_time);

    } else {
      // Initialize spline to match specified parameters. We maintain current
      // values for any parameters that aren't specified.
      const float start_y = (s.valid & kValueValid) ? s.value :
                            interpolator_.X(index);
      const float start_derivative = (s.valid & kVelocityValid) ? s.velocity :
                                     interpolator_.Derivative(index);
      const float end_y = (s.valid & kTargetValueValid) ? s.target_value :
                          interpolator_.EndY(index);
      const float end_derivative = (s.valid & kTargetVelocityValid) ?
                                   s.target_velocity : 0.0f;
      const float end_x = (s.valid & kTargetTimeValid) ? s.target_time :
                          interpolator_.EndX(index);

      d.local_spline.Init(fpl::CreateValidRange(start_y, end_y), 1.0f, 2);
      d.local_spline.AddNode(0.0f, start_y, start_derivative);
      d.local_spline.AddNode(end_x, end_y, end_derivative);
      interpolator_.SetSpline(index, d.local_spline);
    }
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
    interpolator_.MoveIndex(old_index, new_index);
  }

  virtual void SetNumIndices(ImpelIndex num_indices) {
    data_.resize(num_indices);
    interpolator_.SetNumIndices(num_indices);
  }

  const SmoothImpelData& Data(ImpelIndex index) const {
    assert(ValidIndex(index));
    return data_[index];
  }

  SmoothImpelData& Data(ImpelIndex index) {
    assert(ValidIndex(index));
    return data_[index];
  }

  std::vector<SmoothImpelData> data_;
  BulkSplineEvaluator interpolator_;
};

IMPEL_INSTANCE(SmoothImpelInit, SmoothImpelProcessor);


} // namespace impel


