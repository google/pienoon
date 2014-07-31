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

#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <vector>
#include "character.h"

namespace fpl {

class InputSystem;

namespace splat {

class GameState {
 public:
  // Add a player to the game with the given initial state.
  void AddCharacter(int health, Controller* controller,
                    const CharacterStateMachineDef* state_machine_def);

  // Update all players' controller and state machinel.
  void AdvanceFrame();

  // Return the list of players in the game.
  const std::vector<Character>* characters() const { return &characters_; }

 private:
  std::vector<Character> characters_;
};

}  // splat
}  // fpl

#endif  // GAME_STATE_H_
