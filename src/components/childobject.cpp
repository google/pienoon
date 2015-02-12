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

#include "childobject.h"
#include "utilities.h"
#include "scene_object.h"

using mathfu::vec3;
using mathfu::mat4;

namespace fpl {
namespace pie_noon {

void ChildObjectComponent::UpdateAllEntities(entity::WorldTime /*delta_time*/) {
  current_update_id_++;
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef entity = iter->entity;
    PositionAccessory(entity);
  }
}

void ChildObjectComponent::PositionAccessory(entity::EntityRef& entity) {
  ChildObjectData* ac_data = GetEntityData(entity);

  // Check to make sure we're looking at an accessory.  If so, make sure it has
  // not been updated yet this frame, and has a valid parent.
  if (ac_data != nullptr && ac_data->last_update != current_update_id_ &&
      ac_data->parent.IsValid()) {
    SceneObjectData* so_data = Data<SceneObjectData>(entity);

    // just do a quick check to make sure we don't have an obvious loop:
    assert(ac_data->parent != entity);

    // Recursively update the parent's position first, if it hasn't been updated
    // yet.  We need to know where the (ultimate) parent of this objec is
    // before we can correctly position it in the scene.
    PositionAccessory(ac_data->parent);

    // At this point, the parent (and all of its parents) have their final
    // positions.
    const SceneObjectData* parent_so_data =
        Data<SceneObjectData>(ac_data->parent);
    const SceneObjectComponent* scene_object_component =
        GetComponent<SceneObjectComponent>();

    assert(scene_object_component);
    mat4 parent_transform =
        scene_object_component->CalculateMatrix(ac_data->parent);

    so_data->current_transform.position =
        parent_transform * vec3(ac_data->relative_offset);

    so_data->current_transform.orientation =
        (parent_so_data->current_transform.orientation *
         ac_data->relative_orientation);

    so_data->current_transform.scale =
        vec3(parent_so_data->current_transform.scale) *
        vec3(ac_data->relative_scale);

    ac_data->last_update = current_update_id_;
  }
}

void ChildObjectComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_ChildObjectDef);

  ChildObjectData* entity_data = AddEntity(entity);
  const ChildObjectDef* childobject_data =
      static_cast<const ChildObjectDef*>(component_data->data());

  static const float kDegreesToRadians = static_cast<float>(M_PI / 180.0);

  entity_data->relative_orientation = Quat::FromEulerAngles(
      LoadVec3(childobject_data->orientation()) * kDegreesToRadians);
  entity_data->parent = entity::EntityRef();
}

void ChildObjectComponent::InitEntity(entity::EntityRef& entity) {
  ComponentInterface* scene_object_component =
      GetComponent<SceneObjectComponent>();
  assert(scene_object_component);
  scene_object_component->AddEntityGenerically(entity);
  // Initialized to be marked as stale, so it will get an update the first
  // time anything looks at it.
  GetEntityData(entity)->last_update = current_update_id_ - 1;
}

}  // pie noon
}  // fpl
