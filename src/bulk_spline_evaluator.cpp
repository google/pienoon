// Copyright 2015 Google Inc. All rights reserved.
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

#include <string>
#include <sstream>
#include <vector>
#include "bulk_spline_evaluator.h"
#include "dual_cubic.h"

using mathfu::Lerp;

namespace fpl {

void BulkSplineEvaluator::SetNumIndices(const Index num_indices) {
  cubics_.resize(num_indices);
  domains_.resize(num_indices);
  splines_.resize(num_indices, nullptr);
  results_.resize(num_indices);
}

void BulkSplineEvaluator::MoveIndex(const Index old_index,
                                    const Index new_index) {
  cubics_[new_index] = cubics_[old_index];
  domains_[new_index] = domains_[old_index];
  splines_[new_index] = splines_[old_index];
  results_[new_index] = results_[old_index];
}

void BulkSplineEvaluator::SetSpline(const Index index,
                                    const CompactSpline& spline,
                                    const float start_x) {
  splines_[index] = &spline;
  domains_[index].x = start_x;
  domains_[index].x_index = kInvalidSplineIndex;
  InitCubic(index);
  EvaluateIndex(index);
}

void BulkSplineEvaluator::EvaluateIndex(const Index index) {
  const float cubic_x = CubicX(index);
  Result& r = results_[index];
  r.y = cubics_[index].Evaluate(cubic_x);
  r.derivative = cubics_[index].Derivative(cubic_x);
}

void BulkSplineEvaluator::AdvanceFrame(const float delta_x) {
  const Index num_indices = NumIndices();

  // Update x. We traverse the 'domains_' array linearly, which helps with
  // cache performance. TODO OPT: Add cache prefetching.
  for (Index index = 0; index < num_indices; ++index) {
    Domain& d = domains_[index];
    d.x += delta_x;

    // If x is out of the current range, reinitialize the cubic.
    if (!d.range.Contains(d.x)) {
      InitCubic(index);
    }
    d.x = d.range.Clamp(d.x);
  }

  // Evaluate the cubics. We traverse the 'cubics_' array linearly, which helps
  // with cache performance. TODO OPT: Add cache prefetching.
  for (Index index = 0; index < num_indices; ++index) {
    EvaluateIndex(index);
  }
}

bool BulkSplineEvaluator::Valid(const Index index) const {
  return 0 <= index && index < NumIndices() && splines_[index] != nullptr;
}

void BulkSplineEvaluator::InitCubic(const Index index) {
  // Do nothing if the requested index has no spline.
  const CompactSpline* spline = splines_[index];
  if (spline == nullptr) return;

  // Do nothing if the current cubic matches the current spline segment.
  Domain& d = domains_[index];
  const CompactSplineIndex x_index = spline->IndexForX(d.x, d.x_index + 1);
  if (d.x_index == x_index) return;

  // Update the x-related values.
  d.x_index = x_index;
  d.range = spline->RangeX(x_index);

  // Initialize the cubic to interpolate the new spline segment.
  const CubicInit init = spline->CreateCubicInit(x_index);
  cubics_[index].Init(init);
}

}  // namespace fpl
