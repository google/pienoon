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

#include <algorithm>
#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "SDL_log.h"

namespace fpl {
namespace splat {

CharacterStateMachine::CharacterStateMachine(
    const CharacterStateMachineDef* const state_machine_def)
    : state_machine_def_(state_machine_def),
      current_state_(state_machine_def->states()->Get(
          state_machine_def_->initial_state())) {
}

void CharacterStateMachine::Update(uint16_t conditions) {
  for (const auto transition : *current_state_->transitions()) {
    if ((transition->conditions() & conditions) == transition->conditions()) {
      current_state_ = state_machine_def_->states()->Get(
          transition->target_state());
      return;
    }
  }
}

const CharacterState* CharacterStateMachine::current_state() const {
  return current_state_;
}

bool CharacterStateMachineDef_Validate(
    const CharacterStateMachineDef* const state_machine_def) {
  if (state_machine_def->states()->Length() != StateId_Count) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
                    "Must include 1 state for each state id enum. "
                    "Found %i states, expected %i.\n",
                    state_machine_def->states()->Length(), StateId_Count);
    return false;
  }
  for (uint16_t i = 0; i < state_machine_def->states()->Length(); i++) {
    uint16_t id = state_machine_def->states()->Get(i)->id();
    if (id != i) {
      SDL_LogCritical(SDL_LOG_CATEGORY_ERROR,
                      "States must be declared in order. "
                      "State #%i was %s, expected %s.\n",
                      i, EnumNameStateId(id), EnumNameStateId(i));
      return false;
    }
  }
  return true;
}

}  // splat
}  // fpl

