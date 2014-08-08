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
    above_pi_ = MinutelyDifferentFloat(fpl::kPi, 1);
    below_pi_ = MinutelyDifferentFloat(fpl::kPi, -1);
    above_negative_pi_ = MinutelyDifferentFloat(-fpl::kPi, -1);
    below_negative_pi_ = MinutelyDifferentFloat(-fpl::kPi, 1);
    half_pi_ = fpl::Angle(fpl::kHalfPi);
  }
  virtual void TearDown() {}

protected:
  float above_pi_;
  float below_pi_;
  float above_negative_pi_;
  float below_negative_pi_;
  fpl::Angle half_pi_;
};

// Ensure the constants are in (or not in) the valid angle range.
TEST_F(AngleTests, RangeExtremes) {
  EXPECT_TRUE(fpl::Angle::IsAngleInRange(fpl::kPi));
  EXPECT_FALSE(fpl::Angle::IsAngleInRange(-fpl::kPi));
  EXPECT_TRUE(fpl::Angle::IsAngleInRange(fpl::kMinUniqueAngle));
  EXPECT_TRUE(fpl::Angle::IsAngleInRange(fpl::kMaxUniqueAngle));
}

// Ensure constant values are what we expect them to be.
TEST_F(AngleTests, RangeConstants) {
  EXPECT_FLOAT_EQ(above_negative_pi_, fpl::kMinUniqueAngle);
  EXPECT_FLOAT_EQ(static_cast<float>(M_PI), fpl::kMaxUniqueAngle);
}

// Ensure the smallest value above pi is false.
TEST_F(AngleTests, AbovePi) {
  EXPECT_FALSE(fpl::Angle::IsAngleInRange(above_pi_));
}

// Ensure the smallest value above -pi is true.
TEST_F(AngleTests, AboveNegativePi) {
  EXPECT_TRUE(fpl::Angle::IsAngleInRange(above_negative_pi_));
}

// -pi should be represented as pi.
TEST_F(AngleTests, ModFromNegativePi) {
  EXPECT_FLOAT_EQ(fpl::Angle::FromWithinThreePi(-fpl::kPi).angle(), fpl::kPi);
}

// pi should be represented as pi.
TEST_F(AngleTests, ModFromPositivePi) {
  EXPECT_FLOAT_EQ(fpl::Angle::FromWithinThreePi(fpl::kPi).angle(), fpl::kPi);
}

// Slightly below -pi should mod to near pi.
TEST_F(AngleTests, ModBelowNegativePi) {
  const fpl::Angle a = fpl::Angle::FromWithinThreePi(below_negative_pi_);
  EXPECT_TRUE(a.IsValid());
}

// Slightly above pi should mod to near -pi (but above).
TEST_F(AngleTests, ModAbovePi) {
  const fpl::Angle a = fpl::Angle::FromWithinThreePi(above_pi_);
  EXPECT_TRUE(a.IsValid());
}

// Addition should use modular arithmetic.
TEST_F(AngleTests, Addition) {
  const fpl::Angle sum = half_pi_ + half_pi_ + half_pi_ + half_pi_;
  EXPECT_NEAR(sum.angle(), 0.0f, kAnglePrecision);
}

// Subtraction should use modular arithmetic.
TEST_F(AngleTests, Subtraction) {
  const fpl::Angle diff = half_pi_ - half_pi_ - half_pi_ - half_pi_;
  EXPECT_NEAR(diff.angle(), fpl::kPi, kAnglePrecision);
}

// Unary negate should change the sign.
TEST_F(AngleTests, Negate) {
  const fpl::Angle a = fpl::Angle(fpl::kHalfPi);
  EXPECT_FLOAT_EQ(-a.angle(), -fpl::kHalfPi);
}

// Unary negate should send pi to pi, because -pi is not in range.
TEST_F(AngleTests, NegatePi) {
  const fpl::Angle a = fpl::Angle(fpl::kPi);
  const fpl::Angle negative_a = -a;
  EXPECT_FLOAT_EQ(negative_a.angle(), fpl::kPi);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
