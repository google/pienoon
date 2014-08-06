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
#include "game_state.h"
#include "character_state_machine.h"
#include "controller.h"

namespace fpl {
namespace splat {

void GameState::AddCharacter(
    int health, Controller* controller,
    const CharacterStateMachineDef* state_machine_def) {
  characters_.push_back(Character(health, controller, state_machine_def));
}

void GameState::AdvanceFrame() {
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    it->controller()->AdvanceFrame();
  }
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    TransitionInputs transition_inputs;
    transition_inputs.logical_inputs = it->controller()->logical_inputs();
    transition_inputs.animation_time = 0;
    it->state_machine()->Update(transition_inputs);
  }
}

}  // splat
}  // fpl

