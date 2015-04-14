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

#ifndef COMPONENTS_CARDBOARD_PLAYER_H_
#define COMPONENTS_CARDBOARD_PLAYER_H_

#include "common.h"
#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "scene_description.h"

namespace fpl {
namespace pie_noon {

class GameState;

const int kMaxHealthAccessories = 3;

struct CardboardPlayerData {
  entity::EntityRef target_reticle;
  entity::EntityRef loaded_pie;
  entity::EntityRef health[kMaxHealthAccessories];
  CharacterId character_id;
};

class CardboardPlayerComponent : public entity::Component<CardboardPlayerData> {
 public:
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);

  void set_gamestate_ptr(GameState* gamestate_ptr) {
    gamestate_ptr_ = gamestate_ptr;
  }
  void set_config(const Config* config) { config_ = config; }

 private:
  void UpdateTargetReticle(entity::EntityRef entity);
  void UpdateLoadedPie(entity::EntityRef entity);
  void UpdateHealthAccessories(entity::EntityRef entity);
  const Config* config_;
  GameState* gamestate_ptr_;
};

}  // pie_noon
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(
    fpl::pie_noon::CardboardPlayerComponent, fpl::pie_noon::CardboardPlayerData,
    fpl::pie_noon::ComponentDataUnion_CardboardPlayerDef)

#endif  // COMPONENTS_CARDBOARD_PLAYER_H_
