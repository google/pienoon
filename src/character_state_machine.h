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

#ifndef CHARACTER_STATE_MACHINE_
#define CHARACTER_STATE_MACHINE_

#include <cstdint>
#include "common.h"

namespace fpl {
namespace pie_noon {

struct CharacterStateMachineDef;
struct CharacterState;
struct Condition;

struct ConditionInputs {
  // A set of bits that the state machine will compare against when following
  // transitions.
  int32_t is_down;
  int32_t went_down;
  int32_t went_up;

  // The elapsed time of the animation.
  int animation_time;

  // The current world time
  WorldTime current_time;
};

class CharacterStateMachine {
 public:
  // Initializes a state machine with the given state machine definition.
  // This class does not take ownership of the definition.
  CharacterStateMachine(
      const CharacterStateMachineDef* const state_machine_def);

  // Resets back to initial conditions. Assumes time is reseting to 0 too.
  void Reset();

  // Updates the current state of the state machine.
  //
  // inputs is a structure containing the game data that can affect whether or
  // not a state transition occurs
  void Update(const ConditionInputs& inputs);

  const CharacterState* current_state() const { return current_state_; }

  void SetCurrentState(int new_stateId, WorldTime state_start_time);

  WorldTime current_state_start_time() const {
    return current_state_start_time_;
  }

 private:
  const CharacterStateMachineDef* state_machine_def_;
  const CharacterState* current_state_;
  WorldTime current_state_start_time_;
};

bool EvaluateCondition(const Condition* condition,
                       const ConditionInputs& inputs);

// Returns true if the state machine is valid. A valid state machine contains
// a single state for each state id declared in the StateId enum, and in the
// same order.
//
// If the state machine is invalid, returns false and prints an logs an error to
// SDL's error interface.
bool CharacterStateMachineDef_Validate(
    const CharacterStateMachineDef* const state_machine_def);

}  // pie_noon
}  // fpl

#endif  // CHARACTER_STATE_MACHINE_
