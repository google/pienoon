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

#include "scene_object.h"
#include "components_generated.h"
#include "utilities.h"

namespace fpl {
namespace pie_noon {

using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

mat4 SceneObjectComponent::CalculateMatrix(const SceneObjectData* data) const {
  return mat4::FromTranslationVector(vec3(data->current_transform.position)) *
         mat4::FromRotationMatrix(
             data->current_transform.orientation.ToMatrix()) *
         mat4::FromTranslationVector(-vec3(data->origin_point)) *
         mat4::FromScaleVector(vec3(data->current_transform.scale));
}

mathfu::mat4 SceneObjectComponent::CalculateMatrix(
    const entity::EntityRef& entity) const {
  const SceneObjectData* data = GetEntityData(entity);
  return CalculateMatrix(data);
}

void SceneObjectComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    SceneObjectData* sp_data = GetEntityData(iter->entity);
    // Reset our transform.
    sp_data->current_transform = sp_data->base_transform;
  }
}

void SceneObjectComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_SceneObjectDef);

  SceneObjectData* entity_data = AddEntity(entity);
  const SceneObjectDef* scene_object_data =
      static_cast<const SceneObjectDef*>(component_data->data());

  entity_data->base_transform.position =
      LoadVec3(scene_object_data->position());
  static const float kDegreesToRadians = static_cast<float>(M_PI / 180.0);
  mathfu::vec3 orientation_in_degrees =
      LoadVec3(scene_object_data->orientation());
  entity_data->base_transform.orientation =
      Quat::FromEulerAngles(orientation_in_degrees * kDegreesToRadians);
  entity_data->origin_point = LoadVec3(scene_object_data->origin_point());
  entity_data->base_transform.scale = LoadVec3(scene_object_data->scale());
  entity_data->renderable_id = scene_object_data->renderable_id();
  entity_data->tint = LoadVec4(scene_object_data->tint());
  entity_data->visible = scene_object_data->visible();
}

void SceneObjectComponent::PopulateScene(SceneDescription* scene) {
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef entity = iter->entity;
    SceneObjectData* data = GetEntityData(entity);
    if (data->visible) {
      scene->renderables().push_back(std::unique_ptr<Renderable>(new Renderable(
          data->renderable_id, CalculateMatrix(data), vec4(data->tint))));
    }
  }
}

}  // pie noon
}  // fpl
