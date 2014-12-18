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

#ifndef FPL_SPLINE_H_
#define FPL_SPLINE_H_

#include "common.h"
#include "curve.h"


namespace fpl {


struct SplineControlPoint {
  SplineControlPoint() :
      x(0.0f), y(0.0f), derivative(0.0f), second_derivative(0.0f) {
  }
  SplineControlPoint(float x, float y, float derivative,
                     float second_derivative = 0.0f) :
      x(x), y(y), derivative(derivative), second_derivative(second_derivative) {
  }

  float x;
  float y;
  float derivative;
  float second_derivative;
};


// TODO: Delete this class and replace with a cubic spline evaluator.
// The cubic spline evaluator takes a series of control points and
// interpolates them. In fact, it should take several series so that we
// can interpolate them in bulk with SIMD.
//
// The logic in this class should be pulled into a utility function that
// inserts control points whenever the cubic spline is poorly behaved.


// Interpolate between two control points using two cubics, joined in the
// middle.
class DualCubicSpline {
 public:
  DualCubicSpline() : start_x_(0.0f), mid_x_(0.0f), end_x_(0.0f) {}
  DualCubicSpline(const SplineControlPoint& start,
                  const SplineControlPoint& end) {
    Initialize(start, end);
  }
  void Initialize(const SplineControlPoint& start,
                  const SplineControlPoint& end);
  void Initialize(const SplineControlPoint& start,
                  const SplineControlPoint& end, const float mid_percent);

  float Evaluate(const float x) const {
    float u;
    const CubicCurve& c = Cubic(x, &u);
    return c.Evaluate(u);
  }
  float Derivative(const float x) const {
    float u;
    const CubicCurve& c = Cubic(x, &u);
    return c.Derivative(u);
  }
  float SecondDerivative(const float x) const {
    float u;
    const CubicCurve& c = Cubic(x, &u);
    return c.SecondDerivative(u);
  }
  float ThirdDerivative(const float x) const {
    float u;
    const CubicCurve& c = Cubic(x, &u);
    return c.ThirdDerivative(u);
  }

  float StartX() const { return start_x_; }
  float MidX() const { return mid_x_; }
  float EndX() const { return end_x_; }
  float WidthX() const { return EndX() - StartX(); }
  bool Valid() const;

  static Range CalculateValidMidRange(const SplineControlPoint& start,
                                      const SplineControlPoint& end,
                                      bool* is_valid = nullptr);
  static Range SecondDerivativeRangeForStart(const SplineControlPoint& start,
                                             const SplineControlPoint& end,
                                             const float mid_percent);
  static Range SecondDerivativeRangeForEnd(const SplineControlPoint& start,
                                           const SplineControlPoint& end,
                                           const float mid_percent);

 private:
  bool IsStart(const float x) const { return x < mid_x_; }
  const CubicCurve& Cubic(const float x, float* u) const {
    if (IsStart(x)) {
      *u = x - start_x_;
      return start_;
    } else {
      *u = x - mid_x_;
      return end_;
    }
  }

  static float CalculateMidPercent(const SplineControlPoint& start,
                                   const SplineControlPoint& end);

  static SplineControlPoint CalculateMidPoint(const SplineControlPoint& start,
                                              const SplineControlPoint& end,
                                              const float mid_percent);

  static QuadraticCurve CalculateValidMidRangeSplineForStart(
      const SplineControlPoint& start, const SplineControlPoint& end);

  static QuadraticCurve CalculateValidMidRangeSplineForEnd(
      const SplineControlPoint& start, const SplineControlPoint& end);

  CubicCurve start_;
  CubicCurve end_;
  float start_x_;
  float mid_x_;
  float end_x_;
};



} // namespace fpl

#endif // FPL_SPLINE_H_

