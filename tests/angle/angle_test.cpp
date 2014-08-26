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

#define FPL_ANGLE_UNIT_TESTS

#include <string>
#include "angle.h"
#include "gtest/gtest.h"

using fpl::Angle;
using fpl::kPi;
using fpl::kHalfPi;
using fpl::kMinUniqueAngle;
using fpl::kMaxUniqueAngle;

union FloatInt {
  float f;
  int i;
};

static const float kAnglePrecision = 0.0000005f;

// When diff is +-1, returns the smallest float value that is different than f.
// Note: You can construct examples where this function fails. But it works in
// almost all cases, and is good for the values we use in these tests.
static float MinutelyDifferentFloat(float f, int diff) {
  FloatInt u;
  u.f = f;
  u.i += diff;
  return u.f;
}

class AngleTests : public ::testing::Test {
protected:
  virtual void SetUp()
  {
    above_pi_ = MinutelyDifferentFloat(kPi, 1);
    below_pi_ = MinutelyDifferentFloat(kPi, -1);
    above_negative_pi_ = MinutelyDifferentFloat(-kPi, -1);
    below_negative_pi_ = MinutelyDifferentFloat(-kPi, 1);
    half_pi_ = Angle(kHalfPi);
  }
  virtual void TearDown() {}

protected:
  float above_pi_;
  float below_pi_;
  float above_negative_pi_;
  float below_negative_pi_;
  Angle half_pi_;
};

// Ensure the constants are in (or not in) the valid angle range.
TEST_F(AngleTests, RangeExtremes) {
  EXPECT_TRUE(Angle::IsAngleInRange(kPi));
  EXPECT_FALSE(Angle::IsAngleInRange(-kPi));
  EXPECT_TRUE(Angle::IsAngleInRange(kMinUniqueAngle));
  EXPECT_TRUE(Angle::IsAngleInRange(kMaxUniqueAngle));
}

// Ensure constant values are what we expect them to be.
TEST_F(AngleTests, RangeConstants) {
  EXPECT_FLOAT_EQ(above_negative_pi_, kMinUniqueAngle);
  EXPECT_FLOAT_EQ(static_cast<float>(M_PI), kMaxUniqueAngle);
}

// Ensure the smallest value above pi is false.
TEST_F(AngleTests, AbovePi) {
  EXPECT_FALSE(Angle::IsAngleInRange(above_pi_));
}

// Ensure the smallest value above -pi is true.
TEST_F(AngleTests, AboveNegativePi) {
  EXPECT_TRUE(Angle::IsAngleInRange(above_negative_pi_));
}

// -pi should be represented as pi.
TEST_F(AngleTests, ModFromNegativePi) {
  EXPECT_FLOAT_EQ(Angle::FromWithinThreePi(-kPi).angle(), kPi);
}

// pi should be represented as pi.
TEST_F(AngleTests, ModFromPositivePi) {
  EXPECT_FLOAT_EQ(Angle::FromWithinThreePi(kPi).angle(), kPi);
}

// Slightly below -pi should mod to near pi.
TEST_F(AngleTests, ModBelowNegativePi) {
  const Angle a = Angle::FromWithinThreePi(below_negative_pi_);
  EXPECT_TRUE(a.IsValid());
}

// Slightly above pi should mod to near -pi (but above).
TEST_F(AngleTests, ModAbovePi) {
  const Angle a = Angle::FromWithinThreePi(above_pi_);
  EXPECT_TRUE(a.IsValid());
}

// Addition should use modular arithmetic.
TEST_F(AngleTests, Addition) {
  const Angle sum = half_pi_ + half_pi_ + half_pi_ + half_pi_;
  EXPECT_NEAR(sum.angle(), 0.0f, kAnglePrecision);
}

// Subtraction should use modular arithmetic.
TEST_F(AngleTests, Subtraction) {
  const Angle diff = half_pi_ - half_pi_ - half_pi_ - half_pi_;
  EXPECT_NEAR(diff.angle(), kPi, kAnglePrecision);
}

// Addition should use modular arithmetic.
TEST_F(AngleTests, Multiplication) {
  const Angle product = half_pi_ * 3.f;
  EXPECT_NEAR(product.angle(), -half_pi_.angle(), kAnglePrecision);
}

// Subtraction should use modular arithmetic.
TEST_F(AngleTests, Division) {
  const Angle quotient = Angle::FromWithinThreePi(kPi) / 2.0f;
  EXPECT_NEAR(quotient.angle(), kHalfPi, kAnglePrecision);
}

