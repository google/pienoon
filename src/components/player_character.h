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
#include "corgi/component.h"
#include "mathfu/constants.h"
#include "scene_description.h"

namespace fpl {
namespace pie_noon {

class GameState;

const int kMaxAccessories = 15;

// Data for accessory components.
struct PlayerCharacterData {
  corgi::EntityRef base_circle;
  corgi::EntityRef character;
  corgi::EntityRef accessories[kMaxAccessories];
  CharacterId character_id;
};

// Child Objects are basically anything that hangs off of a scene-object as a
// child.  They inherit transformations from their parent.
class PlayerCharacterComponent : public corgi::Component<PlayerCharacterData> {
 public:
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);
  virtual void InitEntity(corgi::EntityRef& entity);
  void set_gamestate_ptr(GameState* gamestate_ptr) {
    gamestate_ptr_ = gamestate_ptr;
  }

  void set_config(const Config* config) { config_ = config; }

 private:
  void UpdateCharacterFacing(corgi::EntityRef entity);
  void UpdateCharacterTint(corgi::EntityRef entity);
  void UpdateUiArrow(corgi::EntityRef entity);
  void UpdateVisibility(corgi::EntityRef entity);
  int PopulatePieAccessories(corgi::EntityRef entity, int num_accessories);
  int PopulateHealthAccessories(corgi::EntityRef entity, int num_accessories);
  Controller::ControllerType ControllerType(
      const corgi::EntityRef& entity) const;
  bool DrawBaseCircle(const corgi::EntityRef& entity) const;

  const Config* config_;
  GameState* gamestate_ptr_;
};

}  // pie_noon
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::pie_noon::PlayerCharacterComponent,
                         fpl::pie_noon::PlayerCharacterData)

#endif  // COMPONENTS_PLAYER_CHARACTER_H_
