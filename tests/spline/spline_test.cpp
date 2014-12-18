/*
* Copyright (c) 2014 Google, Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "gtest/gtest.h"
#include "spline.h"
#include "common.h"

using fpl::SplineControlPoint;
using fpl::QuadraticCurve;
using fpl::CubicCurve;
using fpl::CubicInitWithDerivatives;
using fpl::CubicInitWithWidth;
using fpl::DualCubicSpline;
using fpl::Range;
using mathfu::vec2;
using mathfu::vec2i;

static const int kNumCheckPoints = fpl::kDefaultGraphWidth;


class SplineTests : public ::testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

// Ensure we wrap around from pi to -pi.
TEST_F(SplineTests, Overshoot) {
  DualCubicSpline s;
  SplineControlPoint start(0.0f, 1.0f, -8.0f, 60.0f);
  SplineControlPoint end(1.0f, 0.0f, 0.0f, 0.001f);
  s.Initialize(start, end);

  float x = 0.0f;
  for (int i = 0; i < kNumCheckPoints; ++i) {
    printf("%f, %f, %f, %f, %f\n",
           x, s.Evaluate(x), s.Derivative(x), s.SecondDerivative(x),
           s.ThirdDerivative(x));
    x += 1.0f / (kNumCheckPoints - 1);
  }

  printf("\n%s\n\n", GraphCurve(s, fpl::kCurveValue).c_str());

  // Ensure the cubics have uniform curvature.
  EXPECT_TRUE(s.Valid());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

