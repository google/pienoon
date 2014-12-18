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


static const float kInf = std::numeric_limits<float>::infinity();


class RangeTests : public ::testing::Test {
protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};


TEST_F(RangeTests, Valid) {
  // When start <= end, the range is considered valid.
  const Range valid(0.0f, 1.0f);
  EXPECT_TRUE(valid.Valid());

  // When end > start, the range is considered invalid.
  const Range invalid(1.0f, -1.0f);
  EXPECT_TRUE(valid.Valid());

  // By default, the range should be initialized to something invalid.
  const Range invalid_default;
  EXPECT_FALSE(invalid_default.Valid());
}

// Infinities should be able to be used in ranges.
TEST_F(RangeTests, Valid_Infinity) {
  const Range full(-kInf, kInf);
  EXPECT_TRUE(full.Valid());

  const Range neg_half(-kInf, 0.0f);
  EXPECT_TRUE(neg_half.Valid());

  const Range pos_half(-10.0, kInf);
  EXPECT_TRUE(pos_half.Valid());
}

// Ensure the middle of the range is the algebraic middle.
TEST_F(RangeTests, Middle) {
  // Test positive range.
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.5f, zero_one.Middle());

  // Test range that spans zero.
  const Range minus_one_one(-1.0f, 1.0f);
  EXPECT_EQ(0.0f, minus_one_one.Middle());
}

// Ensure the length is the width of the interval.
TEST_F(RangeTests, Length) {
  // Test positive range.
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(1.0f, zero_one.Length());

  // Test range that spans zero.
  const Range minus_one_one(-1.0f, 1.0f);
  EXPECT_EQ(0.0f, minus_one_one.Middle());

  // Test range with infinity.
  const Range one_inf(1.0f, kInf);
  EXPECT_EQ(kInf, one_inf.Length());
}

// Clamping values inside the range should result in the same value.
TEST_F(RangeTests, Clamp_Inside) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.5f, zero_one.Clamp(0.5f));
  EXPECT_EQ(0.9999999f, zero_one.Clamp(0.9999999f));
}

// Clamping values inside the range should result in the same value.
TEST_F(RangeTests, Clamp_Border) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.0f, zero_one.Clamp(0.0f));
  EXPECT_EQ(1.0f, zero_one.Clamp(1.0f));
}

// Clamping values inside the range should result in the same value.
TEST_F(RangeTests, Clamp_Outside) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.0f, zero_one.Clamp(-1.0f));
  EXPECT_EQ(1.0f, zero_one.Clamp(1.0000001f));
}

// Passing infinity into a range should clamp fine.
TEST_F(RangeTests, Clamp_Infinity) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(1.0f, zero_one.Clamp(kInf));
  EXPECT_EQ(0.0f, zero_one.Clamp(-kInf));
}

// Clamping values to the full range should always return the original value.
TEST_F(RangeTests, Clamp_ToInfinity) {
  const Range full(-kInf, kInf);
  EXPECT_EQ(kInf, full.Clamp(kInf));
  EXPECT_EQ(1.0f, full.Clamp(1.0f));
  EXPECT_EQ(-kInf, full.Clamp(-kInf));
}

// Distance from the range should be zero for elements inside the range.
TEST_F(RangeTests, DistanceFrom_Inside) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.0f, zero_one.DistanceFrom(0.0000001f));
  EXPECT_EQ(0.0f, zero_one.DistanceFrom(0.5f));
  EXPECT_EQ(0.0f, zero_one.DistanceFrom(0.9f));
}

// Distance from the range should be zero for elements on the border.
TEST_F(RangeTests, DistanceFrom_Border) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(0.0f, zero_one.DistanceFrom(0.0f));
  EXPECT_EQ(0.0f, zero_one.DistanceFrom(1.0f));
}

// Distance from the range should be match for elements outside the range.
TEST_F(RangeTests, DistanceFrom_Outside) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(1.0f, zero_one.DistanceFrom(-1.0f));
  EXPECT_NEAR(0.2f, zero_one.DistanceFrom(1.2f), 0.000001f);
}

// Distance from the range should always be infinity for infinite values.
TEST_F(RangeTests, DistanceFrom_Infinity) {
  const Range zero_one(0.0f, 1.0f);
  EXPECT_EQ(kInf, zero_one.DistanceFrom(kInf));
  EXPECT_EQ(kInf, zero_one.DistanceFrom(-kInf));
}

// Distance from the full range should always be zero.
TEST_F(RangeTests, DistanceFrom_InfiniteRange) {
  const Range full(-kInf, kInf);
  EXPECT_EQ(0.0f, full.DistanceFrom(0.0f));
  EXPECT_EQ(0.0f, full.DistanceFrom(1.0f));
// Note: Doesn't work when passing in kInf because kInf - kInf = NaN.
//  EXPECT_EQ(0.0f, full.DistanceFrom(kInf));
//  EXPECT_EQ(0.0f, full.DistanceFrom(-kInf));
}

// 1.  |-a---|    |-b---|  ==>  return invalid
TEST_F(RangeTests, Intersect_DisjointBelow) {
  const Range a(0.0f, 1.0f);
  const Range b(2.0f, 3.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_FALSE(intersection.Valid());
  EXPECT_TRUE(intersection.Invert().Valid());
  EXPECT_EQ(intersection.Invert().Length(), 1.0f);
}

// 2.  |-b---|    |-a---|  ==>  return invalid
TEST_F(RangeTests, Intersect_DisjointAbove) {
  const Range a(2.0f, 3.0f);
  const Range b(0.0f, 1.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_FALSE(intersection.Valid());
  EXPECT_TRUE(intersection.Invert().Valid());
  EXPECT_EQ(intersection.Invert().Length(), 1.0f);
}

// 3.  |-a---------|       ==>  return b
//        |-b---|
TEST_F(RangeTests, Intersect_ContainsSecond) {
  const Range a(-10.0f, 10.0f);
  const Range b(2.0f, 3.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_TRUE(intersection.Valid());
  EXPECT_EQ(intersection, b);
}

// 4.  |-b---------|       ==>  return a
//        |-a---|
TEST_F(RangeTests, Intersect_ContainsFirst) {
  const Range a(2.0f, 3.0f);
  const Range b(-10.0f, 10.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_TRUE(intersection.Valid());
  EXPECT_EQ(intersection, a);
}


// 5.  |-a---|             ==>  return (b.start, a.end)
//        |-b---|
TEST_F(RangeTests, Intersect_OverlapFirst) {
  const Range a(0.0f, 2.0f);
  const Range b(1.0f, 3.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_EQ(intersection, Range(1.0f, 2.0f));
}

// 6.  |-b---|             ==>  return (a.start, b.end)
//        |-a---|
TEST_F(RangeTests, Intersect_OverlapSecond) {
  const Range a(1.0f, 3.0f);
  const Range b(0.0f, 2.0f);
  const Range intersection = Range::Intersect(a, b);
  EXPECT_EQ(intersection, Range(1.0f, 2.0f));
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

