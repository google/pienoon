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

namespace impel {

struct SmoothImpelInit : public ImpelInitWithVelocity {
  IMPEL_INIT_REGISTER();

  SmoothImpelInit() : ImpelInitWithVelocity(kType), accel(0.0f), decel(0.0f) {}

  float accel;
  float decel;
};

class SmoothImpelProcessor :
    public ImpelProcessorWithVelocity<SmoothImpelInit> {

 public:
  IMPEL_PROCESSOR_REGISTER(SmoothImpelProcessor, SmoothImpelInit);
  virtual ~SmoothImpelProcessor() {}

 protected:
  virtual float CalculateVelocity(ImpelTime delta_time,
                                  const ImpelData& d) const;
};

} // namespace impel

#endif // IMPEL_PROCESSOR_SMOOTH_H_

