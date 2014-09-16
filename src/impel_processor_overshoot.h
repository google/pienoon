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

#ifndef IMPEL_PROCESSOR_OVERSHOOT_H_
#define IMPEL_PROCESSOR_OVERSHOOT_H_

#include "impel_processor_base_classes.h"

namespace impel {


struct OvershootImpelInit : public ImpelInitWithVelocity {
  IMPEL_INIT_REGISTER();

  OvershootImpelInit() :
      ImpelInitWithVelocity(kType),
      accel_per_difference(0.0f),
      wrong_direction_multiplier(0.0f)
  {}

  // Acceleration is a multiple of abs('state_.position' - 'target_.position').
  // Bigger differences cause faster acceleration.
  float accel_per_difference;

  // When accelerating away from the target, we multiply our acceleration by
  // this amount. We need counter-acceleration to be stronger so that the
  // amplitude eventually dies down; otherwise, we'd just have a pendulum.
  float wrong_direction_multiplier;
};


class OvershootImpelProcessor :
    public ImpelProcessorWithVelocity<OvershootImpelInit> {

 public:
  IMPEL_PROCESSOR_REGISTER(OvershootImpelProcessor, OvershootImpelInit);
  virtual ~OvershootImpelProcessor() {}

 protected:
  virtual float CalculateVelocity(ImpelTime delta_time,
                                  const ImpelData& d) const;
};

} // namespace impel

#endif // IMPEL_PROCESSOR_OVERSHOOT_H_

