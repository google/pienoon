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
#include "multiplayer_controller.h"

namespace fpl {
namespace pie_noon {

MultiplayerController::MultiplayerController()
    : Controller(kTypeMultiplayer), aim_at_character_id_(kNoCharacter) {}

void MultiplayerController::Initialize(GameState* gamestate,
                                       const Config* config) {
  gamestate_ = gamestate;
  config_ = config;
  character_id_ = kNoCharacter;
  aim_at_character_id_ = kNoCharacter;
  block_hold_ = 0;
  block_delay_ = 0;
  grow_pie_delay_ = 0;
  throw_pie_delay_ = 0;
}

void MultiplayerController::Reset() {
  throw_pie_delay_ = 0;
  grow_pie_delay_ = 0;
  block_hold_ = 0;
  block_delay_ = 0;
  aim_at_character_id_ = kNoCharacter;
}

void MultiplayerController::AdvanceFrame(WorldTime delta_time) {
  if (character_id_ == kNoCharacter) {
    return;
  }
  ClearAllLogicalInputs();

  // Check to make sure we're valid to be sending input.
  Character* character = gamestate_->characters()[character_id_].get();
  auto character_state = character->State();
  if (character->health() <= 0 || character_state == StateId_KO ||
      character_state == StateId_Joining ||
      character_state == StateId_Jumping ||
      character_state == StateId_HitByPie || character_state == StateId_Won) {
    return;
  }

  // Prepare to block
  if (block_delay_ > 0) {
    block_delay_ -= delta_time;
    if (block_delay_ < 0) block_delay_ = 0;
  } else {
    // if we're blocking, keep blocking.
    if (block_hold_ > 0) {
      block_hold_ -= delta_time;
      // aiming has priority so we do it first, then we block
    }
  }

  if (aim_at_character_id_ != kNoCharacter) {
    // we have a character to aim at, make sure we are aimed there.
    if (character->target() != aim_at_character_id_) {
      fplbase::LogInfo(fplbase::kApplication,
              "MultiplayerController: player %d executing aim at %d",
              character_id_, aim_at_character_id_);
      character->force_target(aim_at_character_id_);
      return;
    }
  }

  if (block_hold_ > 0 && block_delay_ == 0) {
    SetLogicalInputs(LogicalInputs_Deflect, true);
    fplbase::LogInfo(fplbase::kApplication,
                     "MultiplayerController: player %d executing block %d",
                     character_id_, block_hold_);
    return;
  }

  if (throw_pie_delay_ > 0 && character_state != StateId_Throwing) {
    throw_pie_delay_ -= delta_time;
    if (throw_pie_delay_ <= 0) {
      fplbase::LogInfo(fplbase::kApplication,
              "MultiplayerController: player %d executing throw pie",
              character_id_);
      SetLogicalInputs(LogicalInputs_ThrowPie, true);
      throw_pie_delay_ = 0;
      return;
    }
  }

  if (grow_pie_delay_ > 0) {
    grow_pie_delay_ -= delta_time;
    if (grow_pie_delay_ <= 0) {
      grow_pie_delay_ = 0;
      SetLogicalInputs(LogicalInputs_TriggerPieGrowth, true);
    }
  }
}

void MultiplayerController::HoldBlock(WorldTime block_delay,
                                      WorldTime block_hold) {
  fplbase::LogInfo(fplbase::kApplication,
          "MultiplayerController: player %d queue in %d: block %d",
          character_id_, block_delay, block_hold);
  if (block_delay <= 0) block_delay = 1;  // must be > 1 to trigger
  if (block_hold <= 0) block_hold = 1;
  block_delay_ = block_delay;
  block_hold_ = block_hold;
}

void MultiplayerController::AimAtCharacter(CharacterId character_id) {
  fplbase::LogInfo(fplbase::kApplication,
                   "MultiplayerController: player %d queue aim at %d",
          character_id_, character_id);
  if (character_id_ != character_id) aim_at_character_id_ = character_id;
}

void MultiplayerController::ThrowPie(WorldTime throw_delay) {
  fplbase::LogInfo(fplbase::kApplication,
          "MultiplayerController: player %d queue in %d: throw pie",
          character_id_, throw_delay);
  if (throw_delay <= 0) throw_delay = 1;  // must be > 1 to trigger
  throw_pie_delay_ = throw_delay;
}

void MultiplayerController::GrowPie(WorldTime grow_delay) {
  fplbase::LogInfo(fplbase::kApplication,
          "MultiplayerController: player %d queue in %d: grow pie",
          character_id_, grow_delay);
  if (grow_delay <= 0) grow_delay = 1;  // must be > 1 to trigger
  grow_pie_delay_ = grow_delay;
}

const Character& MultiplayerController::GetCharacter() {
  return *gamestate_->characters()[character_id_].get();
}

}  // namespace pie_noon
}  // namespace fpl
