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

#ifndef COMPONENTS_PLAYER_CHARACTER_H_
#define COMPONENTS_PLAYER_CHARACTER_H_

#include "character.h"
#include "common.h"
#include "components_generated.h"
#include "config_generated.h"
#include "entity/component.h"
#include "mathfu/constants.h"
#include "scene_description.h"

namespace fpl {
namespace pie_noon {

class GameState;

const int kMaxAccessories = 15;

// Data for accessory components.
struct PlayerCharacterData {
  entity::EntityRef base_circle;
  entity::EntityRef character;
  entity::EntityRef accessories[kMaxAccessories];
  CharacterId character_id;
};

// Child Objects are basically anything that hangs off of a scene-object as a
// child.  They inherit transformations from their parent.
class PlayerCharacterComponent : public entity::Component<PlayerCharacterData> {
 public:
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void InitEntity(entity::EntityRef& entity);
  void set_gamestate_ptr(GameState* gamestate_ptr) {
    gamestate_ptr_ = gamestate_ptr;
  }

  void set_config(const Config* config) { config_ = config; }

 private:
  void UpdateCharacterFacing(entity::EntityRef entity);
  void UpdateCharacterTint(entity::EntityRef entity);
  void UpdateUiArrow(entity::EntityRef entity);
  void UpdateVisibility(entity::EntityRef entity);
  int PopulatePieAccessories(entity::EntityRef entity, int num_accessories);
  int PopulateHealthAccessories(entity::EntityRef entity, int num_accessories);
  const Config* config_;
  GameState* gamestate_ptr_;
};

}  // pie_noon
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(
    fpl::pie_noon::PlayerCharacterComponent, fpl::pie_noon::PlayerCharacterData,
    fpl::pie_noon::ComponentDataUnion_PlayerCharacterDef)

#endif  // COMPONENTS_PLAYER_CHARACTER_H_
