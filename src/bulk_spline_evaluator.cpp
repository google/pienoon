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
  domains_.resize(num_indices);
  splines_.resize(num_indices, nullptr);
  results_.resize(num_indices);
}

void BulkSplineEvaluator::MoveIndex(const Index old_index,
                                    const Index new_index) {
  domains_[new_index] = domains_[old_index];
  splines_[new_index] = splines_[old_index];
  results_[new_index] = results_[old_index];
}

void BulkSplineEvaluator::SetYRange(const Index index, const Range& valid_y,
                                    const bool modular_arithmetic) {
  Domain& d = domains_[index];
  d.valid_y = valid_y;
  d.modular_arithmetic = modular_arithmetic;
}

void BulkSplineEvaluator::SetSpline(const Index index,
                                    const CompactSpline& spline,
                                    const float start_x) {
  Domain& d = domains_[index];
  splines_[index] = &spline;
  d.x = start_x;
  d.x_index = kInvalidSplineIndex;
  InitCubic(index);
  EvaluateIndex(index);
}

void BulkSplineEvaluator::EvaluateIndex(const Index index) {
  Domain& d = domains_[index];
  Result& r = results_[index];

  // Evaluate the cubic spline.
  const float cubic_x = CubicX(index);
  r.y = d.cubic.Evaluate(cubic_x);
  r.derivative = d.cubic.Derivative(cubic_x);

  // Clamp or normalize the y value, to bring into the valid y range.
  // Also adjust the constant of the cubic so that next time we evaluate the
  // cubic it will be inside the normalized range.
  if (d.modular_arithmetic) {
    const float adjustment = d.valid_y.ModularAdjustment(r.y);
    r.y += adjustment;
    d.cubic.SetCoeff(0, d.cubic.Coeff(0) + adjustment);

  } else {
    r.y = d.valid_y.Clamp(r.y);
  }
}

void BulkSplineEvaluator::AdvanceFrame(const float delta_x) {
  const Index num_indices = NumIndices();

  // Update x. We traverse the 'domains_' array linearly, which helps with
  // cache performance. TODO OPT: Add cache prefetching.
  for (Index index = 0; index < num_indices; ++index) {
    Domain& d = domains_[index];
    d.x += delta_x;

    // If x is out of the current valid_x, reinitialize the cubic.
    if (!d.valid_x.Contains(d.x)) {
      InitCubic(index);
    }
    d.x = d.valid_x.ClampBeforeEnd(d.x);

    // Evaluate the cubics.
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
  d.valid_x = spline->RangeX(x_index);

  // Initialize the cubic to interpolate the new spline segment.
  const CubicInit init = spline->CreateCubicInit(x_index);
  d.cubic.Init(init);

  // The start y value of the cubic is d.cubic.Coeff(0) (the constant
  // coefficient) since cubic_x=0 at the start. So to ensure that the cubic
  // is normalized, it sufficents to ensure that d.cubic.Coeff(0) is normalized.
  if (d.modular_arithmetic) {
    d.cubic.SetCoeff(0, d.valid_y.NormalizeWildValue(d.cubic.Coeff(0)));
  }
}

}  // namespace fpl
