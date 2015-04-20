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

#include "cardboard_player.h"
#include "character.h"
#include "game_state.h"
#include "player_character.h"
#include "scene_object.h"
#include "utilities.h"

using mathfu::vec3;

namespace fpl {
namespace pie_noon {

void CardboardPlayerComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef entity = iter->entity;
    UpdateTargetReticle(entity);
    UpdateLoadedPie(entity);
    UpdateHealthAccessories(entity);
  }
}

void CardboardPlayerComponent::UpdateTargetReticle(entity::EntityRef entity) {
  CardboardPlayerData* cp_data = GetEntityData(entity);
  std::vector<std::unique_ptr<Character>>& character_vector =
      gamestate_ptr_->characters();
  std::unique_ptr<Character>& character =
      character_vector[cp_data->character_id];
  std::unique_ptr<Character>& target = character_vector[character->target()];
  const vec3 to_target = target->position() - character->position();
  Angle angle_to_target =
      Angle::FromXZVector(to_target) + Angle::FromRadians(fpl::kHalfPi);

  SceneObjectData* target_so_data =
      Data<SceneObjectData>(cp_data->target_reticle);
  target_so_data->SetTranslation(
      character->position() + config_->target_reticle_distance() * to_target +
      config_->target_reticle_height() * mathfu::kAxisY3f);
  target_so_data->SetRotationAboutY(-angle_to_target.ToRadians());
}

void CardboardPlayerComponent::UpdateLoadedPie(entity::EntityRef entity) {
  CardboardPlayerData* cp_data = GetEntityData(entity);
  std::vector<std::unique_ptr<Character>>& character_vector =
      gamestate_ptr_->characters();
  std::unique_ptr<Character>& character =
      character_vector[cp_data->character_id];
  SceneObjectData* pie_so_data = Data<SceneObjectData>(cp_data->loaded_pie);

  if (character->pie_damage() <= 0) {
    pie_so_data->set_visible(false);
  } else {
    const CharacterHealth index = mathfu::Clamp<CharacterHealth>(
        character->pie_damage(), 0,
        config_->blocked_sound_id_for_pie_damage()->Length() - 1);
    const auto& id = config_->renderable_id_for_pie_damage()->Get(index);

    pie_so_data->set_renderable_id(id);
    pie_so_data->set_visible(true);
  }
}

void CardboardPlayerComponent::UpdateHealthAccessories(
    entity::EntityRef entity) {
  CardboardPlayerData* cp_data = GetEntityData(entity);
  std::vector<std::unique_ptr<Character>>& character_vector =
      gamestate_ptr_->characters();
  std::unique_ptr<Character>& character =
      character_vector[cp_data->character_id];

  const int max_key = config_->health_map()->Length() - 1;
  const int key = mathfu::Clamp(character->health(), 0, max_key);
  auto indices = config_->health_map()->Get(key)->indices();
  const int num_indices = mathfu::Clamp(static_cast<int>(indices->Length()), 0,
                                        kMaxHealthAccessories);
  int i = 0;
  for (; i < num_indices; i++) {
    const int index = indices->Get(i);
    const FixedAccessory* heart = config_->health_accessories()->Get(index);
    const vec2 location(LoadVec2i(heart->location()));
    const vec3 offset = vec3(location.y() * config_->pixel_to_world_scale(),
                             -location.x() * config_->pixel_to_world_scale(),
                             -i * config_->accessory_z_increment());
    const vec2 scale(LoadVec2(heart->scale()));

    entity::EntityRef& heart_entity = cp_data->health[i];
    SceneObjectData* heart_so_data = Data<SceneObjectData>(heart_entity);
    heart_so_data->set_visible(true);
    heart_so_data->SetTranslation(LoadVec3(config_->cardboard_health_offset()) +
                                  offset);
    heart_so_data->set_renderable_id(heart->renderable());
    heart_so_data->SetScale(vec3(scale.x(), scale.y(), 1));
  }
  // Turn the remaining health accessories off
  for (; i < kMaxHealthAccessories; i++) {
    SceneObjectData* heart_so_data = Data<SceneObjectData>(cp_data->health[i]);
    heart_so_data->set_visible(false);
  }
}

void CardboardPlayerComponent::AddFromRawData(entity::EntityRef& entity,
                                              const void* /*raw_data*/) {
  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_CardboardPlayerDef);
}

void CardboardPlayerComponent::InitEntity(entity::EntityRef& entity) {
  CardboardPlayerData* cp_data = GetEntityData(entity);

  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_PlayerCharacterDef);
  PlayerCharacterData* pc_data = Data<PlayerCharacterData>(entity);

  cp_data->target_reticle = entity_manager_->AllocateNewEntity();
  entity_manager_->AddEntityToComponent(cp_data->target_reticle,
                                        ComponentDataUnion_SceneObjectDef);
  SceneObjectData* target_so_data =
      Data<SceneObjectData>(cp_data->target_reticle);
  target_so_data->set_renderable_id(RenderableId_TargetReticle);

  // Set up the pie display, attached to the arrow on the PlayerCharacter
  cp_data->loaded_pie = entity_manager_->AllocateNewEntity();
  entity_manager_->AddEntityToComponent(cp_data->loaded_pie,
                                        ComponentDataUnion_SceneObjectDef);
  SceneObjectData* pie_so_data = Data<SceneObjectData>(cp_data->loaded_pie);
  pie_so_data->set_parent(pc_data->base_circle);
  pie_so_data->SetRotationAboutZ(-fpl::kHalfPi);
  pie_so_data->SetTranslation(LoadVec3(config_->cardboard_pie_offset()));
  pie_so_data->SetScale(LoadVec3(config_->cardboard_pie_scale()));

  // Set up slots for health, attached to the arrow
  for (int i = 0; i < kMaxHealthAccessories; i++) {
    entity::EntityRef& heart = cp_data->health[i];
    heart = entity_manager_->AllocateNewEntity();
    entity_manager_->AddEntityToComponent(heart,
                                          ComponentDataUnion_SceneObjectDef);
    SceneObjectData* heart_so_data = Data<SceneObjectData>(heart);
    heart_so_data->set_parent(pc_data->base_circle);
    heart_so_data->SetRotationAboutZ(-fpl::kHalfPi);
    heart_so_data->set_visible(false);
  }
}

}  // pie noon
}  // fpl
