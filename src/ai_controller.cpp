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
#include "common.h"
#include "ai_controller.h"
#include "controller.h"
#include "utilities.h"

namespace fpl {
namespace splat {

AiController::AiController() : Controller(kTypeAi) {}


void AiController::Initialize(GameState* gamestate,
                              const Config* config,
                              CharacterId character_id) {
  gamestate_ = gamestate;
  config_ = config;
  character_id_ = character_id;
  time_to_next_action_ = 0;
}

// Helper function for picking a random number in a range.
// TODO(ccornell) Put this in Mathfu!
static int RandIntInRange(int a, int b) {
  int delta = b - a;
  return rand() % delta + a;
}

void AiController::AdvanceFrame(WorldTime delta_time) {
  ClearAllLogicalInputs();
  time_to_next_action_ -= delta_time;
  if (time_to_next_action_ > 0) {
    return;
  }

  if (gamestate_->characters()[character_id_]->health() <= 0) {
      return;
  }

  time_to_next_action_ +=
      RandIntInRange(config_->ai_minimum_time_between_actions(),
                     config_->ai_maximum_time_between_actions());

  float action = mathfu::Random<float>();
  if (action < config_->ai_chance_to_change_aim()) {
      if (action < config_->ai_chance_to_change_aim() / 2) {
        SetLogicalInputs(LogicalInputs_Left, true);
      }
      else {
        SetLogicalInputs(LogicalInputs_Right, true);
      }
  }
  action -= config_->ai_chance_to_change_aim();
  if (action >= 0 && action < config_->ai_chance_to_throw()) {
      SetLogicalInputs(LogicalInputs_ThrowPie, true);
  } // else do nothing.

  if (IsInDanger(character_id_) &&
      mathfu::Random<float>() < config_->ai_chance_to_block()) {
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

}  // splat
}  // fpl

