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

#ifndef FPL_CURVE_H_
#define FPL_CURVE_H_

#include "common.h"
#include "range.h"


namespace fpl {


enum CurveValueType {
  kCurveValue,
  kCurveDerivative,
  kCurveSecondDerivative,
  kCurveThirdDerivative,
};


static const int kDefaultGraphWidth = 80;
static const int kDefaultGraphHeight = 30;
static const mathfu::vec2i kDefaultGraphSize(kDefaultGraphWidth,
                                             kDefaultGraphHeight);

// 2^22 = the max precision of significand.
static const float kEpsilonScale = 1.0f / static_cast<float>(1 << 22);


// Match start and end values, and start derivative.
// Start is x = 0. End is x = 1.
struct QuadraticInitWithStartDerivative {
  QuadraticInitWithStartDerivative(
      const float start_y, const float start_derivative, const float end_y) :
    start_y(start_y), start_derivative(start_derivative), end_y(end_y) {
  }

  float start_y;
  float start_derivative;
  float end_y;
};


// Represent a quadratic polynomial in the form,
//   c_[2] * x^2  +  c_[1] * x  +  c_[0]
class QuadraticCurve {
  static const int kNumCoeff = 3;

 public:
  QuadraticCurve() { memset(c_, 0, sizeof(c_)); }
  QuadraticCurve(const float c2, const float c1, const float c0) {
    c_[2] = c2; c_[1] = c1; c_[0] = c0;
  }
  QuadraticCurve(const float* c) { memcpy(c_, c, sizeof(c_)); }
  QuadraticCurve(const QuadraticInitWithStartDerivative& init) { Init(init); }
  void Init(const QuadraticInitWithStartDerivative& init);

  // f(x) = c2*x^2 + c1*x + c0
  float Evaluate(const float x) const {
    return (c_[2] * x + c_[1]) * x + c_[0];
  }

  // f'(x) = 2*c2*x + c1
  float Derivative(const float x) const {
    return 2.0f * c_[2] * x  +  c_[1];
  }

  // f''(x) = 2*c2
  float SecondDerivative(const float x) const {
    (void)x;
    return 2.0f * c_[2];
  }

  // f'''(x) = 0
  float ThirdDerivative(const float x) const {
    (void)x;
    return 0.0f;
  }

  // Returns a value below which floating point precision is unreliable.
  // If we're testing for zero, for instance, we should test against this
  // Epsilon().
  float Epsilon() const {
    using std::max;
    const float max_c = max(max(fabs(c_[2]), fabs(c_[1])), fabs(c_[0]));
    return max_c * kEpsilonScale;
  }

  // Used for finding roots, and more.
  // See http://en.wikipedia.org/wiki/Discriminant
  float Discriminant() const { return c_[1] * c_[1] - 4.0f * c_[2] * c_[0]; }

  // When Discriminant() is close to zero, set to zero.
  // Often floating point precision problems can make the discriminant
  // very slightly even though mathematically it should be zero.
  float ReliableDiscriminant(const float epsilon) const;

  // Return point at which derivative is zero.
  float CriticalPoint() const {
    assert(fabs(c_[2]) >= Epsilon());

    // 0 = f'(x) = 2*c2*x + c1  ==>  x = -c1 / 2c2
    return -(c_[1] / c_[2]) * 0.5f;
  }

  // Calculate the x-coordinates where this quadratic is zero, and put them into
  // 'roots', in ascending order. Returns the 0, 1, or 2, the number of unique
  // values put into 'roots'.
  void Roots(std::vector<float>* roots) const;

  // Calculate the x-coordinates within the range [start_x, end_x] where the
  // quadratic is zero. Put them into 'roots'. Return the number of unique
  // values put into 'roots'.
  void RootsInRange(const Range& x_limits, std::vector<float>* roots) const;

  // Get ranges above or below zero. Since a quadratic can cross zero at most
  // twice, there can be at most two ranges. Ranges are clamped to 'x_limits'.
  // 'sign' is used to determine above or below zero only--actual value is
  // ignored.
  void RangesMatchingSign(const Range& x_limits, float sign,
                          std::vector<Range>* matching) const;
  void RangesAboveZero(const Range& x_limits,
                       std::vector<Range>* matching) const {
    return RangesMatchingSign(x_limits, 1.0f, matching);
  }
  void RangesBelowZero(const Range& x_limits,
                       std::vector<Range>* matching) const {
    return RangesMatchingSign(x_limits, -1.0f, matching);
  }

  // Returns the coefficient for x to the ith power.
  float Coeff(int i) const { return c_[i]; }

  // Returns the number of coefficients in this curve.
  int NumCoeff() const { return kNumCoeff; }

