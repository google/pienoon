// Copyright 2015 Google Inc. All rights reserved.
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

#ifndef MULTIPLAYER_CONTROLLER_H_
#define MULTIPLAYER_CONTROLLER_H_

#include <vector>
#include "common.h"
#include "controller.h"
#include "game_state.h"

namespace fpl {
namespace pie_noon {

// The MultiplayerController class is a thin interface between
// MultiplayerDirector and the game state.
//
// MultiplayerDirector tells each MultiplayerController what it wants it to do
// for the given turn, then it's MultiplayerController's job to inject the
// correct button presses with the correct timing to make it happen.
class MultiplayerController : public Controller {
 public:
  MultiplayerController();

  // Give the multiplayer controller everything it will need.
  void Initialize(GameState* gamestate_ptr, const Config* config);

  // Decide what the character is doing this frame.
  virtual void AdvanceFrame(WorldTime delta_time);

  void AimAtCharacter(int character_id);  // Aim towards this character.
  void HoldBlock(WorldTime block_delay,
                 WorldTime block_hold);  // Block for a predetermined time.
  void ThrowPie(WorldTime throw_delay);  // Throw a pie where we are aiming.
  void GrowPie(WorldTime grow_delay);    // Grow a pie to the next level.

  void Reset();  // Reset our action and target back to defaults.

  // Sometimes MultiplayerDirector needs access to our character.
  // This gives it that access.
  const Character& GetCharacter();

 private:
  GameState* gamestate_;             // Pointer to the gamestate object
  const Config* config_;             // Pointer to the config structure
  CharacterId aim_at_character_id_;  // Who to aim at.
  WorldTime block_delay_;            // How long to wait until blocking.
  WorldTime block_hold_;             // How many milliseconds we are blocking.
  WorldTime throw_pie_delay_;        // After this long, throw a pie.
  WorldTime grow_pie_delay_;         // After this long, grow the pie 1 level.
};

}  // pie_noon
}  // fpl

#endif  // MULTIPLAYER_CONTROLLER_H_
