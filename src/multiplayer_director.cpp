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

#include "precompiled.h"
#include "common.h"
#include "controller.h"
#include "multiplayer_director.h"
#include "pie_noon_game.h"

namespace fpl {
namespace pie_noon {

MultiplayerDirector::MultiplayerDirector()
    : turn_timer_(0), debug_input_system_(nullptr) {}

void MultiplayerDirector::Initialize(GameState* gamestate,
                                     const Config* config) {
  gamestate_ = gamestate;
  config_ = config;
  turn_timer_ = 0;
  start_turn_timer_ = 0;
  seconds_per_turn_ =
      config_->multiscreen_options()->turn_length()->Get(0)->turn_seconds();
  turn_number_ = 0;
  num_ai_players_ = 0;
  game_running_ = false;
}

void MultiplayerDirector::RegisterController(
    MultiplayerController* controller) {
  controllers_.push_back(controller);
  commands_.push_back(Command());
  character_splats_.push_back(0);
}

void MultiplayerDirector::StartGame() {
  game_running_ = true;
  turn_number_ = 0;
  turn_timer_ = 0;
  start_turn_timer_ =
      config_->multiscreen_options()->first_turn_delay_milliseconds();
  for (unsigned int i = 0; i < commands_.size(); i++) {
    commands_[i].aim_at = (i + 1) % commands_.size();
    commands_[i].is_firing = false;
    commands_[i].is_blocking = false;
  }
  for (unsigned int i = 0; i < controllers_.size(); i++) {
    controllers_[i]->Reset();
  }
  for (unsigned int i = 0; i < character_splats_.size(); i++) {
    character_splats_[i] = 0;
  }
}

void MultiplayerDirector::EndGame() {
  game_running_ = false;
  turn_timer_ = 0;
}

void MultiplayerDirector::AdvanceFrame(WorldTime delta_time) {
  if (debug_input_system_ != nullptr) {
    DebugInput(debug_input_system_);
  }

  if (start_turn_timer_ > 0) {
    start_turn_timer_ -= delta_time;
    if (start_turn_timer_ <= 0) {
      TriggerStartOfTurn();
    }
  }

  if (turn_timer_ > 0) {
    turn_timer_ -= delta_time;
    if (turn_timer_ <= 0) {
      TriggerEndOfTurn();
    }
  }
}

void MultiplayerDirector::TriggerEndOfTurn() {
  LogInfo(kApplication, "MultiplayerDirector: END TURN");
  // if we have any AI players, set their commands now
  if (config_->multiscreen_options()->ai_enabled()) {
    for (unsigned int i = 0; i < num_ai_players(); i++) {
      CharacterId id =
          static_cast<CharacterId>(commands_.size() - num_ai_players() + i);
      if (id >= 0 && id < static_cast<int>(commands_.size())) {
        ChooseAICommand(id);
      }
    }
  }

  for (int i = 0; i < static_cast<int>(controllers_.size()); i++) {
    int character_delay =
        i * config_->multiscreen_options()->char_delay_milliseconds();
    int pie_throw_delay =
        config_->multiscreen_options()->pie_delay_milliseconds();
    int blocking_delay =
        config_->multiscreen_options()->block_delay_milliseconds();
    int blocking_hold =
        config_->multiscreen_options()->block_hold_milliseconds();
    int pie_grow_delay =
        config_->multiscreen_options()->grow_delay_milliseconds();

    if (commands_[i].aim_at != kNoCharacter) {
      controllers_[i]->AimAtCharacter(commands_[i].aim_at);
    }
    if (commands_[i].is_firing) {
      controllers_[i]->ThrowPie(pie_throw_delay + character_delay);
    } else if (commands_[i].is_blocking) {
      controllers_[i]->HoldBlock(blocking_delay + character_delay,
                                 blocking_hold);
    } else {
      controllers_[i]->GrowPie(pie_grow_delay + character_delay);
    }
  }

  for (unsigned int i = 0; i < character_splats_.size(); i++) {
    character_splats_[i] = 0;  // splats only last one turn
  }

  turn_timer_ = 0;
  if (debug_input_system_ == nullptr) {
    // Schedule the next turn to start soon
    start_turn_timer_ =
        config_->multiscreen_options()->start_turn_delay_milliseconds();
  }
}

unsigned int MultiplayerDirector::CalculateSecondsPerTurn(
    unsigned int turn_number) {
  for (auto turn_spec : *config_->multiscreen_options()->turn_length()) {
    if (turn_spec->until_turn_number() == -1 ||
        turn_number <=
            static_cast<unsigned int>(turn_spec->until_turn_number()))
      return turn_spec->turn_seconds();
  }
  // By default just return the first turn length
  return config_->multiscreen_options()->turn_length()->Get(0)->turn_seconds();
}

void MultiplayerDirector::TriggerStartOfTurn() {
  start_turn_timer_ = 0;
  turn_number_++;
  set_seconds_per_turn(CalculateSecondsPerTurn(turn_number_));
  turn_timer_ = seconds_per_turn() * kMillisecondsPerSecond +
                config_->multiscreen_options()->network_grace_milliseconds();
#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  SendStartTurnMsg(seconds_per_turn());
#endif
}

void MultiplayerDirector::TriggerPlayerHitByPie(CharacterId player,
                                                int damage) {
  if (!game_running_) return;
  LogInfo(kApplication, "MultiplayerDirector: %d hit for %d", player, damage);
  int num_splats = 0;
  if (damage >=
      config_->multiscreen_options()->heavy_splat_damage_threshold()) {
    // heavy splat, splat lots of buttons
    num_splats += config_->multiscreen_options()->heavy_splat_num_buttons();
  } else if (damage >=
             config_->multiscreen_options()->light_splat_damage_threshold()) {
    // light splat, splat fewer buttons
    num_splats += config_->multiscreen_options()->light_splat_num_buttons();
  } else {
    // no splat
  }
  // Go through and try to find num_splats new buttons to splat.
  std::vector<int> splats_available;
  for (unsigned int i = 0; i < controllers_.size(); i++) {
    unsigned int splat_mask = (1 << i);
    if ((character_splats_[player] & splat_mask) == 0) {
      splats_available.push_back(i);
    }
  }
  while (num_splats > 0 && splats_available.size() > 0) {
    unsigned int idx = mathfu::RandomInRange<int>(
        0, static_cast<int>(splats_available.size()));
    unsigned int splat_used = splats_available[idx];
    unsigned int splat_mask = (1 << splat_used);
    character_splats_[player] |= splat_mask;
    num_splats--;
    splats_available.erase(splats_available.begin() + idx);
  }
#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  SendPlayerStatusMsg();  // sent unreliably since we may send a bunch in a row
#endif
}

bool MultiplayerDirector::IsAIPlayer(CharacterId player) {
  return (static_cast<unsigned int>(player) >=
          controllers_.size() - num_ai_players());
}

void MultiplayerDirector::InputPlayerCommand(
    CharacterId id, const multiplayer::PlayerCommand& player_command) {
  Command command;
  if (player_command.aim_at() >= 0) {
    command.aim_at = (CharacterId)player_command.aim_at();
  } else {
    command.aim_at = kNoCharacter;
  }
  command.is_firing = player_command.is_firing() != 0;
  command.is_blocking = player_command.is_blocking() != 0;
  commands_[id] = command;
}

void MultiplayerDirector::ChooseAICommand(CharacterId id) {
  // If we are dead, don't do anything.
  if (controllers_[id]->GetCharacter().health() <= 0) return;

  Command command = commands_[id];  // Get previous command.
  const auto* options = config_->multiscreen_options();

  float action = mathfu::Random<float>();
  if (action < options->ai_chance_to_throw()) {
    LogInfo(kApplication, "MultiplayerDirector: AI %d setting action to throw",
            id);
    command.is_firing = true;
    command.is_blocking = false;
  }
  action -= options->ai_chance_to_throw();
  if (action >= 0 && action < options->ai_chance_to_block()) {
    LogInfo(kApplication, "MultiplayerDirector: AI %d setting action to block",
            id);
    command.is_firing = false;
    command.is_blocking = true;
  }
  action -= options->ai_chance_to_block();
  if (action >= 0 && action < options->ai_chance_to_wait()) {
    LogInfo(kApplication, "MultiplayerDirector: AI %d setting action to wait",
            id);
    command.is_firing = false;
    command.is_blocking = false;
  }
  action -= options->ai_chance_to_wait();
  // If action is still > 0, command has the action from the previous turn,
  // don't change it.

  unsigned int self = static_cast<unsigned int>(id);  // for comparison
  std::vector<unsigned int> candidate_targets;
  // Choose how to target opponents.
  float target = mathfu::Random<float>();
  if (target < options->ai_chance_to_target_largest_pie()) {
    // First get the max pie damage. Then put everyone with that pie damage
    // into the candidate targets list.
    LogInfo(kApplication, "MultiplayerDirector: AI %d targeting largest pie",
            id);

    int max_pie = -1;
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.pie_damage() > max_pie) {
        max_pie = enemy.pie_damage();
      }
    }
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.pie_damage() == max_pie) {
        candidate_targets.push_back(i);
      }
    }
  }
  target -= options->ai_chance_to_target_largest_pie();
  if (target >= 0 && target < options->ai_chance_to_target_lowest_health()) {
    // First get the lowest enemy health. Then put everyone with that health
    // into the candidate targets list.
    LogInfo(kApplication, "MultiplayerDirector: AI %d targeting lowest health",
            id);
    int min_health = config_->character_health() + 1;
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.health() < min_health) {
        min_health = enemy.health();
      }
    }
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.health() == min_health) {
        candidate_targets.push_back(i);
      }
    }
  }
  target -= options->ai_chance_to_target_lowest_health();
  if (target >= 0 && target < options->ai_chance_to_target_highest_health()) {
    // First get the highest enemy health. Then put everyone with that health
    // into the candidate targets list.
    LogInfo(kApplication, "MultiplayerDirector: AI %d targeting highest health",
            id);
    int max_health = -1;
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.health() > max_health) {
        max_health = enemy.health();
      }
    }
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      if (enemy.health() == max_health) {
        candidate_targets.push_back(i);
      }
    }
  }
  target -= options->ai_chance_to_target_highest_health();
  if (target >= 0 && target < options->ai_chance_to_target_random()) {
    LogInfo(kApplication, "MultiplayerDirector: AI %d targeting randomly", id);
    // Just put all living enemies in the list.
    for (unsigned int i = 0; i < controllers_.size(); i++) {
      const Character& enemy = controllers_[i]->GetCharacter();
      if (i == self || enemy.health() <= 0) continue;  // ignore self/dead enemy
      candidate_targets.push_back(i);
    }
  }
  target -= options->ai_chance_to_target_random();
  // If target is still > 0, command has the action from the previous turn,
  // don't change it.

  if (candidate_targets.size() > 0) {
    int which = mathfu::RandomInRange<int>(
        0, static_cast<int>(candidate_targets.size()));
    command.aim_at = candidate_targets[which];
  }
  // If we have no candidate targets, we won't change aim at all.

  // Find a random
  commands_[id] = command;
}

