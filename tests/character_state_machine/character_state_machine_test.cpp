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

#include <string>
#include "character_state_machine.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"
#include "gtest/gtest.h"

namespace sp = ::fpl::splat;
namespace fb = ::flatbuffers;

TEST(CharacterStateMachineTests, NotAllStatesUsedDeathTest) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<sp::CharacterState>> states;
  // Omit the final state
  for (int i = 0; i < sp::StateId_Count - 1; i++) {
    auto trans = builder.CreateVector<fb::Offset<sp::Transition>>(nullptr, 0);
    auto timeline = fpl::CreateTimeline(builder);
    states.push_back(sp::CreateCharacterState(builder, i, trans, timeline));
  }
  auto state_machine_offset = sp::CreateCharacterStateMachineDef(builder,
      builder.CreateVector<fb::Offset<sp::CharacterState>>(
          &states.front(), states.size()), 0);
  builder.Finish(state_machine_offset);
  auto state_machine = sp::GetCharacterStateMachineDef(
      builder.GetBufferPointer());

  ASSERT_FALSE(CharacterStateMachineDef_Validate(state_machine));
}

TEST(CharacterStateMachineTests, StatesOutOfOrderDeathTest) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<sp::CharacterState>> states;
  for (int i = 0; i < sp::StateId_Count; i++) {
    auto trans = builder.CreateVector<fb::Offset<sp::Transition>>(nullptr, 0);
    auto timeline = fpl::CreateTimeline(builder);
    // Set all state id's to 0
    states.push_back(sp::CreateCharacterState(builder, 0, trans, timeline));
  }
  auto state_machine_offset = sp::CreateCharacterStateMachineDef(builder,
      builder.CreateVector<fb::Offset<sp::CharacterState>>(
          &states.front(), states.size()), 0);
  builder.Finish(state_machine_offset);
  auto state_machine = sp::GetCharacterStateMachineDef(
      builder.GetBufferPointer());

  ASSERT_FALSE(CharacterStateMachineDef_Validate(state_machine));
}

TEST(CharacterStateMachineTests, AllStatesPass) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<sp::CharacterState>> states;
  for (int i = 0; i < sp::StateId_Count; i++) {
    auto trans = builder.CreateVector<fb::Offset<sp::Transition>>(nullptr, 0);
    auto timeline = fpl::CreateTimeline(builder);
    states.push_back(sp::CreateCharacterState(builder, i, trans, timeline));
  }
  auto state_machine_offset = sp::CreateCharacterStateMachineDef(builder,
      builder.CreateVector<fb::Offset<sp::CharacterState>>(
          &states.front(), states.size()), 0);
  builder.Finish(state_machine_offset);
  auto def = sp::GetCharacterStateMachineDef(builder.GetBufferPointer());

  CharacterStateMachineDef_Validate(def);
}

TEST(CharacterStateMachineTests, FollowTransitions) {
  flatbuffers::FlatBufferBuilder builder;
  std::vector<flatbuffers::Offset<sp::CharacterState>> states;
  for (int i = 0; i < sp::StateId_Count; i++) {
    std::vector<flatbuffers::Offset<sp::Transition>> trans_vec;
    uint8_t target_id = (i + 1) % sp::StateId_Count;
    uint16_t conditions = sp::LogicalInputs_ThrowPie;
    trans_vec.push_back(sp::CreateTransition(builder, target_id, conditions));

    auto trans = builder.CreateVector<fb::Offset<sp::Transition>>(
      &trans_vec.front(), trans_vec.size());
    auto timeline = fpl::CreateTimeline(builder);
    states.push_back(sp::CreateCharacterState(builder, i, trans, timeline));
  }
  auto state_machine_offset = sp::CreateCharacterStateMachineDef(builder,
      builder.CreateVector<fb::Offset<sp::CharacterState>>(
          &states.front(), states.size()), 0);
  builder.Finish(state_machine_offset);
  auto def = sp::GetCharacterStateMachineDef(builder.GetBufferPointer());

  CharacterStateMachineDef_Validate(def);

  sp::CharacterStateMachine state_machine(def);
  sp::ConditionInputs correct_input1;
  correct_input1.is_down = sp::LogicalInputs_ThrowPie;

  sp::ConditionInputs correct_input2;
  correct_input2.is_down =
      sp::LogicalInputs_ThrowPie | sp::LogicalInputs_Deflect;

  sp::ConditionInputs incorrect_input;
  incorrect_input.is_down = sp::LogicalInputs_Deflect;

  ASSERT_EQ(state_machine.current_state()->id(), 0);
  state_machine.Update(correct_input1);
  ASSERT_EQ(state_machine.current_state()->id(), 1);
  state_machine.Update(correct_input2);
  ASSERT_EQ(state_machine.current_state()->id(), 2);
  state_machine.Update(incorrect_input);
  ASSERT_EQ(state_machine.current_state()->id(), 2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
