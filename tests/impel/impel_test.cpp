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
#include "flatbuffers/flatbuffers.h"
#include "impel_engine.h"
#include "impel_init.h"
#include "mathfu/constants.h"
#include "angle.h"
#include "common.h"

#define DEBUG_PRINT_MATRICES 0

using fpl::kPi;
using fpl::kHalfPi;
using fpl::Range;
using impel::ImpelEngine;
using impel::Impeller1f;
using impel::ImpellerMatrix4f;
using impel::ImpelTime;
using impel::ImpelInit;
using impel::ImpelTarget1f;
using impel::OvershootImpelInit;
using impel::SmoothImpelInit;
using impel::MatrixImpelInit;
using impel::MatrixOperationInit;
using impel::Settled1f;
using impel::MatrixOperationType;
using impel::kRotateAboutX;
using impel::kRotateAboutY;
using impel::kRotateAboutZ;
using impel::kTranslateX;
using impel::kTranslateY;
using impel::kTranslateZ;
using impel::kScaleX;
using impel::kScaleY;
using impel::kScaleZ;
using impel::kScaleUniformly;
using mathfu::mat4;
using mathfu::vec3;

typedef mathfu::Matrix<float, 3> mat3;

static const ImpelTime kTimePerFrame = 10;
static const ImpelTime kMaxTime = 10000;
static const float kMatrixEpsilon = 0.00001f;
static const float kAngleEpsilon = 0.01f;


class ImpelTests : public ::testing::Test {
protected:
  virtual void SetUp()
  {
    const Range angle_range(-3.14159265359f, 3.14159265359f);
    impel::OvershootImpelInit::Register();
    impel::SmoothImpelInit::Register();
    impel::MatrixImpelInit::Register();

    // Create an OvershootImpelInit with reasonable values.
    overshoot_angle_init_.set_modular(true);
    overshoot_angle_init_.set_range(angle_range);
    overshoot_angle_init_.set_max_velocity(0.021f);
    overshoot_angle_init_.set_max_delta(3.141f);
    overshoot_angle_init_.at_target().max_difference = 0.087f;
    overshoot_angle_init_.at_target().max_velocity = 0.00059f;
    overshoot_angle_init_.set_accel_per_difference(0.00032f);
    overshoot_angle_init_.set_wrong_direction_multiplier(4.0f);
    overshoot_angle_init_.set_max_delta_time(10);

    // Create an OvershootImpelInit that represents a percent from 0 ~ 100.
    // It does not wrap around.
    overshoot_percent_init_.set_modular(false);
    overshoot_percent_init_.set_range(Range(0.0f, 100.0f));
    overshoot_percent_init_.set_max_velocity(10.0f);
    overshoot_percent_init_.set_max_delta(50.0f);
    overshoot_percent_init_.at_target().max_difference = 0.087f;
    overshoot_percent_init_.at_target().max_velocity = 0.00059f;
    overshoot_percent_init_.set_accel_per_difference(0.00032f);
    overshoot_percent_init_.set_wrong_direction_multiplier(4.0f);
    overshoot_percent_init_.set_max_delta_time(10);

    smooth_angle_init_.set_modular(true);
    smooth_angle_init_.set_range(angle_range);

    smooth_scalar_init_.set_modular(false);
    smooth_scalar_init_.set_range(Range(-100.0f, 100.0f));
  }
  virtual void TearDown() {}

protected:
  void InitImpeller(const ImpelInit& init, float start_value,
                    float start_velocity, float target_value,
                    Impeller1f* impeller) {
    impeller->InitializeWithTarget(
        init, &engine_, impel::CurrentToTarget1f(start_value, start_velocity,
                                                 target_value, 0.0f, 1));
  }

  void InitOvershootImpeller(Impeller1f* impeller) {
    InitImpeller(overshoot_percent_init_, overshoot_percent_init_.Max(),
                 overshoot_percent_init_.max_velocity(),
                 overshoot_percent_init_.Max(), impeller);
  }

  void InitOvershootImpellerArray(Impeller1f* impellers, int len) {
    for (int i = 0; i < len; ++i) {
      InitOvershootImpeller(&impellers[i]);
    }
  }

  ImpelTime TimeToSettle(const Impeller1f& impeller, const Settled1f& settled) {
    ImpelTime time = 0;
    while (time < kMaxTime && !settled.Settled(impeller)) {
      engine_.AdvanceFrame(kTimePerFrame);
      time += kTimePerFrame;
    }
    return time;
  }

  ImpelEngine engine_;
  OvershootImpelInit overshoot_angle_init_;
  OvershootImpelInit overshoot_percent_init_;
  SmoothImpelInit smooth_angle_init_;
  SmoothImpelInit smooth_scalar_init_;
};