void MultiplayerDirector::DebugInput(InputSystem* input) {
  // debug keys: 3 players to aim at, 2 buttons for fire or block
  // player 0: 1 2 3 to aim, 4 5 6 to fire or block or idle
  // player 1: Q W E to aim, R T Y to fire or block or idle
  // player 2: A S D to aim, F G H to fire or block or idle
  // player 3: Z X C to aim, V B N to fire or block or idle

  // Player 0
  if (input->GetButton(FPLK_1).went_down()) {
    LogInfo(kApplication, "MP: Key 1: Player 0 AimAt 1");
    commands_[0].aim_at = 1;
  }
  if (input->GetButton(FPLK_2).went_down()) {
    LogInfo(kApplication, "MP: Key 2: Player 0 AimAt 2");
    commands_[0].aim_at = 2;
  }
  if (input->GetButton(FPLK_3).went_down()) {
    LogInfo(kApplication, "MP: Key 3: Player 0 AimAt 3");
    commands_[0].aim_at = 3;
  }
  if (input->GetButton(FPLK_4).went_down()) {
    LogInfo(kApplication, "MP: Key 4: Player 0 Fire");
    commands_[0].is_firing = true;
    commands_[0].is_blocking = false;
  }
  if (input->GetButton(FPLK_5).went_down()) {
    LogInfo(kApplication, "MP: Key 5: Player 0 Block");
    commands_[0].is_blocking = true;
    commands_[0].is_firing = false;
  }
  if (input->GetButton(FPLK_6).went_down()) {
    LogInfo(kApplication, "MP: Key 6: Player 0 Wait");
    commands_[0].is_blocking = false;
    commands_[0].is_firing = false;
  }

  // Player 1
  if (input->GetButton(FPLK_q).went_down()) {
    LogInfo(kApplication, "MP: Key Q: Player 1 AimAt 0");
    commands_[1].aim_at = 0;
  }
  if (input->GetButton(FPLK_w).went_down()) {
    LogInfo(kApplication, "MP: Key W: Player 1 AimAt 2");
    commands_[1].aim_at = 2;
  }
  if (input->GetButton(FPLK_e).went_down()) {
    LogInfo(kApplication, "MP: Key E: Player 1 AimAt 3");
    commands_[1].aim_at = 3;
  }
  if (input->GetButton(FPLK_r).went_down()) {
    LogInfo(kApplication, "MP: Key R: Player 1 Fire");
    commands_[1].is_firing = true;
    commands_[1].is_blocking = false;
  }
  if (input->GetButton(FPLK_t).went_down()) {
    LogInfo(kApplication, "MP: Key T: Player 1 Block");
    commands_[1].is_blocking = true;
    commands_[1].is_firing = false;
  }
  if (input->GetButton(FPLK_y).went_down()) {
    LogInfo(kApplication, "MP: Key Y: Player 1 Wait");
    commands_[1].is_blocking = false;
    commands_[1].is_firing = false;
  }

  // Player 2
  if (input->GetButton(FPLK_a).went_down()) {
    LogInfo(kApplication, "MP: Key A: Player 2 AimAt 0");
    commands_[2].aim_at = 0;
  }
  if (input->GetButton(FPLK_s).went_down()) {
    LogInfo(kApplication, "MP: Key S: Player 2 AimAt 1");
    commands_[2].aim_at = 1;
  }
  if (input->GetButton(FPLK_d).went_down()) {
    LogInfo(kApplication, "MP: Key D: Player 2 AimAt 3");
    commands_[2].aim_at = 3;
  }
  if (input->GetButton(FPLK_f).went_down()) {
    LogInfo(kApplication, "MP: Key F: Player 2 Fire");
    commands_[2].is_firing = true;
    commands_[2].is_blocking = false;
  }
  if (input->GetButton(FPLK_g).went_down()) {
    LogInfo(kApplication, "MP: Key G: Player 2 Block");
    commands_[2].is_blocking = true;
    commands_[2].is_firing = false;
  }
  if (input->GetButton(FPLK_h).went_down()) {
    LogInfo(kApplication, "MP: Key H: Player 2 Wait");
    commands_[2].is_blocking = false;
    commands_[2].is_firing = false;
  }

  // Player 3
  if (input->GetButton(FPLK_z).went_down()) {
    LogInfo(kApplication, "MP: Key Z: Player 3 AimAt 0");
    commands_[3].aim_at = 0;
  }
  if (input->GetButton(FPLK_x).went_down()) {
    LogInfo(kApplication, "MP: Key X: Player 3 AimAt 1");
    commands_[3].aim_at = 1;
  }
  if (input->GetButton(FPLK_c).went_down()) {
    LogInfo(kApplication, "MP: Key C: Player 3 AimAt 2");
    commands_[3].aim_at = 2;
  }
  if (input->GetButton(FPLK_v).went_down()) {
    LogInfo(kApplication, "MP: Key V: Player 3 Fire");
    commands_[3].is_firing = true;
    commands_[3].is_blocking = false;
  }
  if (input->GetButton(FPLK_b).went_down()) {
    LogInfo(kApplication, "MP: Key B: Player 3 Block");
    commands_[3].is_blocking = true;
    commands_[3].is_firing = false;
  }
  if (input->GetButton(FPLK_n).went_down()) {
    LogInfo(kApplication, "MP: Key N: Player 3 Wait");
    commands_[3].is_blocking = false;
    commands_[3].is_firing = false;
  }

  // Enter triggers end of turn manually.
  if (input->GetButton(FPLK_RETURN).went_down()) {
    LogInfo(kApplication, "MP: Enter: Trigger EndOfTurn");
    turn_timer_ = 1;
  }
}

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
void MultiplayerDirector::SendPlayerAssignmentMsg(const std::string& instance,
                                                  CharacterId id) {
  flatbuffers::FlatBufferBuilder builder;
  auto message_root = multiplayer::CreateMessageRoot(
      builder, multiplayer::Data_PlayerAssignment,
      multiplayer::CreatePlayerAssignment(builder, id).Union());
  builder.Finish(message_root);

  std::vector<uint8_t> message(builder.GetBufferPointer(),
                               builder.GetBufferPointer() + builder.GetSize());
  gpg_multiplayer_->SendMessage(instance, message, true);
}

