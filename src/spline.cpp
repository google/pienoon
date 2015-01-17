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

#include <string>
#include <sstream>
#include <vector>
#include "spline.h"


namespace fpl {


static const Range kZeroToOne(0.0f, 1.0f);


bool DualCubicSpline::Valid() const {
  return start_.UniformCurvature(Range(0.0f, mid_x_ - start_x_)) &&
         end_.UniformCurvature(Range(0.0f, end_x_ - mid_x_)) &&
         start_x_ <= mid_x_ && mid_x_ <= end_x_;
}

void DualCubicSpline::Initialize(const SplineControlPoint& start,
                                 const SplineControlPoint& end) {
  const float mid_percent = CalculateMidPercent(start, end);
  Initialize(start, end, mid_percent);
}

void DualCubicSpline::Initialize(const SplineControlPoint& start,
                                 const SplineControlPoint& end,
                                 const float mid_percent) {
  const SplineControlPoint mid = CalculateMidPoint(start, end, mid_percent);
  start_.Init(CubicInitWithWidth(start.y, start.derivative,
                                 mid.y, mid.derivative, mid.x - start.x));
  end_.Init(CubicInitWithWidth(mid.y, mid.derivative,
                               end.y, end.derivative, end.x - mid.x));
  start_x_ = start.x;
  mid_x_ = mid.x;
  end_x_ = end.x;
}

// static
Range SecondDerivativeRangeForStart(const SplineControlPoint& start,
                                    const SplineControlPoint& end,
                                    const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float max_second = s_diff + (1.0f / k) *
      (3.0f * y_diff - 2.0f * start.derivative - end.derivative);
  return Range(0.0f, max_second);
}

// static
Range SecondDerivativeRangeForEnd(const SplineControlPoint& start,
                                  const SplineControlPoint& end,
                                  const float mid_percent) {
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float k = mid_percent;
  const float max_second = (1.0f / (k - 1.0f)) *
      (s_diff * k + 3.0f * y_diff - 3.0f * end.derivative);
  return Range(0.0f, max_second);
}

// static
float DualCubicSpline::CalculateMidPercent(const SplineControlPoint& start,
                                           const SplineControlPoint& end) {
  // The mid value we called 'k' in the dual cubic documentation.
  // It's between 0~1 and determines where the start and end cubics are joined
  // along the x-axis.
  const Range valid_range = CalculateValidMidRange(start, end);

  // Return the part of the range closest to the half-way mark. This seems to
  // generate the smoothest looking curves.
  const float mid_unclamped = valid_range.Clamp(0.5f);

  // Clamp away from 0 and 1. The math requires the mid point to be strictly
  // between 0 and 1. If we get to close to 0 or 1, some divisions are going to
  // explode and we'll lose numerical precision.
  const float kMinPercent = 0.1f;
  const float kMaxPercent = 1.0f - kMinPercent;
  const float mid = mathfu::Clamp(mid_unclamped, kMinPercent, kMaxPercent);
  return mid;
}

// static
QuadraticCurve DualCubicSpline::CalculateValidMidRangeSplineForStart(
    const SplineControlPoint& start, const SplineControlPoint& end) {

  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w0 = start.second_derivative;
  const float w1 = end.second_derivative;
  const float s0 = start.derivative;
  const float s1 = end.derivative;

  // r_g(k) = wd * k^2  +  (4*sd - w0 - 2w1)k  +  6yd - 2s0 - 4s1 + w1
  const float c2 = wd;
  const float c1 = 4.0f * sd  -  w0  -  2.0f * w1;
  const float c0 = 6.0f * yd  -  2.0f * s0  -  4.0f * s1  +  w1;
  return QuadraticCurve(c2, c1, c0);
}

QuadraticCurve DualCubicSpline::CalculateValidMidRangeSplineForEnd(
    const SplineControlPoint& start, const SplineControlPoint& end) {

  const float yd = end.y - start.y;
  const float sd = end.derivative - start.derivative;
  const float wd = end.second_derivative - start.second_derivative;
  const float w1 = end.second_derivative;
  const float s1 = end.derivative;

  // r_g(k) = -wd * k^2  +  (-4*sd + 3w1)k  -  6yd + 6s1 - 2w1
  const float c2 = -wd;
  const float c1 = -4.0f * sd  +  3.0f * w1;
  const float c0 = -6.0f * yd  +  6.0f * s1  -  2.0f * w1;
  return QuadraticCurve(c2, c1, c0);
}

// static
Range DualCubicSpline::CalculateValidMidRange(const SplineControlPoint& start,
                                              const SplineControlPoint& end,
                                              bool* is_valid) {
  // The sign of these quadratics determine where the mid-point is valid.
  // One quadratic for the start cubic, and one for the end cubic.
  const QuadraticCurve start_spline =
      CalculateValidMidRangeSplineForStart(start, end);
  const QuadraticCurve end_spline =
      CalculateValidMidRangeSplineForEnd(start, end);

  // The mid point is valid when the quadratic sign matches the second
  // derivative's sign.
  std::vector<Range> start_ranges;
  std::vector<Range> end_ranges;
  start_spline.RangesMatchingSign(kZeroToOne, start.second_derivative,
                                  &start_ranges);
  end_spline.RangesMatchingSign(kZeroToOne, end.second_derivative,
                                &end_ranges);

  // Find the valid overlapping ranges, or the gaps inbetween the ranges.
  std::vector<Range> intersections;
  std::vector<Range> gaps;
  Range::IntersectRanges(start_ranges, end_ranges, &intersections, &gaps);

  // The mid-point is valid only if there is an overlapping range.
  if (is_valid != nullptr) {
    *is_valid = intersections.size() > 0;
  }

  // Take the largest overlapping range. If none, find the smallest gap
  // between the ranges.
  return intersections.size() > 0 ?
         intersections[Range::IndexOfLongest(intersections)] :
         gaps.size() > 0 ? gaps[Range::IndexOfShortest(gaps)] :
         kZeroToOne;
}

// static
SplineControlPoint DualCubicSpline::CalculateMidPoint(
    const SplineControlPoint& start_wide, const SplineControlPoint& end_wide,
    const float k) {
  using mathfu::Lerp;

  // The equations are set up for x running from 0 ~ 1. Convert inputs to this
  // format.
  const float x_width = end_wide.x - start_wide.x;
  const SplineControlPoint start(0.0f, start_wide.y,
                                 start_wide.derivative * x_width,
                                 start_wide.second_derivative *
                                 x_width * x_width);
  const SplineControlPoint end(1.0f, end_wide.y,
                               end_wide.derivative * x_width,
                               end_wide.second_derivative * x_width * x_width);

  // The mid point is at x = Lerp(start.x, end.x, k)
  // It has y value of 'y' and slope of 's', defined as:
  //
  // s = 3(y1-y0) - 2Lerp(s1,s0,k) - 1/2(k^2*w0 - (1-k)^2*w1)
  // y = Lerp(y0,y1,k) + k(1-k)(-2/3(s1-s0) + 1/6 Lerp(w1,w0,k))
  //
  // where (x0, y0, s0, w0) is the start control point's x, y, derivative, and
  // second derivative, and (x1, y1, s1, w1) similarly represents the end
  // control point.
  //
  // See the document on "Dual Cubics" for a derivation of this solution.
  const float y_diff = end.y - start.y;
  const float s_diff = end.derivative - start.derivative;
  const float derivative_k = Lerp(end.derivative, start.derivative, k);
  const float y_k = Lerp(start.y, end.y, k);
  const float second_k = Lerp(end.second_derivative,
                              start.second_derivative, k);
  const float j = 1.0f - k;
  const float second_k_squared = k * k * start.second_derivative -
                                 j * j * end.second_derivative;

  const float s = 3.0f * y_diff - 2.0f * derivative_k - 0.5f * second_k_squared;
  const float y = y_k + k * j * (-2.0f / 3.0f * s_diff +
                                  1.0f / 6.0f * second_k);
  const float x = Lerp(start_wide.x, end_wide.x, k);

  return SplineControlPoint(x, y, s / x_width, 0.0f);
}

std::string DualCubicSpline::Text() const {
  std::ostringstream text;
  text << "start, mid, end x: (" <<
      start_x_ << ", " << Evaluate(start_x_) << ", " <<
      Derivative(start_x_) << ", " << SecondDerivative(start_x_) << "), (" <<
      mid_x_ << ", " << Evaluate(mid_x_) << ", " <<
      Derivative(mid_x_) << ", " << SecondDerivative(mid_x_) << "), (" <<
      end_x_ << ", " << Evaluate(end_x_) << ", " <<
      Derivative(end_x_) << ", " << SecondDerivative(end_x_) <<  "); " <<
      "Start: " << start_.Text() << ", End: " << end_.Text();
  return text.str();
}


} // namespace fpl

