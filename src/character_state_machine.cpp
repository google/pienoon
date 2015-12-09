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

#include "precompiled.h"
#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "timeline_generated.h"

namespace fpl {
namespace pie_noon {

CharacterStateMachine::CharacterStateMachine(
    const CharacterStateMachineDef* const state_machine_def)
    : state_machine_def_(state_machine_def) {
  Reset();
}

void CharacterStateMachine::Reset() {
  current_state_ =
      state_machine_def_->states()->Get(state_machine_def_->initial_state());
  current_state_start_time_ = 0;
}

void CharacterStateMachine::SetCurrentState(int new_stateId,
                                            WorldTime state_start_time) {
  current_state_ = state_machine_def_->states()->Get(new_stateId);
  current_state_start_time_ = state_start_time;
}

bool EvaluateCondition(const Condition* condition,
                       const ConditionInputs& inputs) {
  unsigned int is_down = condition->is_down();
  unsigned int is_up = condition->is_up();
  unsigned int went_down = condition->went_down();
  unsigned int went_up = condition->went_up();

  bool is_game_mode_ok = false;
  if (condition->game_mode() == GameModeCondition_AnyMode) {
    is_game_mode_ok = true;
  } else if (condition->game_mode() == GameModeCondition_SinglePlayerOnly &&
             !inputs.is_multiscreen) {
    is_game_mode_ok = true;
  }
  if (condition->game_mode() == GameModeCondition_MultiPlayerOnly &&
      inputs.is_multiscreen) {
    is_game_mode_ok = true;
  }
  return (inputs.is_down & is_down) == is_down &&
         (~inputs.is_down & is_up) == is_up &&
         (inputs.went_down & went_down) == went_down &&
         (inputs.went_up & went_up) == went_up &&
         inputs.animation_time >= condition->time() &&
         inputs.animation_time < condition->end_time() && is_game_mode_ok;
}

void CharacterStateMachine::Update(const ConditionInputs& inputs) {
  if (!current_state_->transitions()) {
    return;
  }
  for (auto it = current_state_->transitions()->begin();
       it != current_state_->transitions()->end(); ++it) {
    const Condition* condition = it->condition();
    if (condition && EvaluateCondition(condition, inputs)) {
      current_state_ = state_machine_def_->states()->Get(it->target_state());
      current_state_start_time_ = inputs.current_time;
      return;
    }
  }
}

bool CharacterStateMachineDef_Validate(
    const CharacterStateMachineDef* const state_machine_def) {
  if (state_machine_def->states()->Length() != StateId_Count) {
    printf(
        "Must include 1 state for each state id enum. "
        "Found %i states, expected %i.\n",
        state_machine_def->states()->Length(), StateId_Count);
    return false;
  }
  for (uint16_t i = 0; i < state_machine_def->states()->Length(); i++) {
    auto id = state_machine_def->states()->Get(i)->id();
    if (id != i) {
      printf(
          "States must be declared in order. "
          "State #%i was %s, expected %s.\n",
          i, EnumNameStateId(id), EnumNameStateId(static_cast<StateId>(i)));
      return false;
    }
  }
  return true;
}

}  // pie_noon
}  // fpl
