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
#include "magnet1f.h"
#include "mathfu/constants.h"
#include "angle.h"

using fpl::kPi;
using fpl::MagnetConstraints1f;
using fpl::OvershootMagnet1fDef;
using fpl::OvershootMagnet1f;
using fpl::MagnetState1f;
using fpl::Magnet1f;
using fpl::WorldTime;

class MagnetTests : public ::testing::Test {
protected:
  virtual void SetUp()
  {
    // Create an OvershootMagnet1fDef class with reasonable values.
    // The data is held in the flat_buffer_builder.
    flatbuffers::FlatBufferBuilder* fbb = &flat_buffer_builders_[0];
    const auto twitch_settled = fpl::CreateMagnetSettled1f(
        *fbb, 0.026f, 0.00104f);
    const auto snap_settled = fpl::CreateMagnetSettled1f(
        *fbb, 0.087f, 0.00059f);
    const auto overshoot_def = fpl::CreateOvershootMagnet1fDef(
        *fbb, 0.00032f, 4.0f, 0.0052f, twitch_settled, snap_settled);
    fbb->Finish(overshoot_def);
    overshoot_def_ = flatbuffers::GetRoot<OvershootMagnet1fDef>(
        fbb->GetBufferPointer());

    // Create a MagnetConstraints1f that represent an angle that wraps around
    // from -pi to pi.
    fbb = &flat_buffer_builders_[1];
    const auto angle_unit = fbb->CreateString("radians");
    const auto angle_constraints = fpl::CreateMagnetConstraints1f(
      *fbb, true, -kPi, kPi, 0.021f, 3.141f, angle_unit);
    fbb->Finish(angle_constraints);
    angle_constraints_ = flatbuffers::GetRoot<MagnetConstraints1f>(
        fbb->GetBufferPointer());

    // Create a MagnetConstraints1f that represents a percent from 0 ~ 100.
    // It does not wrap around.
    fbb = &flat_buffer_builders_[1];
    const auto percent_unit = fbb->CreateString("percent");
    const auto percent_constraints = fpl::CreateMagnetConstraints1f(
      *fbb, false, 0.0f, 100.0f, 10.f, 50.0f, percent_unit);
    fbb->Finish(percent_constraints);
    percent_constraints_ = flatbuffers::GetRoot<MagnetConstraints1f>(
        fbb->GetBufferPointer());
  }
  virtual void TearDown() {}

protected:
  OvershootMagnet1f CreateOvershootMagnet(
      const MagnetConstraints1f& constraints, float start_position,
      float start_velocity, float target_position) {
    const MagnetState1f start_state(start_position, start_velocity);
    OvershootMagnet1f magnet;
    magnet.Initialize(constraints, *overshoot_def_, start_state);
    magnet.SetTargetPosition(target_position);
    return magnet;
  }

  flatbuffers::FlatBufferBuilder flat_buffer_builders_[2];
  const MagnetConstraints1f* angle_constraints_;
  const MagnetConstraints1f* percent_constraints_;
  const OvershootMagnet1fDef* overshoot_def_;
};

// Ensure we wrap around from pi to -pi.
TEST_F(MagnetTests, ModularMovement) {
  const MagnetState1f start_state(kPi, 0.001f);
  OvershootMagnet1f magnet;
  magnet.Initialize(*angle_constraints_, *overshoot_def_, start_state);
  magnet.SetTargetPosition(-kPi + 1.0f);
  magnet.AdvanceFrame(1);

  // We expect the position to go up from +pi since it has positive velocity.
  // Since +pi is the max of the range, we expect the value to wrap down to -pi.
  EXPECT_LE(magnet.Position(), 0.0f);
}

// Iterate calls to magnet.AdvanceFrame() until the magnet settles on its
// target.
static WorldTime TimeToSettle(Magnet1f& magnet) {
  static const WorldTime kTimePerFrame = 10;
  static const WorldTime kMaxTime = 10000;

  WorldTime time = 0;
  while (time < kMaxTime && !magnet.Settled()) {
    magnet.AdvanceFrame(kTimePerFrame);
    time += kTimePerFrame;
  }
  return time;
}

// Ensure the simulation settles on the target in a reasonable amount of time.
TEST_F(MagnetTests, EventuallySettles) {
  OvershootMagnet1f magnet(CreateOvershootMagnet(
      *angle_constraints_, 0.0f, angle_constraints_->max_velocity(),
      -kPi + 1.0f));
  const WorldTime time_to_settle = TimeToSettle(magnet);

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 700);
}

// Ensure the simulation settles when the target is the max bound in a modular
// type. It will oscillate between the max and min bound a lot.
TEST_F(MagnetTests, SettlesOnMax) {
  OvershootMagnet1f magnet(CreateOvershootMagnet(
      *angle_constraints_, kPi, angle_constraints_->max_velocity(), kPi));
  const WorldTime time_to_settle = TimeToSettle(magnet);

  // The simulation should complete in about half a second (time is in ms).
  // Checke that it doesn't finish too quickly nor too slowly.
  EXPECT_GT(time_to_settle, 0);
  EXPECT_LT(time_to_settle, 500);
}

// Ensure the simulation does not exceed the max bound, on constrants that
// do not wrap around.
TEST_F(MagnetTests, StaysWithinBound) {
  OvershootMagnet1f magnet(CreateOvershootMagnet(
      *percent_constraints_, percent_constraints_->max(),
      percent_constraints_->max_velocity(), percent_constraints_->max()));
  magnet.AdvanceFrame(1);

  // Even though we're at the bound and trying to travel beyond the bound,
  // the simulation should clamp our position to the bound.
  EXPECT_EQ(magnet.Position(), percent_constraints_->max());
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