// Unary negate should change the sign.
TEST_F(AngleTests, Negate) {
  const Angle a = Angle(kHalfPi);
  EXPECT_FLOAT_EQ(-a.angle(), -kHalfPi);
}

// Unary negate should send pi to pi, because -pi is not in range.
TEST_F(AngleTests, NegatePi) {
  const Angle a = Angle(kPi);
  const Angle negative_a = -a;
  EXPECT_FLOAT_EQ(negative_a.angle(), kPi);
}

// Ensure wrapping produces angles in the range (-pi, pi].
TEST_F(AngleTests, WrapAngleTest) {
  const float a1 = Angle::WrapAngle(
      static_cast<float>(-M_PI - M_2_PI - M_2_PI));
  EXPECT_TRUE(Angle::IsAngleInRange(a1));

  const float a2 = Angle::WrapAngle(static_cast<float>(-M_PI - M_2_PI));
  EXPECT_TRUE(Angle::IsAngleInRange(a2));

  const float a3 = Angle::WrapAngle(static_cast<float>(-M_PI));
  EXPECT_TRUE(Angle::IsAngleInRange(a3));

  const float a4 = Angle::WrapAngle(0.f);
  EXPECT_TRUE(Angle::IsAngleInRange(a4));

  const float a5 = Angle::WrapAngle(static_cast<float>(M_PI + M_2_PI));
  EXPECT_TRUE(Angle::IsAngleInRange(a5));

  const float a6 = Angle::WrapAngle(
      static_cast<float>(M_PI + M_2_PI + M_2_PI));
  EXPECT_TRUE(Angle::IsAngleInRange(a6));
}

// Clamping a value that's inside the range should not change the value.
TEST_F(AngleTests, ClampInside) {
  const Angle a(kHalfPi + 0.1f);
  const Angle center(kHalfPi);
  const Angle max_diff(0.2f);
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(), a.angle());
}

// Clamping a value that's above the range should clamp to the top boundary.
TEST_F(AngleTests, ClampAbove) {
  const Angle a(kHalfPi + 0.2f);
  const Angle center(kHalfPi);
  const Angle max_diff(0.1f);
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center + max_diff).angle());
}

// Clamping a value that's below the range should clamp to the bottom boundary.
TEST_F(AngleTests, ClampBelow) {
  const Angle a(-kHalfPi - 0.2f);
  const Angle center(-kHalfPi);
  const Angle max_diff(0.1f);
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center - max_diff).angle());
}

// Clamping to a range that strattles pi should wrap to the boundary that's
// closest under modular arithmetic.
TEST_F(AngleTests, ClampModularAtPositiveCenterPositiveAngle) {
  const Angle a(kPi - 0.2f);
  const Angle center(kPi);
  const Angle max_diff(0.1f);
  // This tests a positive number clamped to the range.
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center - max_diff).angle());
}

// Clamping to a range that strattles pi should wrap to the boundary that's
// closest under modular arithmetic.
TEST_F(AngleTests, ClampModularAtPositiveCenterNegativeAngle) {
  const Angle a(-kPi + 1.1f);
  const Angle center(kPi);
  const Angle max_diff(0.1f);
  // This tests a negative number clamped to the range.
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center + max_diff).angle());
}

// Clamping to a range that strattles pi should wrap to the boundary that's
// closest under modular arithmetic.
TEST_F(AngleTests, ClampModularAtNegativeCenterPositiveAngle) {
  const Angle a(kPi - 0.2f);
  const Angle center(kMinUniqueAngle);
  const Angle max_diff(0.1f);
  // This tests a positive number clamped to a range centered about a negative
  // number.
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center - max_diff).angle());
}

// Clamping to a range that strattles pi should wrap to the boundary that's
// closest under modular arithmetic.
TEST_F(AngleTests, ClampModularAtNegativeCenterNegativeAngle) {
  const Angle a(-kPi + 1.1f);
  const Angle center(kMinUniqueAngle);
  const Angle max_diff(0.1f);
  // This tests a negative number clamped to the range.
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(),
                  (center + max_diff).angle());
}

// Clamping with zero diff should return the center.
TEST_F(AngleTests, ClampWithZeroDiff) {
  const Angle a(-kPi + 1.1f);
  const Angle center(kPi - 2.1f);
  const Angle max_diff(0.0f);
  // This tests a negative number clamped to the range.
  EXPECT_FLOAT_EQ(a.Clamp(center, max_diff).angle(), center.angle());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
