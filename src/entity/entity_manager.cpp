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
#include "component_id_lookup.h"
#include "entity_manager.h"

namespace fpl {
namespace entity {

EntityManager::EntityManager() : entity_factory_(nullptr) {
  for (int i = 0; i < kMaxComponentCount; i++) {
    components_[i] = nullptr;
  }
}

EntityRef EntityManager::AllocateNewEntity() {
  return EntityRef(entities_.GetNewElement(kAddToFront));
}

// Note: This function doesn't actually delete the entity immediately -
// it just marks it for deletion, and it gets cleaned out at the end of the
// next AdvanceFrame.
void EntityManager::DeleteEntity(EntityRef entity) {
  if (entity->marked_for_deletion()) {
    // already marked for deletion.
    return;
  }
  entity->set_marked_for_deletion(true);
  entities_to_delete_.push_back(entity);
}

// This deletes the entity instantly.  You should generally use the regular
// DeleteEntity unless you have a particuarly good reason to need it instantly.
void EntityManager::DeleteEntityImmediately(EntityRef entity) {
  RemoveAllComponents(entity);
  entities_.FreeElement(entity);
}

void EntityManager::DeleteMarkedEntities() {
  for (size_t i = 0; i < entities_to_delete_.size(); i++) {
    EntityRef entity = entities_to_delete_[i];
    RemoveAllComponents(entity);
    entities_.FreeElement(entity);
  }
  entities_to_delete_.resize(0);
}

void EntityManager::RemoveAllComponents(EntityRef entity) {
  for (ComponentId i = 0; i < kMaxComponentCount; i++) {
    if (entity->IsRegisteredForComponent(i)) {
      components_[i]->RemoveEntity(entity);
    }
  }
}

void EntityManager::AddEntityToComponent(EntityRef entity,
                                         ComponentId component_id) {
  ComponentInterface* component = GetComponent(component_id);
  assert(component != nullptr);
  component->AddEntityGenerically(entity);
}

void EntityManager::RegisterComponentHelper(ComponentInterface* new_component,
                                            ComponentId id) {
  assert(id < kMaxComponentCount);
  // Make sure this ID isn't already associated with a component.
  assert(components_[id] == nullptr);
  components_[id] = new_component;
  new_component->SetEntityManager(this);
  new_component->Init();
}

void* EntityManager::GetComponentDataAsVoid(EntityRef entity,
                                            ComponentId component_id) {
  return components_[component_id]
             ? components_[component_id]->GetEntityDataAsVoid(entity)
             : nullptr;
}

const void* EntityManager::GetComponentDataAsVoid(
    EntityRef entity, ComponentId component_id) const {
  return components_[component_id]
             ? components_[component_id]->GetEntityDataAsVoid(entity)
             : nullptr;
}

void EntityManager::UpdateComponents(WorldTime delta_time) {
  // Update all the registered components.
  for (size_t i = 0; i < kMaxComponentCount; i++) {
    if (components_[i]) components_[i]->UpdateAllEntities(delta_time);
  }
  DeleteMarkedEntities();
}

void EntityManager::Clear() {
  for (size_t i = 0; i < kMaxComponentCount; i++) {
    if (components_[i]) {
      components_[i]->ClearEntityData();
      components_[i]->Cleanup();
    }
    components_[i] = nullptr;
  }
  entities_.Clear();
}

EntityRef EntityManager::CreateEntityFromData(const void* data) {
  assert(entity_factory_ != nullptr);
  return entity_factory_->CreateEntityFromData(data, this);
}

}  // entity
}  // fpl
