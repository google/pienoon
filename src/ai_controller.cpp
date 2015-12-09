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
#include "ai_controller.h"
#include "common.h"
#include "controller.h"

namespace fpl {
namespace pie_noon {

AiController::AiController() : Controller(kTypeAI) {}

void AiController::Initialize(GameState* gamestate, const Config* config,
                              CharacterId character_id) {
  gamestate_ = gamestate;
  config_ = config;
  character_id_ = character_id;
  time_to_next_action_ = 0;
  block_timer_ = 0;
}

void AiController::AdvanceFrame(WorldTime delta_time) {
  if (character_id_ == kNoCharacter) {
    return;
  }
  ClearAllLogicalInputs();
  time_to_next_action_ -= delta_time;

  // Check to make sure we're valid to be sending input.
  const Character* character = gamestate_->characters()[character_id_].get();
  auto character_state = character->State();
  if (character->health() <= 0 || character_state == StateId_KO ||
      character_state == StateId_Joining ||
      character_state == StateId_Jumping) {
    return;
  }

  // if we're blocking, keep blocking.
  if (block_timer_ > 0) {
    block_timer_ -= delta_time;
    SetLogicalInputs(LogicalInputs_Deflect, true);
    return;
  }

  if (time_to_next_action_ > 0) return;

  time_to_next_action_ = mathfu::RandomInRange<WorldTime>(
      config_->ai_minimum_time_between_actions(),
      config_->ai_maximum_time_between_actions());

  float action = mathfu::Random<float>();
  if (action < config_->ai_chance_to_change_aim()) {
    if (action < config_->ai_chance_to_change_aim() / 2) {
      SetLogicalInputs(LogicalInputs_Left, true);
    } else {
      SetLogicalInputs(LogicalInputs_Right, true);
    }
  }
  action -= config_->ai_chance_to_change_aim();
  if (action >= 0 && action < config_->ai_chance_to_throw()) {
    SetLogicalInputs(LogicalInputs_ThrowPie, true);
  }  // else do nothing.

  if (!gamestate_->is_in_cardboard() && IsInDanger(character_id_) &&
      mathfu::Random<float>() < config_->ai_chance_to_block()) {
    block_timer_ = mathfu::RandomInRange<WorldTime>(
        config_->ai_block_min_duration(), config_->ai_block_max_duration());
    SetLogicalInputs(LogicalInputs_Deflect, true);
  }
}

// Utility function for checking if someone is in danger.
bool AiController::IsInDanger(CharacterId id) const {
  for (size_t i = 0; i < gamestate_->pies().size(); i++) {
    if (gamestate_->pies()[i]->target() == id) {
      return true;
    }
  }
  return false;
}

}  // pie_noon
}  // fpl
