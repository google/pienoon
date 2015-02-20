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
#include "bulk_spline_evaluator.h"

using mathfu::vec2;
using mathfu::vec2i;

namespace fpl {

static float ClampNearZero(const float x, const float epsilon) {
  const bool is_near_zero = fabs(x) <= epsilon;
  return is_near_zero ? 0.0f : x;
}

void QuadraticCurve::Init(const QuadraticInitWithStartDerivative& init) {
  //  f(u) = cu^2 + bu + a
  //  f(0) = a
  //  f'(0) = b
  //  f(1) = c + b + a   ==>   c = f(1) - b - a
  //                             = f(1) - f(0) - f'(0)
  c_[0] = init.start_y;
  c_[1] = init.start_derivative;
  c_[2] = init.end_y - init.start_y - init.start_derivative;
}

float QuadraticCurve::ReliableDiscriminant(const float epsilon) const {
  // When discriminant is (relative to coefficients) close to zero, we treat
  // it as zero. It's possible that the discriminant is barely below zero due
  // to floating point error.
  const float discriminant = Discriminant();
  return ClampNearZero(discriminant, epsilon);
}

// See the Quadratic Formula for details:
// http://en.wikipedia.org/wiki/Quadratic_formula
// Roots returned in sorted order, smallest to largest.
size_t QuadraticCurve::Roots(float roots[2]) const {
  const float epsilon = Epsilon();

  // x^2 coefficient of zero means that curve is linear or constant.
  if (fabs(c_[2]) < epsilon) {
    // If constant, even if zero, return no roots. This is arbitrary.
    if (fabs(c_[1]) < epsilon) return 0;

    // Linear 0 = c1x + c0 ==> x = -c0 / c1.
    roots[0] = -c_[0] / c_[1];
    return 1;
  }

  // A negative discriminant means no real roots.
  const float discriminant = ReliableDiscriminant(epsilon);
  if (discriminant < 0.0f) return 0;

  // A zero discriminant means there is only one root.
  const float divisor = (1.0f / c_[2]) * 0.5f;
  if (discriminant == 0.0f) {
    roots[0] = -c_[1] * divisor;
    return 1;
  }

  // Positive discriminant means two roots. We use the quadratic formula.
  const float sqrt_discriminant = sqrt(discriminant);
  const float root_minus = (-c_[1] - sqrt_discriminant) * divisor;
  const float root_plus = (-c_[1] + sqrt_discriminant) * divisor;
  assert(root_minus != root_plus);
  roots[0] = std::min(root_minus, root_plus);
  roots[1] = std::max(root_minus, root_plus);
  return 2;
}

size_t QuadraticCurve::RootsInRange(const Range& valid_x,
                                    float roots[2]) const {
  const size_t num_roots = Roots(roots);

  // We allow the roots to be slightly outside the bounds, since this may
  // happen due to floating point error.
  const float epsilon_x = valid_x.Length() * kEpsilonScale;

  // Loop through each root and only return it if it is within the range
  // [start_x - epsilon_x, end_x + epsilon_x]. Clamp to [start_x, end_x].
  return Range::ValuesInRange(valid_x, epsilon_x, num_roots, roots);
}

size_t QuadraticCurve::RangesMatchingSign(const Range& x_limits, float sign,
                                          Range matching[2]) const {
  // Gather the roots of the validity spline. These are transitions between
  // valid and invalid regions.
  float roots[2];
  const size_t num_roots = RootsInRange(x_limits, roots);

  // We want ranges where the spline's sign equals valid_sign's.
  const bool valid_at_start = sign * Evaluate(x_limits.start()) >= 0.0f;
  const bool valid_at_end = sign * Evaluate(x_limits.end()) >= 0.0f;

  // If no roots, the curve never crosses zero, so the start and end validity
  // must be the same.
  // If two roots, the curve crosses zero twice, so the start and end validity
  // must be the same.
  assert(num_roots == 1 || valid_at_start == valid_at_end);

  // Starts invalid, and never crosses zero so never becomes valid.
  if (num_roots == 0 && !valid_at_start) return 0;

  // Starts valid, crosses zero to invalid, crosses zero again back to valid,
  // then ends valid.
  if (num_roots == 2 && valid_at_start) {
    matching[0] = Range(x_limits.start(), roots[0]);
    matching[1] = Range(roots[1], x_limits.end());
    return 2;
  }

  // If num_roots == 0: must be valid at both start and end so entire range.
  // If num_roots == 1: crosses zero once, or just touches zero.
  // If num_roots == 2: must start and end invalid, so valid range is between
  // roots.
  const float start = valid_at_start ? x_limits.start() : roots[0];
  const float end =
      valid_at_end ? x_limits.end() : num_roots == 2 ? roots[1] : roots[0];
  matching[0] = Range(start, end);
  return 1;
}

void CubicCurve::Init(const CubicInit& init) {
  //  f(x) = dx^3 + cx^2 + bx + a
  //
  // Solve for a and b by substituting with x = 0.
  //  y0 = f(0) = a
  //  s0 = f'(0) = b
  //
  // Solve for c and d by substituting with x = init.width_x = w. Gives two
  // linear equations with unknowns 'c' and 'd'.
  //  y1 = f(x1) = dw^3 + cw^2 + bw + a
  //  s1 = f'(x1) = 3dw^2 + 2cw + b
  //    ==> 3*y1 - w*s1 = (3dw^3 + 3cw^2 + 3bw + 3a) - (3dw^3 + 2cw^2 + bw)
  //        3*y1 - w*s1 = cw^2 - 2bw + 3a
  //               cw^2 = 3*y1 - w*s1 + 2bw - 3a
  //               cw^2 = 3*y1 - w*s1 + 2*s0*w - 3*y0
  //               cw^2 = 3(y1 - y0) - w*(s1 + 2*s0)
  //                  c = (3/w^2)*(y1 - y0) - (1/w)*(s1 + 2*s0)
  //    ==> 2*y1 - w*s1 = (2dw^3 + 2cw^2 + 2bw + 2a) - (3dw^3 + 2cw^2 + bw)
  //        2*y1 - w*s1 = -dw^3 + bw + 2a
  //               dw^3 = -2*y1 + w*s1 + bw + 2a
  //               dw^3 = -2*y1 + w*s1 + s0*w + 2*y0
  //               dw^3 = 2(y0 - y1) + w*(s1 + s0)
  //                  d = (2/w^3)*(y0 - y1) + (1/w^2)*(s1 + s0)
  const float one_over_w = 1.0f / init.width_x;
  const float one_over_w_sq = one_over_w * one_over_w;
  const float one_over_w_cubed = one_over_w_sq * one_over_w;
  c_[0] = init.start_y;
  c_[1] = init.start_derivative;
  c_[2] = 3.0f * one_over_w_sq * (init.end_y - init.start_y) -
          one_over_w * (init.end_derivative + 2.0f * init.start_derivative);
  c_[3] = 2.0f * one_over_w_cubed * (init.start_y - init.end_y) +
          one_over_w_sq * (init.end_derivative + init.start_derivative);
}

bool CubicCurve::UniformCurvature(const Range& x_limits) const {
  // Curvature is given by the second derivative. The second derivative is
  // linear. So, the curvature is uniformly positive or negative iff
  //     Sign(f''(x_limits.start)) == Sign(f''(x_limits.end))
  const float epsilon = Epsilon();
  const float start_second_derivative =
      ClampNearZero(SecondDerivative(x_limits.start()), epsilon);
  const float end_second_derivative =
      ClampNearZero(SecondDerivative(x_limits.end()), epsilon);
  return start_second_derivative * end_second_derivative >= 0.0f;
}

std::string CubicCurve::Text() const {
  std::ostringstream text;
  text << c_[3] << "x^3 + " << c_[2] << "x^2 + " << c_[1] << "x + " << c_[0];
  return text.str();
}

// TODO: Move these to mathfu and templatize.
static inline int Round(float f) { return static_cast<int>(f + 0.5f); }

static inline vec2i Round(const vec2& v) {
  return vec2i(Round(v.x()), Round(v.y()));
}

static inline vec2 Min(const vec2& a, const vec2& b) {
  return vec2(std::min(a.x(), b.x()), std::min(a.y(), b.y()));
}

static inline vec2 Max(const vec2& a, const vec2& b) {
  return vec2(std::max(a.x(), b.x()), std::max(a.y(), b.y()));
}

static inline bool CompareBigYSmallX(const vec2i& a, const vec2i& b) {
  return a.y() == b.y() ? a.x() < b.x() : a.y() > b.y();
}

std::string Graph2DPoints(const vec2* points, const int num_points,
                          const vec2i& size) {
#if defined(FPL_CURVE_GRAPH_FUNCTIONS)
  if (num_points == 0) return std::string();

  // Calculate x extents.
  vec2 min = points[0];
  vec2 max = points[0];
  for (const vec2* q = points + 1; q < &points[num_points]; ++q) {
    min = Min(min, *q);
    max = Max(max, *q);
  }
  const vec2 p_size = max - min;
  const vec2 gaps = vec2(size) - vec2(1.0f, 1.0f);
  const vec2 inc = p_size / gaps;
  const int zero_row = Round((0.0f - min.y()) * size.y() / p_size.y());

  // Convert to graph space on the screen.
  std::vector<vec2i> p;
  p.reserve(num_points);
  for (int i = 0; i < num_points; ++i) {
    p.push_back(Round((points[i] - min) / inc));
  }

  // Sort by Y, biggest to smallest, then by X smallest to largest.
  // This is the write order: top to bottom, left to right.
  std::sort(p.begin(), p.end(), CompareBigYSmallX);

  // Avoid reallocating the string by setting to a reasonable max size.
  std::string r;
  r.reserve(size.y() * size.x() + 100);

  // Iterate through each "pixel" of the graph.
  r += "y = " + std::to_string(max.y()) + "\n";
  const vec2i* q = &p[0];
  for (int row = size.y(); row >= 0; --row) {
    for (int col = 0; col <= size.x(); ++col) {
      if (q->x() == col && q->y() == row) {
        r += '*';
        q++;
        if (q >= &p[num_points]) break;
      } else if (col == 0) {
        r += '|';
      } else if (row == zero_row) {
        r += '-';
      } else if (q->y() > row) {
        break;
      } else {
        r += ' ';
      }
    }
    r += '\n';
    if (q >= &p[num_points]) break;
  }
  r += "y = " + std::to_string(min.y()) + "\n";
  return r;
#else
  (void)points;
  (void)num_points;
  (void)size;
  return std::string();
#endif  // defined(FPL_CURVE_GRAPH_FUNCTIONS)
}

}  // namespace fpl
