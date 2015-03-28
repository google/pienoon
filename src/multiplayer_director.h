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

#ifndef MULTIPLAYER_DIRECTOR_H_
#define MULTIPLAYER_DIRECTOR_H_

#include <vector>
#include "common.h"
#include "controller.h"
#include "game_state.h"
#include "multiplayer_controller.h"
#include "multiplayer_generated.h"
#include "pie_noon_game.h"

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
#include "gpg_multiplayer.h"
#endif

namespace fpl {
namespace pie_noon {

// MultiplayerDirector is used for the multiscreen version of Pie Noon.
//
// It is responsible for managing the timings of all of the turns, receiving the
// messages from clients saying what actions they wish to perform, and for the
// formatting and sending of multiplayer messages between the host and client.
//
// When you create MultiplayerDirector, you must register multiple
// MultiplayerControllers (one for each player), which is how
// MultiplayerDirector can direct what the characters do each turn.
class MultiplayerDirector {
 public:
  MultiplayerDirector();

  // Give the multiplayer director everything it will need.
  void Initialize(GameState *gamestate_ptr, const Config *config);
#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  // Register a pointer to GPGMultiplayer, so we can send multiplayer messages.
  void RegisterGPGMultiplayer(GPGMultiplayer *gpg_multiplayer) {
    gpg_multiplayer_ = gpg_multiplayer;
  }
#endif
  // Register one MultiplayerController assigned to each player.
  void RegisterController(MultiplayerController *);

  // Call this each frame if multiplayer gameplay is going on.
  void AdvanceFrame(WorldTime delta_time);

  // Start a new multi-screen game.
  void StartGame();

  // End the multi-screen game.
  void EndGame();

  // If testing on PC, pass in your PC keyboard input system to use debug
  // keys for testing turn-based timings.
  void SetDebugInputSystem(InputSystem *input) { debug_input_system_ = input; }

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  // Tell one of your connected players what his player number is.
  void SendPlayerAssignmentMsg(const std::string &instance, CharacterId id);
  // Broadcast start-of-turn to the players.
  void SendStartTurnMsg(uint turn_seconds);
  // Broadcast end-of-game message to the players.
  void SendEndGameMsg();
  // Broadcast player health to the players.
  void SendPlayerStatusMsg();
#endif

  // Takes effect when the next turn starts.
  void set_seconds_per_turn(uint seconds) { seconds_per_turn_ = seconds; }

  uint seconds_per_turn() { return seconds_per_turn_; }

  // First turn is numbered 1, second turn numbered 2, etc.
  // Is 0 before the first turn starts.
  uint turn_number() { return turn_number_; }

  // Tell the multiplayer director about a player's input.
  void InputPlayerCommand(CharacterId id,
                          const multiplayer::PlayerCommand &command);
  // How long until the current turn ends? 0 if outside a turn.
  WorldTime turn_timer() { return turn_timer_; }
  // How long until the next turn starts? 0 if in a turn.
  WorldTime start_turn_timer() { return start_turn_timer_; }

  // Set the number of AI players. The last N players are AIs.
  void set_num_ai_players(uint n) { num_ai_players_ = n; }
  uint num_ai_players() const { return num_ai_players_; }

 private:
  struct Command {
    CharacterId aim_at;
    bool is_firing;
    bool is_blocking;
    Command() : aim_at(-1), is_firing(false), is_blocking(false) {}
  };

  void TriggerStartOfTurn();
  void TriggerEndOfTurn();
  uint CalculateSecondsPerTurn(uint turn_number);

  // Get all the players' healths so we can send them in an update
  std::vector<uint8_t> ReadPlayerHealth();

  // Tell the multiplayer director to choose AI commands for this player.
  void ChooseAICommand(CharacterId id);

  void DebugInput(InputSystem *input);

  GameState *gamestate_;  // Pointer to the gamestate object
  const Config *config_;  // Pointer to the config structure

  std::vector<MultiplayerController *> controllers_;
  // How long the current turn lasts.
  WorldTime turn_timer_;
  // In how long to start the next turn.
  WorldTime start_turn_timer_;
  uint seconds_per_turn_;
  uint turn_number_;
  // Grace period to allow for network travel time, in milliseonds.
  uint network_grace_milliseconds_;

  uint num_ai_players_;  // the last N players are AI.

  InputSystem *debug_input_system_;

  std::vector<Command> commands_;

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  GPGMultiplayer *gpg_multiplayer_ = nullptr;
#endif
};

}  // pie_noon
}  // fpl

#endif  // MULTIPLAYER_DIRECTOR_H_
