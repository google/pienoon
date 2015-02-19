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

#ifndef BEZIER_CURVE_H_
#define BEZIER_CURVE_H_

// TODO: Move this into mathfu.
namespace fpl {

// A cubic Bezier curve.
// Can be initialized with start and end positions and derivatives.
// Start is when x = 0. End is when x = 1. These values are chosen purposefully
// to keep the domain close to 0.
// Bezier curves are nice because they are generally well behaved in between
// the start and end points.
template <class Vector, class Scalar>
class BezierCurve {
 public:
  BezierCurve()
      : a(static_cast<Vector>(0.0f)),
        b(static_cast<Vector>(0.0f)),
        c(static_cast<Vector>(0.0f)),
        d(static_cast<Vector>(0.0f)),
        start_x(static_cast<Scalar>(0.0f)),
        one_over_width_x(static_cast<Scalar>(0.0f)) {}

  void Initialize(const Vector& start_value, const Vector& start_derivative,
                  const Vector& end_value, const Vector& end_derivative,
                  Scalar start_x_param, Scalar end_x_param) {
    // We scale the input so that start is x = 0, and end is x = 1.
    // We want to find Bezier curve B(x) such that,
    //   B(0) = start_value,
    //   B'(0) = start_derivative,
    //   B(1) = end_value,
    //   B'(1) = end_derivative,
    //
    // B(x) = ax^3  +  bx^2(1 - x)  +  cx(1 - x)^2  +  d(1 - x)^3
    // B'(x) = (3a - b)x^2  +  2(b - c)x(1 - x)  +  (c - 3d)(1 - x)^2
    //
    // Substituting 0 and 1 into the equations above gives,
    //   B(0) = d         ==>  d = start_value
    //   B(1) = a         ==>  a = end_value
    //   B'(0) = c - 3d   ==>  c = 3*start_value + start_derivative
    //   B'(1) = 3a - b   ==>  b = 3*end_value - end_derivative
    //
    a = end_value;
    b = 3.0f * end_value - end_derivative;
    c = 3.0f * start_value + start_derivative;
    d = start_value;
    start_x = start_x_param;
    one_over_width_x = 1.0f / (end_x_param - start_x_param);
    assert(one_over_width_x < std::numeric_limits<Scalar>::infinity());
  }

  Vector Evaluate(Scalar unscaled_x) const {
    const Scalar x = ScaleX(unscaled_x);

    // B(x) = ax^3  +  bx^2(1 - x)  +  cx(1 - x)^2  +  d(1 - x)^3
    const Scalar one_minus_x = 1.0f - x;
    const Scalar one_minus_x_squared = one_minus_x * one_minus_x;
    const Scalar one_minus_x_cubed = one_minus_x_squared * one_minus_x;
    const Scalar x_squared = x * x;
    const Scalar x_cubed = x_squared * x;
    return a * x_cubed + b * x_squared * one_minus_x +
           c * x * one_minus_x_squared + d * one_minus_x_cubed;
  }

  Vector Derivative(Scalar unscaled_x) const {
    const Scalar x = ScaleX(unscaled_x);

    // B'(x) = 3at^2  +  b(2t(1 - t) + -t^2)  +
    //         c((1 - t)^2 + -2(1 - t)t)  -  3d(1 - t)^2
    //       = (3a - b)x^2  +  2(b - c)(x(1 - x)  +  (c - 3d)(1 - x)^2
    const Scalar one_minus_x = 1.0f - x;
    return (3.0f * a - b) * x * x + 2.0f * (b - c) * x * one_minus_x +
           (c - 3.0f * d) * one_minus_x * one_minus_x;
  }

  Vector SecondDerivative(Scalar unscaled_x) const {
    const Scalar x = ScaleX(unscaled_x);

    // B''(x) = 2(3a - b)x  +  2(b - c)((1 - x) - x)  -  2(c - 3d)(1 - x)
    //        = (6a - 2b)x  +  (2b - 2c)(1 - 2x)  +  (-2c + 6d)(1 - x)
    //        = (6a - 2b - 4b + 4c + 2c - 6d)x  + (2b - 2c - 2c + 6d)
    //        = (6a - 6b + 6c - 6d)x  +  (2b - 4c + 6d)
    //        = 6(a - b + c - d)x  +  2(b - 2c + 3d)
    return 6.0f * (a - b + c - d) * x + 2.0f * (b - 2.0f * c + 3.0f * d);
  }

 private:
  Scalar ScaleX(Scalar unscaled_x) const {
    return mathfu::Clamp<Scalar>((unscaled_x - start_x) * one_over_width_x,
                                 static_cast<Scalar>(0.0f),
                                 static_cast<Scalar>(1.0f));
  }

  // B(x) = ax^3  +  bx^2(1 - x)  +  cx(1 - x)^2  +  d(1 - x)^3
  Vector a, b, c, d;

  // The valid range for x starts at start_x. X's before start_x get clamped.
  Scalar start_x;

  // The valid range for x ends at start_x + 1/one_over_width_x.
  // We record one-over the width to eliminate a floating point division.
  Scalar one_over_width_x;
};

}  // namespace fpl

#endif  // BEZIER_CURVE_H_
