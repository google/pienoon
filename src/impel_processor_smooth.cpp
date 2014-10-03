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
#include "impel_engine.h"
#include "impel_processor_smooth.h"

namespace impel {

IMPEL_INIT_INSTANTIATE(SmoothImpelInit);

void SmoothImpelProcessor::AdvanceFrame(ImpelTime delta_time) {
  // Loop through every impeller one at a time.
  // TODO OPT: reorder data and then optimize with SIMD to process in groups
  // of 4 floating-point or 8 fixed-point values.
  for (SmoothImpelData* d = map_.Begin(); d < map_.End(); ++d) {
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

void SmoothImpelProcessor::CalculateCurve(SmoothImpelData* d) const {
  if (d->end_time > 0.0f) {
    d->curve.Initialize(d->value, d->velocity, d->target_value, 0.0f, 0.0f,
                        d->end_time);
  } else {
    d->curve = BezierCurve1f();
  }
  d->time = 0.0f;
  d->curve_valid = true;
}

} // namespace impel


