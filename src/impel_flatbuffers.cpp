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

#include "impel_flatbuffers.h"
#include "impel_generated.h"
#include "impel_processor_overshoot.h"
#include "impel_processor_smooth.h"

namespace impel {

void VelocityInitFromFlatBuffers(const VelocityParameters& params,
                                 ImpelInitWithVelocity* init) {
  init->modular = params.modular() != 0;
  init->min = params.min();
  init->max = params.max();
  init->max_delta = params.max_delta();
  init->max_velocity = params.max_velocity();
  Settled1fFromFlatBuffers(*params.at_target(), &init->at_target);
}

void OvershootInitFromFlatBuffers(const OvershootParameters& params,
                                  OvershootImpelInit* init) {
  VelocityInitFromFlatBuffers(*params.base(), init);
  init->accel_per_difference = params.acceleration_per_difference();
  init->wrong_direction_multiplier =
      params.wrong_direction_acceleration_multiplier();
  init->max_delta_time = params.max_delta_time();
}

void SmoothInitFromFlatBuffers(const SmoothParameters& params,
                               SmoothImpelInit* init) {
  VelocityInitFromFlatBuffers(*params.base(), init);
}

void Settled1fFromFlatBuffers(const Settled1fParameters& params,
                              Settled1f* settled) {
  settled->max_velocity = params.max_velocity();
  settled->max_difference = params.max_difference();
}

} // namespace impel

