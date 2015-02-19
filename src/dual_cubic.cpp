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

#include "dual_cubic.h"

using mathfu::Lerp;

namespace fpl {

static const float kMaxSteepness = 4.0f;
static const float kMinMidPercent = 0.1f;
static const float kMaxMidPercent = 1.0f - kMinMidPercent;

// One node of a spline that specifies both first and second derivatives.
// Only used internally.
struct SplineControlNode {
  SplineControlNode(const float x, const float y, const float derivative,
                    const float second_derivative = 0.0f)
      : x(x),
        y(y),
        derivative(derivative),
        second_derivative(second_derivative) {}

  float x;
  float y;
  float derivative;
  float second_derivative;
};

static const Range kZeroToOne(0.0f, 1.0f);

static QuadraticCurve CalculateValidMidRangeSplineForStart(
    const SplineControlNode& start, const SplineControlNode& end) {
  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w0 = start.second_derivative;
  const float w1 = end.second_derivative;
  const float s0 = start.derivative;
  const float s1 = end.derivative;

  // r_g(k) = wd * k^2  +  (4*sd - w0 - 2w1)k  +  6yd - 2s0 - 4s1 + w1
  const float c2 = wd;
  const float c1 = 4 * sd - w0 - 2 * w1;
  const float c0 = 6 * yd - 2 * s0 - 4 * s1 + w1;
  return QuadraticCurve(c2, c1, c0);
}

static QuadraticCurve CalculateValidMidRangeSplineForEnd(
    const SplineControlNode& start, const SplineControlNode& end) {
  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w1 = end.second_derivative;
  const float s1 = end.derivative;

  // r_g(k) = -wd * k^2  +  (-4*sd + 3w1)k  -  6yd + 6s1 - 2w1
  const float c2 = -wd;
  const float c1 = -4 * sd + 3 * w1;
  const float c0 = -6 * yd + 6 * s1 - 2 * w1;
  return QuadraticCurve(c2, c1, c0);
}

static Range CalculateValidMidRange(const SplineControlNode& start,
                                    const SplineControlNode& end,
                                    bool* is_valid = nullptr) {
  // The sign of these quadratics determine where the mid-node is valid.
  // One quadratic for the start cubic, and one for the end cubic.
  const QuadraticCurve start_spline =
      CalculateValidMidRangeSplineForStart(start, end);
  const QuadraticCurve end_spline =
      CalculateValidMidRangeSplineForEnd(start, end);

  // The mid node is valid when the quadratic sign matches the second
  // derivative's sign.
  Range::RangeArray<2> start_ranges;
  Range::RangeArray<2> end_ranges;
  start_spline.RangesMatchingSign(kZeroToOne, start.second_derivative,
                                  &start_ranges);
  end_spline.RangesMatchingSign(kZeroToOne, end.second_derivative, &end_ranges);

  // Find the valid overlapping ranges, or the gaps inbetween the ranges.
  Range::RangeArray<4> intersections;
  Range::RangeArray<4> gaps;
  Range::IntersectRanges(start_ranges, end_ranges, &intersections, &gaps);

  // The mid-node is valid only if there is an overlapping range.
  if (is_valid != nullptr) {
    *is_valid = intersections.len > 0;
  }

  // Take the largest overlapping range. If none, find the smallest gap
  // between the ranges.
  return intersections.len > 0
             ? intersections.arr[Range::IndexOfLongest(intersections)]
             : gaps.len > 0 ? gaps.arr[Range::IndexOfShortest(gaps)]
                            : kZeroToOne;
}

static float CalculateMidPercent(const SplineControlNode& start,
                                 const SplineControlNode& end) {
  // The mid value we called 'k' in the dual cubic documentation.
  // It's between 0~1 and determines where the start and end cubics are joined
  // along the x-axis.
  const Range valid_range = CalculateValidMidRange(start, end);

  // Return the part of the range closest to the half-way mark. This seems to
  // generate the smoothest looking curves.
  const float mid_unclamped = valid_range.Clamp(0.5f);

  // Clamp away from 0 and 1. The math requires the mid node to be strictly
  // between 0 and 1. If we get to close to 0 or 1, some divisions are going to
  // explode and we'll lose numerical precision.
  const float mid =
      mathfu::Clamp(mid_unclamped, kMinMidPercent, kMaxMidPercent);
  return mid;
}

static SplineControlNode CalculateMidNode(const SplineControlNode& start,
                                          const SplineControlNode& end,
                                          const float k) {
  // The mid node is at x = Lerp(start.x, end.x, k)
  // It has y value of 'y' and slope of 's', defined as:
  //
  // s = 3(y1-y0) - 2Lerp(s1,s0,k) - 1/2(k^2*w0 - (1-k)^2*w1)
  // y = Lerp(y0,y1,k) + k(1-k)(-2/3(s1-s0) + 1/6 Lerp(w1,w0,k))
  //
  // where (x0, y0, s0, w0) is the start control node's x, y, derivative, and
  // second derivative, and (x1, y1, s1, w1) similarly represents the end
  // control node.
  //
  // See the document on "Dual Cubics" for a derivation of this solution.
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float derivative_k = Lerp(end.derivative, start.derivative, k);
  const float y_k = Lerp(start.y, end.y, k);
  const float second_k =
      Lerp(end.second_derivative, start.second_derivative, k);
  const float j = 1.0f - k;
  const float second_k_squared =
      k * k * start.second_derivative - j * j * end.second_derivative;

  const float s = 3.0f * y_diff - 2.0f * derivative_k - 0.5f * second_k_squared;
  const float y =
      y_k + k * j * (-2.0f / 3.0f * s_diff + 1.0f / 6.0f * second_k);
  const float x = Lerp(start.x, end.x, k);

  return SplineControlNode(x, y, s, 0.0f);
}

// See the Dual Cubics document for a derivation of this equation.
static float ExtremeSecondDerivativeForStart(const SplineControlNode& start,
                                             const SplineControlNode& end,
                                             const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float extreme_second =
      s_diff +
      (1.0f / k) * (3.0f * y_diff - 2.0f * start.derivative - end.derivative);
  return extreme_second;
}

// See the Dual Cubics document for a derivation of this equation.
static float ExtremeSecondDerivativeForEnd(const SplineControlNode& start,
                                           const SplineControlNode& end,
                                           const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float extreme_second =
      (1.0f / (k - 1.0f)) *
      (s_diff * k + 3.0f * y_diff - 3.0f * end.derivative);
  return extreme_second;
}

// Android does not support log2 in its math.h.
static inline float Log2(const float x) {
#ifdef __ANDROID__
  static const float kOneOverLog2 = 3.32192809489f;
  return log(x) * kOneOverLog2;  // log2(x) = log(x) / log(2)
#else
  return log2(x);
#endif
}

// Steepness is a notion of how much the derivative has to change from the
// start (x=0) to the end (x=1). For derivatives under 1, we don't really care,
// since cubics can change fast enough to cover those differences. Only extreme
// differences in derivatives cause trouble.
static float CalculateSteepness(const float derivative) {
  const float abs_derivative = fabs(derivative);
  return abs_derivative <= 1.0f ? 0.0f : Log2(abs_derivative);
}

static float ApproximateMidPercent(const SplineControlNode& start,
                                   const SplineControlNode& end,
                                   float* start_percent, float* end_percent) {
  // The greater the difference in steepness, the more skewed the mid percent.
  const float start_steepness = CalculateSteepness(start.derivative);
  const float end_steepness = CalculateSteepness(end.derivative);
  const float diff_steepness = fabs(start_steepness - end_steepness);
  const float percent_extreme = std::min(diff_steepness / kMaxSteepness, 1.0f);

  // We skew the mid percent towards the steeper side.
  // If equally steep, the mid percent is right in the middle: 0.5.
  const bool start_is_steeper = start_steepness >= end_steepness;
  const float extreme_percent =
      start_is_steeper ? kMinMidPercent : kMaxMidPercent;
  const float mid_percent = Lerp(0.5f, extreme_percent, percent_extreme);

  // Later, when we calculate the second derivatives, we want to skew to the
  // extreme second derivatives in the same manner (steeper side gets skewed to
  // more).
  *start_percent = start_is_steeper ? percent_extreme : 1.0f - percent_extreme;
  *end_percent = start_is_steeper ? 1.0f - percent_extreme : percent_extreme;
  return mid_percent;
}

void CalculateDualCubicMidNode(const CubicInit& init, float* x, float* y,
                               float* derivative) {
  // The initial y and derivative values of our node are given by the
  // 'init' control nodes. We scale to x from 0~1, because all of our math
  // assumes x on this domain.
  SplineControlNode start(0.0f, init.start_y,
                          init.start_derivative * init.width_x);
  SplineControlNode end(1.0f, init.end_y, init.end_derivative * init.width_x);

  // Use a heuristic to guess a reasonably close place to split the cubic into
  // two cubics.
  float start_percent, end_percent;
  const float approx_mid_percent =
      ApproximateMidPercent(start, end, &start_percent, &end_percent);

  // Given the start and end conditions and the place to split the cubic,
  // find the extreme second derivatives for start and end curves. See the
  // Dual Cubic document for a derivation of the math here.
  const float start_extreme_second =
      ExtremeSecondDerivativeForStart(start, end, approx_mid_percent);
  const float end_extreme_second =
      ExtremeSecondDerivativeForEnd(start, end, approx_mid_percent);

  // Don't just use the extreme values since this will create a curve that's
  // flat in the middle. Skew the second derivative to favor the steeper side.
  start.second_derivative = Lerp(0.0f, start_extreme_second, start_percent);
  end.second_derivative = Lerp(0.0f, end_extreme_second, end_percent);

  // Now that we have the full characterization of the start end end nodes
  // (including second derivatives), calculate the actual ideal mid percent
  // (i.e. the place to split the curve).
  const float mid_percent = CalculateMidPercent(start, end);

  // With a full characterization of start and end nodes, and a place to
  // split the curve, we can uniquely calculate the mid node.
  const SplineControlNode mid = CalculateMidNode(start, end, mid_percent);

  // Re-scale the output values to the proper x-width.
  *x = mid.x * init.width_x;
  *y = mid.y;
  *derivative = mid.derivative / init.width_x;
}

}  // namespace fpl
