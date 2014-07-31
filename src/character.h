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

#ifndef SPLAT_CHARACTER_H_
#define SPLAT_CHARACTER_H_

#include "character_state_machine.h"
#include "sdl_controller.h"

namespace fpl {
namespace splat {

class Controller;

// The current state of the game player. This class tracks information external
// to the state machine, like player health.
class Character {
 public:
  // Creates a character with the given initial values.
  // The Character does not take ownership of the controller or
  // character_state_machine_def pointers.
  Character(int health,
            Controller* controller,
            const CharacterStateMachineDef* character_state_machine_def);

  int health() const { return health_; }

  void set_health(int health) { health_ = health; }

  const Controller* controller() const { return controller_; }

  Controller* controller() { return controller_; }

  CharacterStateMachine* state_machine() { return &state_machine_; }

  const CharacterStateMachine* state_machine() const { return &state_machine_; }

 private:
  // The character's current health. They're out when their health reaches 0.
  int health_;

  // The controller used to translate inputs into game actions.
  Controller* controller_;

  // The current state of the character.
  CharacterStateMachine state_machine_;
};

}  // splat
}  // fpl

#endif  // SPLAT_CHARACTER_H_

