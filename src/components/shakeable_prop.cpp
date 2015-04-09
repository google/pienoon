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

#include <assert.h>
#include "scene_object.h"
#include "shakeable_prop.h"
#include "utilities.h"

using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace pie_noon {

void ShakeablePropComponent::UpdateAllEntities(
    entity::WorldTime /*delta_time*/) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef entity = iter->entity;
    ShakeablePropData* sp_data = GetEntityData(iter->entity);
    SceneObjectData* so_data = Data<SceneObjectData>(entity);
    assert(so_data != nullptr && sp_data != nullptr);

    if (sp_data->motivator.Valid()) {
      so_data->SetPreRotationAboutAxis(sp_data->motivator.Value(),
                                       sp_data->axis);
    }
  }
}

// Add the basic renderable component to the entity, if it doesn't have it
// already.
void ShakeablePropComponent::InitEntity(entity::EntityRef& entity) {
  entity_manager_->AddEntityToComponent(entity,
                                        ComponentDataUnion_SceneObjectDef);
}

void ShakeablePropComponent::AddFromRawData(entity::EntityRef& entity,
                                            const void* data) {
  auto component_data = static_cast<const ComponentDefInstance*>(data);
  assert(component_data->data_type() == ComponentDataUnion_ShakeablePropDef);

  ShakeablePropData* entity_data = AddEntity(entity);
  const ShakeablePropDef* sp_data =
      static_cast<const ShakeablePropDef*>(component_data->data());

  entity_data->axis = sp_data->shake_axis();
  entity_data->shake_scale = sp_data->shake_scale();

  if (sp_data->shake_motivator() != MotivatorSpecification_None) {
    motive::OvershootInit scaled_shake_init =
        motivator_inits[sp_data->shake_motivator()];
    scaled_shake_init.set_range(scaled_shake_init.range() *
                                entity_data->shake_scale);
    scaled_shake_init.set_accel_per_difference(
        scaled_shake_init.accel_per_difference() * entity_data->shake_scale);

    entity_data->motivator.Initialize(scaled_shake_init, engine_);
  }
}

// Preload specifications for motivators from the config file.
void ShakeablePropComponent::LoadMotivatorSpecs() {
  // Load the motivator specifications. Skip over "None".
  auto motivator_specifications = config_->motivator_specifications();
  assert(motivator_specifications->Length() == MotivatorSpecification_Count);
  for (int i = MotivatorSpecification_None + 1;
       i < MotivatorSpecification_Count; ++i) {
    auto specification = motivator_specifications->Get(i);
    motive::OvershootInitFromFlatBuffers(*specification, &motivator_inits[i]);
  }
}

// Invalidate all our motivators before we get removed.
void ShakeablePropComponent::CleanupEntity(entity::EntityRef& entity) {
  ShakeablePropData* sp_data = GetEntityData(entity);
  sp_data->motivator.Invalidate();
}

// General function to shake props when something hits near them.
// Usually called by gamestate, in response to a pie landing.
void ShakeablePropComponent::ShakeProps(float damage_percent,
                                        const vec3& damage_position) {
  (void)damage_percent;
  (void)damage_position;

  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    ShakeablePropData* data = &iter->data;

    SceneObjectData* so_data = Data<SceneObjectData>(iter->entity);

    float shake_scale = data->shake_scale;
    if (shake_scale == 0.0f) {
      continue;
    }

    // We always want to add to the speed, so if the current velocity is
    // negative, we add a negative amount.
    const float current_velocity = data->motivator.Velocity();
    const float current_direction = current_velocity >= 0.0f ? 1.0f : -1.0f;

    // The closer the prop is to the damage_position, the more it should shake.
    // The effect trails off with distance squared.
    const vec3 prop_position = so_data->GlobalPosition();
    const float closeness =
        mathfu::Clamp(config_->prop_shake_identity_distance_sq() /
                          (damage_position - prop_position).LengthSquared(),
                      0.01f, 1.0f);

    // Velocity added is the product of all the factors.
    const float delta_velocity = current_direction * damage_percent *
                                 closeness * shake_scale *
                                 config_->prop_shake_velocity();
    const float new_velocity = current_velocity + delta_velocity;
    const float current_value = data->motivator.Value();
    data->motivator.SetTarget(motive::Current1f(current_value, new_velocity));
  }
}

}  // pie noon
}  // fpl