 private:
  float c_[kNumCoeff]; // c_[2] * x^2  +  c_[1] * x  +  c_[0]
};


// Match start and end y-values and derivatives.
// Start is x = 0. End is x = 1.
struct CubicInitWithDerivatives {
  CubicInitWithDerivatives(const float start_y, const float start_derivative,
                           const float end_y, const float end_derivative) :
      start_y(start_y),
      start_derivative(start_derivative),
      end_y(end_y),
      end_derivative(end_derivative) {
  }
                          // short-form in comments:
  float start_y;          // y0
  float start_derivative; // s0
  float end_y;            // y1
  float end_derivative;   // s1
};

// Match start and end y-values and derivatives.
// Start is x = 0. End is x = width_x.
struct CubicInitWithWidth : public CubicInitWithDerivatives {
  CubicInitWithWidth(const float start_y, const float start_derivative,
                     const float end_y, const float end_derivative,
                     const float width_x) :
      CubicInitWithDerivatives(start_y, start_derivative, end_y,
                               end_derivative),
      width_x(width_x) {
  }
                          // short-form in comments:
  float width_x;          // w
};


// Represent a cubic polynomial of the form,
//   c_[3] * x^3  +  c_[2] * x^2  +  c_[1] * x  +  c_[0]
class CubicCurve {
  static const int kNumCoeff = 4;

 public:
  CubicCurve() { memset(c_, 0, sizeof(c_)); }
  CubicCurve(const float c3, const float c2, const float c1, const float c0) {
    c_[3] = c3; c_[2] = c2; c_[1] = c1; c_[0] = c0;
  }
  CubicCurve(const float* c) { memcpy(c_, c, sizeof(c_)); }
  CubicCurve(const CubicInitWithDerivatives& init) { Init(init); }
  CubicCurve(const CubicInitWithWidth& init) { Init(init); }
  void Init(const CubicInitWithDerivatives& init);
  void Init(const CubicInitWithWidth& init);

  // f(x) = c3*x^3 + c2*x^2 + c1*x + c0
  float Evaluate(const float x) const {
    // Take advantage of multiply-and-add instructions that are common on FPUs.
    return ((c_[3] * x + c_[2]) * x + c_[1]) * x + c_[0];
  }

  // f'(x) = 3*c3*x^2 + 2*c2*x + c1
  float Derivative(const float x) const {
    return (3.0f * c_[3] * x + 2.0f * c_[2]) * x + c_[1];
  }

  // f''(x) = 6*c3*x + 2*c2
  float SecondDerivative(const float x) const {
    return  6.0f * c_[3] * x + 2.0f * c_[2];
  }

  // f'''(x) = 6*c3
  float ThirdDerivative(const float x) const {
    (void)x;
    return 6.0f * c_[3];
  }

  // Returns true if always curving upward or always curving downward on the
  // specified x_limits.
  bool UniformCurvature(const Range& x_limits) const;

  // Return a value below which floating point precision is unreliable.
  // If we're testing for zero, for instance, we should test against this
  // Epsilon().
  float Epsilon() const {
    using std::max;
    const float max_c = max(max(max(fabs(c_[3]), fabs(c_[2])), fabs(c_[1])),
                                    fabs(c_[0]));
    return max_c * kEpsilonScale;
  }

  // Returns the coefficient for x to the ith power.
  float Coeff(int i) const { return c_[i]; }

  // Returns the number of coefficients in this curve.
  int NumCoeff() const { return kNumCoeff; }

  std::string Text() const;

 private:
  float c_[kNumCoeff]; // c_[3] * x^3  +  c_[2] * x^2  +  c_[1] * x  +  c_[0]
};


// Draw an ASCII-art graph of the array of (x,y) 'points'.
// The size of the graph in (horizontal characters, vertical lines) is given
// by 'size'.
std::string Graph2DPoints(const mathfu::vec2* points, const int num_points,
                          const mathfu::vec2i& size);

// Slow function that returns one of the possible values that this curve
// can evaluate. Useful for debugging.
template<class T>
float CurveValue(const T& curve, const float x,
                 const CurveValueType value_type) {
  switch(value_type) {
    case kCurveValue: return curve.Evaluate(x);
    case kCurveDerivative: return curve.Derivative(x);
    case kCurveSecondDerivative: return curve.SecondDerivative(x);
    case kCurveThirdDerivative: return curve.ThirdDerivative(x);
    default:
      assert(false);
  }
  return 0.0f;
}

// Returns an ASCII-art graph for x in 'x_range' for the requested type.
template<class T>
std::string GraphCurveOnXRange(
    const T& curve, const CurveValueType value_type,
    const Range& x_range, const mathfu::vec2i& size = kDefaultGraphSize) {

  // Gather a collection of (x,y) points to graph.
  const int num_points = size.x();
  std::vector<mathfu::vec2> points(num_points);
  const float inc_x = x_range.Length() / (num_points - 1);
  float x = x_range.start();
  for (int i = 0; i < num_points; ++i) {
    points[i] = mathfu::vec2(x, CurveValue(curve, x, value_type));
    x += inc_x;
  }

  // Output the points in an ASCII-art graph.
  return Graph2DPoints(&points[0], num_points, size);
}

// Returns an ASCII-art graph from StartX()() to EndX() for the requested type.
template<class T>
std::string GraphCurve(
    const T& curve, const CurveValueType value_type,
    const mathfu::vec2i& size = kDefaultGraphSize) {
  return curve.Text() + "\n" + GraphCurveOnXRange(curve, value_type,
                                  Range(curve.StartX(), curve.EndX()), size);
}


} // namespace fpl

#endif // FPL_CURVE_H_

