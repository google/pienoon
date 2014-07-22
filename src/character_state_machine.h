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

namespace fpl {
namespace splat {

class CharacterStateMachineDef;
class CharacterState;

class CharacterStateMachine {
 public:
  // Initializes a state machine with the given state machine definition.
  // This class does not take ownership of the definition.
  CharacterStateMachine(
      const CharacterStateMachineDef* const state_machine_def);

  // Updates the current state of the state machine.
  //
  // conditions is a bitfield of enum values from the Conditions enum defined in
  // the flatbuffer definition.
  void Update(uint16_t conditions);

  // Retrieves the current state of the state machine.
  const CharacterState* current_state() const;

 private:
  const CharacterStateMachineDef* const state_machine_def_;
  const CharacterState* current_state_;
};

// Returns true if the state machine is valid. A valid state machine contains
// a single state for each state id declared in the StateId enum, and in the
// same order.
//
// If the state machine is invalid, returns false and prints an logs an error to
// SDL's error interface.
bool CharacterStateMachineDef_Validate(
    const CharacterStateMachineDef* const state_machine_def);

}  // splat
}  // fpl

#endif  // CHARACTER_STATE_MACHINE_