void MultiplayerDirector::SendStartTurnMsg(unsigned int seconds) {
  // read the player healths
  std::vector<uint8_t> health_vec = ReadPlayerHealth();
  std::vector<uint8_t> splats_vec = ReadPlayerSplats();

  flatbuffers::FlatBufferBuilder builder;
  auto health = builder.CreateVector(health_vec);
  auto splats = builder.CreateVector(splats_vec);
  auto player_status = multiplayer::CreatePlayerStatus(builder, health, splats);
  auto message_root = multiplayer::CreateMessageRoot(
      builder, multiplayer::Data_StartTurn,
      multiplayer::CreateStartTurn(builder, (unsigned short)seconds,
                                   player_status)
          .Union());
  builder.Finish(message_root);

  std::vector<uint8_t> message(builder.GetBufferPointer(),
                               builder.GetBufferPointer() + builder.GetSize());
  gpg_multiplayer_->BroadcastMessage(message, true);
}

void MultiplayerDirector::SendEndGameMsg() {
  std::vector<uint8_t> health_vec = ReadPlayerHealth();
  std::vector<uint8_t> splats_vec = ReadPlayerSplats();

  flatbuffers::FlatBufferBuilder builder;
  auto health = builder.CreateVector(health_vec);
  auto splats = builder.CreateVector(splats_vec);
  auto player_status = multiplayer::CreatePlayerStatus(builder, health, splats);
  auto message_root = multiplayer::CreateMessageRoot(
      builder, multiplayer::Data_EndGame,
      multiplayer::CreateEndGame(builder, player_status).Union());
  builder.Finish(message_root);

  std::vector<uint8_t> message(builder.GetBufferPointer(),
                               builder.GetBufferPointer() + builder.GetSize());
  gpg_multiplayer_->BroadcastMessage(message, true);
}

