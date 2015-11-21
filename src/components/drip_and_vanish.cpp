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

#include "drip_and_vanish.h"
#include "scene_object.h"

namespace fpl {
namespace pie_noon {

using mathfu::vec3;

// DripAndVanish component does exactly one, highly specific thing:
// Gives a child scene object behavior such that it waits for a while,
// and then slowly sinks, while shrinking.  It's used to govern behavior
// for splatters on the background.
void DripAndVanishComponent::UpdateAllEntities(entity::WorldTime delta_time) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    SceneObjectData* so_data = Data<SceneObjectData>(iter->entity);
    DripAndVanishData* dv_data = GetEntityData(iter->entity);

    dv_data->lifetime_remaining -= delta_time;
    if (dv_data->lifetime_remaining > 0) {
      if (dv_data->lifetime_remaining < dv_data->slide_time) {
        float slide_amount =
            1.0f - dv_data->lifetime_remaining / dv_data->slide_time;

        vec3 relative_offset = so_data->Translation();
        vec3 relative_scale = so_data->Scale();

        // The amount it moves is a cubic function, mostly because
        // that looked the prettiest.
        relative_offset.y() = vec3(dv_data->start_position).y() -
                              (slide_amount * slide_amount * slide_amount) *
                                  dv_data->drip_distance;

        relative_scale = vec3(dv_data->start_scale) * (1.0f - slide_amount);

        so_data->SetTranslation(relative_offset);
        so_data->SetScale(relative_scale);
      }
    } else {
      entity_manager_->DeleteEntity(iter->entity);
    }
  }
}

void DripAndVanishComponent::AddFromRawData(entity::EntityRef& entity,
                                            const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_DripAndVanishDef);

  DripAndVanishData* entity_data = AddEntity(entity);
  const DripAndVanishDef* dripandvanish_data =
      static_cast<const DripAndVanishDef*>(component_data->data());

  entity_data->drip_distance = dripandvanish_data->distance_dripped();
  entity_data->lifetime_remaining =
      dripandvanish_data->total_lifetime() * kMillisecondsPerSecond;
  entity_data->slide_time =
      dripandvanish_data->time_spent_dripping() * kMillisecondsPerSecond;
}

// Make sure we have an accessory.
void DripAndVanishComponent::InitEntity(entity::EntityRef& entity) {
  ComponentInterface* scene_object_component =
      GetComponent<SceneObjectComponent>();
  assert(scene_object_component);
  scene_object_component->AddEntityGenerically(entity);
}

// Set the starting values of the drip thing, since we populate them
// just after creation.
void DripAndVanishComponent::SetStartingValues(entity::EntityRef& entity) {
  DripAndVanishData* entity_data = GetEntityData(entity);
  SceneObjectData* so_data = Data<SceneObjectData>(entity);

  entity_data->start_position = so_data->Translation();
  entity_data->start_scale = so_data->Scale();
}

}  // pie noon
}  // fpl