// Ensure we wrap around from pi to -pi.
TEST_F(ImpelTests, ModularMovement) {
  Impeller1f impeller;
  InitImpeller(overshoot_angle_init_, kPi, 0.001f, -kPi + 1.0f, &impeller);
  engine_.AdvanceFrame(1);

  // We expect the position to go up from +pi since it has positive velocity.
  // Since +pi is the max of the range, we expect the value to wrap down to -pi.
  EXPECT_LE(impeller.Value(), 0.0f);
}

// Ensure the simulation settles on the target in a reasonable amount of time.
TEST_F(ImpelTests, EventuallySettles) {
  Impeller1f impeller;
  InitImpeller(overshoot_angle_init_, 0.0f,
               overshoot_angle_init_.max_velocity(),
               -kPi + 1.0f, &impeller);
  const ImpelTime time_to_settle = TimeToSettle(
      impeller, overshoot_angle_init_.at_target());

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 700);
}

// Ensure the simulation settles when the target is the max bound in a modular
// type. It will oscillate between the max and min bound a lot.
TEST_F(ImpelTests, SettlesOnMax) {
  Impeller1f impeller;
  InitImpeller(overshoot_angle_init_, kPi, overshoot_angle_init_.max_velocity(),
               kPi, &impeller);
  const ImpelTime time_to_settle = TimeToSettle(
      impeller, overshoot_angle_init_.at_target());

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 500);
}

// Ensure the simulation does not exceed the max bound, on constrants that
// do not wrap around.
TEST_F(ImpelTests, StaysWithinBound) {
  Impeller1f impeller;
  InitOvershootImpeller(&impeller);
  engine_.AdvanceFrame(1);

  // Even though we're at the bound and trying to travel beyond the bound,
  // the simulation should clamp our position to the bound.
  EXPECT_EQ(impeller.Value(), overshoot_percent_init_.Max());
}

// Open up a hole in the data and then call Defragment() to close it.
TEST_F(ImpelTests, Defragment) {
  Impeller1f impellers[4];
  const int len = static_cast<int>(ARRAYSIZE(impellers));
  for (int hole = 0; hole < len; ++hole) {
    InitOvershootImpellerArray(impellers, len);

    // Invalidate impeller at index 'hole'.
    impellers[hole].Invalidate();
    EXPECT_FALSE(impellers[hole].Valid());

    // Defragment() is called at the start of AdvanceFrame.
    engine_.AdvanceFrame(1);
    EXPECT_FALSE(impellers[hole].Valid());

    // Compare the remaining impellers against each other.
    const int compare = hole == 0 ? 1 : 0;
    EXPECT_TRUE(impellers[compare].Valid());
    for (int i = 0; i < len; ++i) {
      if (i == hole || i == compare)
        continue;

      // All the impellers should be valid and have the same values.
      EXPECT_TRUE(impellers[i].Valid());
      EXPECT_EQ(impellers[i].Value(), impellers[compare].Value());
      EXPECT_EQ(impellers[i].Velocity(), impellers[compare].Velocity());
      EXPECT_EQ(impellers[i].TargetValue(), impellers[compare].TargetValue());
    }
  }
}

// Copy a valid impeller. Ensure original impeller gets invalidated.
TEST_F(ImpelTests, CopyConstructor) {
  Impeller1f orig_impeller;
  InitOvershootImpeller(&orig_impeller);
  EXPECT_TRUE(orig_impeller.Valid());
  const float value = orig_impeller.Value();

  Impeller1f new_impeller(orig_impeller);
  EXPECT_FALSE(orig_impeller.Valid());
  EXPECT_TRUE(new_impeller.Valid());
  EXPECT_EQ(new_impeller.Value(), value);
}

// Copy an invalid impeller.
TEST_F(ImpelTests, CopyConstructorInvalid) {
  Impeller1f invalid_impeller;
  EXPECT_FALSE(invalid_impeller.Valid());

  Impeller1f copy_of_invalid(invalid_impeller);
  EXPECT_FALSE(copy_of_invalid.Valid());
}

TEST_F(ImpelTests, AssignmentOperator) {
  Impeller1f orig_impeller;
  InitOvershootImpeller(&orig_impeller);
  EXPECT_TRUE(orig_impeller.Valid());
  const float value = orig_impeller.Value();

  Impeller1f new_impeller;
  new_impeller = orig_impeller;
  EXPECT_FALSE(orig_impeller.Valid());
  EXPECT_TRUE(new_impeller.Valid());
  EXPECT_EQ(new_impeller.Value(), value);
}