void MultiplayerDirector::SendPlayerStatusMsg() {
  std::vector<uint8_t> health_vec = ReadPlayerHealth();
  std::vector<uint8_t> splats_vec = ReadPlayerSplats();

  flatbuffers::FlatBufferBuilder builder;
  auto health = builder.CreateVector(health_vec);
  auto splats = builder.CreateVector(splats_vec);
  auto message_root = multiplayer::CreateMessageRoot(
      builder, multiplayer::Data_PlayerStatus,
      multiplayer::CreatePlayerStatus(builder, health, splats).Union());
  builder.Finish(message_root);

  std::vector<uint8_t> message(builder.GetBufferPointer(),
                               builder.GetBufferPointer() + builder.GetSize());
  gpg_multiplayer_->BroadcastMessage(message, false);  // Send unreliably.
}

#endif  // PIE_NOON_USES_GOOGLE_PLAY_GAMES

std::vector<uint8_t> MultiplayerDirector::ReadPlayerHealth() {
  std::vector<uint8_t> vec;
  for (auto controller : controllers_) {
    int health = controller->GetCharacter().health();
    vec.push_back((health < 0) ? 0 : (uint8_t)health);
  }
  return vec;
}

std::vector<uint8_t> MultiplayerDirector::ReadPlayerSplats() {
  return character_splats_;
}

}  // namespace pie_noon
}  // namespace fpl
