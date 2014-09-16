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
#include "impel_processor_overshoot.h"
#include "impel_processor_smooth.h"
#include "mathfu/constants.h"
#include "angle.h"

using fpl::kPi;
using impel::ImpelEngine;
using impel::Impeller1f;
using impel::ImpelTime;
using impel::OvershootImpelInit;
using impel::Settled1f;

class ImpelTests : public ::testing::Test {
protected:
  virtual void SetUp()
  {
    impel::OvershootImpelProcessor::Register();
    impel::SmoothImpelProcessor::Register();

    // Create an OvershootImpelInit with reasonable values.
    overshoot_angle_init_.modular = true;
    overshoot_angle_init_.min = -3.14159265359f;
    overshoot_angle_init_.max = 3.14159265359f;
    overshoot_angle_init_.max_velocity = 0.021f;
    overshoot_angle_init_.max_delta = 3.141f;
    overshoot_angle_init_.at_target.max_difference = 0.087f;
    overshoot_angle_init_.at_target.max_velocity = 0.00059f;
    overshoot_angle_init_.accel_per_difference = 0.00032f;
    overshoot_angle_init_.wrong_direction_multiplier = 4.0f;

    // Create an OvershootImpelInit that represents a percent from 0 ~ 100.
    // It does not wrap around.
    overshoot_percent_init_.modular = false;
    overshoot_percent_init_.min = 0.0f;
    overshoot_percent_init_.max = 100.0f;
    overshoot_percent_init_.max_velocity = 10.0f;
    overshoot_percent_init_.max_delta = 50.0f;
    overshoot_percent_init_.at_target.max_difference = 0.087f;
    overshoot_percent_init_.at_target.max_velocity = 0.00059f;
    overshoot_percent_init_.accel_per_difference = 0.00032f;
    overshoot_percent_init_.wrong_direction_multiplier = 4.0f;
  }
  virtual void TearDown() {}

protected:
  void InitMagnet(const OvershootImpelInit& init, float start_value,
                  float start_velocity, float target_value,
                  Impeller1f* impeller) {
    impeller->Initialize(init, &engine_);
    impeller->SetValue(start_value);
    impeller->SetVelocity(start_velocity);
    impeller->SetTargetValue(target_value);
  }

  ImpelTime TimeToSettle(const Impeller1f& impeller, const Settled1f& settled) {
    static const ImpelTime kTimePerFrame = 10;
    static const ImpelTime kMaxTime = 10000;

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
};

// Ensure we wrap around from pi to -pi.
TEST_F(ImpelTests, ModularMovement) {
  Impeller1f impeller;
  InitMagnet(overshoot_angle_init_, kPi, 0.001f, -kPi + 1.0f, &impeller);
  engine_.AdvanceFrame(1);

  // We expect the position to go up from +pi since it has positive velocity.
  // Since +pi is the max of the range, we expect the value to wrap down to -pi.
  EXPECT_LE(impeller.Value(), 0.0f);
}

// Ensure the simulation settles on the target in a reasonable amount of time.
TEST_F(ImpelTests, EventuallySettles) {
  Impeller1f impeller;
  InitMagnet(overshoot_angle_init_, 0.0f, overshoot_angle_init_.max_velocity,
             -kPi + 1.0f, &impeller);
  const ImpelTime time_to_settle = TimeToSettle(
      impeller, overshoot_angle_init_.at_target);

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 700);
}

// Ensure the simulation settles when the target is the max bound in a modular
// type. It will oscillate between the max and min bound a lot.
TEST_F(ImpelTests, SettlesOnMax) {
  Impeller1f impeller;
  InitMagnet(overshoot_angle_init_, kPi, overshoot_angle_init_.max_velocity,
             kPi, &impeller);
  const ImpelTime time_to_settle = TimeToSettle(
      impeller, overshoot_angle_init_.at_target);

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 500);
}

// Ensure the simulation does not exceed the max bound, on constrants that
// do not wrap around.
TEST_F(ImpelTests, StaysWithinBound) {
  Impeller1f impeller;
  InitMagnet(overshoot_percent_init_, overshoot_percent_init_.max,
             overshoot_percent_init_.max_velocity, overshoot_percent_init_.max,
             &impeller);
  engine_.AdvanceFrame(1);

  // Even though we're at the bound and trying to travel beyond the bound,
  // the simulation should clamp our position to the bound.
  EXPECT_EQ(impeller.Value(), overshoot_percent_init_.max);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
