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
#include "impel_init.h"

namespace impel {

static void ModularInitFromFlatBuffers(const ModularParameters& params,
                                       ModularImpelInit* init) {
  init->set_modular(params.modular() != 0);
  init->set_range(fpl::Range(params.min(), params.max()));
}

void OvershootInitFromFlatBuffers(const OvershootParameters& params,
                                  OvershootImpelInit* init) {
  ModularInitFromFlatBuffers(*params.base(), init);
  init->set_max_velocity(params.max_velocity());
  init->set_max_delta(params.max_delta());
  Settled1fFromFlatBuffers(*params.at_target(), &init->at_target());
  init->set_accel_per_difference(params.acceleration_per_difference());
  init->set_wrong_direction_multiplier(
      params.wrong_direction_acceleration_multiplier());
  init->set_max_delta_time(params.max_delta_time());
}

void SmoothInitFromFlatBuffers(const SmoothParameters& params,
                               SmoothImpelInit* init) {
  ModularInitFromFlatBuffers(*params.base(), init);
}

void Settled1fFromFlatBuffers(const Settled1fParameters& params,
                              Settled1f* settled) {
  settled->max_velocity = params.max_velocity();
  settled->max_difference = params.max_difference();
}

}  // namespace impel