TEST_F(ImpelTests, VectorResize) {
  static const int kStartSize = 4;
  std::vector<Impeller1f> impellers(kStartSize);

  // Create the impellers and ensure that they're valid.
  for (int i = 0; i < kStartSize; ++i) {
    InitOvershootImpeller(&impellers[i]);
    EXPECT_TRUE(impellers[i].Valid());
  }

  // Expand the size of 'impellers'. This should force the array to be
  // reallocated and all impellers in the array to be moved.
  const Impeller1f* orig_address = &impellers[0];
  impellers.resize(kStartSize + 1);
  const Impeller1f* new_address = &impellers[0];
  EXPECT_NE(orig_address, new_address);

  // All the move impellers should still be valid.
  for (int i = 0; i < kStartSize; ++i) {
    InitOvershootImpeller(&impellers[i]);
    EXPECT_TRUE(impellers[i].Valid());
  }
}

TEST_F(ImpelTests, SmoothModular) {
  static const float kMargin = 0.1f;
  static const ImpelTime kTime = 10;
  static const float kStart = kPi - kMargin;
  static const float kEnd = -kPi + kMargin;
  Impeller1f angle(smooth_angle_init_, &engine_,
                   impel::CurrentToTarget1f(kStart, 0.0f, kEnd, 0.0f, kTime));

  // The difference should be the short way around, across kPi.
  EXPECT_NEAR(angle.Value(), kStart, kAngleEpsilon);
  EXPECT_NEAR(angle.Difference(), 2.0f * kMargin, kAngleEpsilon);

  // Ensure that we're always near kPi, never near 0. We want to go the
  // short way around.
  for (ImpelTime t = 0; t < kTime; ++t) {
    EXPECT_TRUE(kStart - kAngleEpsilon <= angle.Value() ||
                angle.Value() <= kEnd + kAngleEpsilon);
    engine_.AdvanceFrame(1);
  }
  EXPECT_NEAR(angle.Value(), kEnd, kAngleEpsilon);
}

// Print matrices with columns vertically.
static void PrintMatrix(const char* name, const mat4& m) {
  (void)name; (void)m;
  #if DEBUG_PRINT_MATRICES
  printf("%s\n(%f %f %f %f)\n(%f %f %f %f)\n(%f %f %f %f)\n(%f %f %f %f)\n",
         name, m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13],
         m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
  #endif // DEBUG_PRINT_MATRICES
}

// Create a matrix that performs the transformation specified in 'op_init'.
static mat4 CreateMatrixFromOp(const MatrixOperationInit& op_init) {
  const float v = op_init.initial_value;

  switch (op_init.type) {
    case kRotateAboutX: return mat4::FromRotationMatrix(mat3::RotationX(v));
    case kRotateAboutY: return mat4::FromRotationMatrix(mat3::RotationY(v));
    case kRotateAboutZ: return mat4::FromRotationMatrix(mat3::RotationZ(v));
    case kTranslateX: return mat4::FromTranslationVector(vec3(v, 0.0f, 0.0f));
    case kTranslateY: return mat4::FromTranslationVector(vec3(0.0f, v, 0.0f));
    case kTranslateZ: return mat4::FromTranslationVector(vec3(0.0f, 0.0f, v));
    case kScaleX: return mat4::FromScaleVector(vec3(v, 1.0f, 1.0f));
    case kScaleY: return mat4::FromScaleVector(vec3(1.0f, v, 1.0f));
    case kScaleZ: return mat4::FromScaleVector(vec3(1.0f, 1.0f, v));
    case kScaleUniformly: return mat4::FromScaleVector(vec3(v));
    default:
      assert(false);
      return mat4::Identity();
  }
}

// Return the product of the matrices for each operation in 'matrix_init'.
static mat4 CreateMatrixFromOps(const MatrixImpelInit& matrix_init) {
  const MatrixImpelInit::OpVector& ops = matrix_init.ops();

  mat4 m = mat4::Identity();
  for (size_t i = 0; i < ops.size(); ++i) {
    m *= CreateMatrixFromOp(ops[i]);
  }
  return m;
}

static void ExpectMatricesEqual(const mat4& a, const mat4&b, float epsilon) {
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      EXPECT_NEAR(a(i, j), b(i, j), epsilon);
    }
  }
}

static void TestMatrixImpeller(const MatrixImpelInit& matrix_init,
                               ImpelEngine* engine) {
  ImpellerMatrix4f matrix_impeller(matrix_init, engine);
  engine->AdvanceFrame(kTimePerFrame);
  const mat4 check_matrix = CreateMatrixFromOps(matrix_init);
  const mat4 impel_matrix = matrix_impeller.Value();
  ExpectMatricesEqual(impel_matrix, check_matrix, kMatrixEpsilon);

  // Output matrices for debugging.
  PrintMatrix("impeller", impel_matrix);
  PrintMatrix("check", check_matrix);
}

