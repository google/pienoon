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
using fpl::CubicInitWithWidth;
using fpl::DualCubicSpline;
using fpl::Range;
using mathfu::vec2;
using mathfu::vec2i;


class CurveTests : public ::testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

static void CheckQuadraticRoots(const QuadraticCurve& s,
                                int num_expected_roots) {
  // Ensure we have the correct number of roots.
  std::vector<float> roots;
  s.Roots(&roots);
  const int num_roots = static_cast<int>(roots.size());
  EXPECT_EQ(num_expected_roots, num_roots);

  // Ensure roots all evaluate to zero.
  const float epsilon = s.Epsilon();
  for (int i = 0; i < num_roots; ++i) {
    const float should_be_zero = s.Evaluate(roots[i]);
    EXPECT_LT(fabs(should_be_zero), epsilon);
  }
}

static void CheckCriticalPoint(const QuadraticCurve& s) {
  // Derivative should be zero at critical point.
  const float epsilon = s.Epsilon();
  const float critical_point_x = s.CriticalPoint();
  const float critical_point_derivative = s.Derivative(critical_point_x);
  EXPECT_LT(fabs(critical_point_derivative), epsilon);
}

TEST_F(CurveTests, QuadraticRoot_UpwardsAbove) {
  // Curves upwards, critical point above zero ==> no real roots.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 6.0f), 0);
}

TEST_F(CurveTests, QuadraticRoot_UpwardsAt) {
  // Curves upwards, critical point at zero ==> one real roots.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 4.26666689f), 1);
}

TEST_F(CurveTests, QuadraticRoot_UpwardsBelow) {
  // Curves upwards, critical point below zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(60.0f, -32.0f, 4.0f), 2);
}

TEST_F(CurveTests, QuadraticRoot_DownwardsAbove) {
  // Curves downwards, critical point above zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f, 0.0001f), 2);
}

TEST_F(CurveTests, QuadraticRoot_DownwardsAt) {
  // Curves downwards, critical point above zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f,
                                     -0.0000032666619999999896f), 1);
}

TEST_F(CurveTests, QuadraticRoot_DownwardsBelow) {
  // Curves downwards, critical point above zero ==> two real roots.
  CheckQuadraticRoots(QuadraticCurve(-0.00006f, -0.000028f, -0.000006f), 0);
}

TEST_F(CurveTests, QuadraticCriticalPoint) {
  // Curves upwards, critical point above zero ==> no real roots.
  CheckCriticalPoint(QuadraticCurve(60.0f, -32.0f, 6.0f));
}

TEST_F(CurveTests, CubicWithWidth) {
  const CubicInitWithWidth init(1.0f, -8.0f, 0.3f, -4.0f, 1.0f);
  const CubicCurve c(init);
  const float epsilon = c.Epsilon();
  EXPECT_LT(fabs(c.Evaluate(init.width_x) - init.end_y), epsilon);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