// Test the matrix operation kTranslateX.
TEST_F(ImpelTests, MatrixTranslateX) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateX, smooth_scalar_init_, 2.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Don't use an impeller to drive the animation. Use a constant value.
TEST_F(ImpelTests, MatrixTranslateXConstValue) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateX, 2.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the matrix operation kRotateAboutX.
TEST_F(ImpelTests, MatrixRotateAboutX) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kRotateAboutX, smooth_angle_init_, kHalfPi);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the matrix operation kRotateAboutY.
TEST_F(ImpelTests, MatrixRotateAboutY) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kRotateAboutY, smooth_angle_init_, kHalfPi / 3.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the matrix operation kRotateAboutZ.
TEST_F(ImpelTests, MatrixRotateAboutZ) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kRotateAboutZ, smooth_angle_init_, -kHalfPi / 1.2f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the matrix operation kScaleX.
TEST_F(ImpelTests, MatrixScaleX) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kScaleX, smooth_scalar_init_, -3.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the series of matrix operations for translating XYZ.
TEST_F(ImpelTests, MatrixTranslateXYZ) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateX, smooth_scalar_init_, 2.0f);
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, -3.0f);
  matrix_init.AddOp(impel::kTranslateZ, smooth_scalar_init_, 0.5f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the series of matrix operations for rotating about X, Y, and Z,
// in turn.
TEST_F(ImpelTests, MatrixRotateAboutXYZ) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kRotateAboutX, smooth_angle_init_, -kHalfPi / 2.0f);
  matrix_init.AddOp(impel::kRotateAboutY, smooth_angle_init_, kHalfPi / 3.0f);
  matrix_init.AddOp(impel::kRotateAboutZ, smooth_angle_init_, kHalfPi / 5.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the series of matrix operations for scaling XYZ non-uniformly.
TEST_F(ImpelTests, MatrixScaleXYZ) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kScaleX, smooth_scalar_init_, -3.0f);
  matrix_init.AddOp(impel::kScaleY, smooth_scalar_init_, 2.2f);
  matrix_init.AddOp(impel::kScaleZ, smooth_scalar_init_, 1.01f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the matrix operation kScaleUniformly.
TEST_F(ImpelTests, MatrixScaleUniformly) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kScaleUniformly, smooth_scalar_init_, 10.1f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the series of matrix operations for translating and rotating.
TEST_F(ImpelTests, MatrixTranslateRotateTranslateBack) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, 1.0f);
  matrix_init.AddOp(impel::kRotateAboutX, smooth_angle_init_, kHalfPi);
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, -1.0f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test the series of matrix operations for translating, rotating, and scaling.
TEST_F(ImpelTests, MatrixTranslateRotateScale) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, 1.0f);
  matrix_init.AddOp(impel::kRotateAboutX, smooth_angle_init_, kHalfPi);
  matrix_init.AddOp(impel::kScaleZ, smooth_scalar_init_, -1.4f);
  TestMatrixImpeller(matrix_init, &engine_);
}

// Test a complex the series of matrix operations.
TEST_F(ImpelTests, MatrixTranslateRotateScaleGoneWild) {
  MatrixImpelInit matrix_init;
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, 1.0f);
  matrix_init.AddOp(impel::kTranslateX, smooth_scalar_init_, -1.6f);
  matrix_init.AddOp(impel::kRotateAboutX, smooth_angle_init_, kHalfPi * 0.1f);
  matrix_init.AddOp(impel::kRotateAboutY, smooth_angle_init_, kHalfPi * 0.33f);
  matrix_init.AddOp(impel::kScaleZ, smooth_scalar_init_, -1.4f);
  matrix_init.AddOp(impel::kRotateAboutY, smooth_angle_init_, -kHalfPi * 0.33f);
  matrix_init.AddOp(impel::kTranslateX, smooth_scalar_init_, -1.2f);
  matrix_init.AddOp(impel::kTranslateY, smooth_scalar_init_, -1.5f);
  matrix_init.AddOp(impel::kTranslateZ, smooth_scalar_init_, -2.2f);
  matrix_init.AddOp(impel::kRotateAboutZ, smooth_angle_init_, -kHalfPi * 0.5f);
  matrix_init.AddOp(impel::kScaleX, smooth_scalar_init_, 2.0f);
  matrix_init.AddOp(impel::kScaleY, smooth_scalar_init_, 4.1f);
  TestMatrixImpeller(matrix_init, &engine_);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
