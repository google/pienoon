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

#ifndef FPL_COMPONENT_H_
#define FPL_COMPONENT_H_

#include "component_id_lookup.h"
#include "component_interface.h"
#include "entity.h"
#include "entity_common.h"
#include "entity_manager.h"
#include "vector_pool.h"

namespace fpl {
namespace entity {

// Component class.
// All components should should extend this class.  The type T is used to
// specify the structure of the data that needs to be associated with each
// entity.
template <typename T>
class Component : public ComponentInterface {
 public:
  // Structure associated with each entity.
  // Contains the template struct, as well as a pointer back to the
  // entity that owns this data.
  struct EntityData {
    EntityRef entity;
    T data;
  };
  typedef typename VectorPool<EntityData>::Iterator EntityIterator;

  Component() : entity_manager_(nullptr) {}

  virtual ~Component() {}

  // AddEntity is a much better function, but we can't have it in the
  // interface class, since it needs to know about type T for the return
  // value.  This provides an alternate way to add things if you don't
  // care about the returned data structure, and you don't feel like
  // casting the BaseComponent into something more specific.
  virtual void AddEntityGenerically(EntityRef& entity) { AddEntity(entity); }

  // Adds an entity to the list of things this component is tracking.
  // Returns the data structure associated with the component.
  // Note that If we're already registered for this component, this
  // will just return a reference to the existing data and not change anything.
  T* AddEntity(EntityRef& entity, AllocationLocation alloc_location) {
    if (entity->IsRegisteredForComponent(GetComponentId())) {
      return GetEntityData(entity);
    }
    // No existing data, so we allocate some and return it:
    size_t index = entity_data_.GetNewElement(alloc_location).index();
    entity->SetComponentDataIndex(GetComponentId(), index);
    EntityData* entity_data = entity_data_.GetElementData(index);
    entity_data->entity = entity;
    new (&(entity_data->data)) T();
    InitEntity(entity);
    return &(entity_data->data);
  }

  T* AddEntity(EntityRef entity) { return AddEntity(entity, kAddToBack); }

  // Removes an entity from our list of entities, marks the entity as not using
  // this component anymore, calls the destructor on the data, and returns
  // the memory to the memory pool.
  virtual void RemoveEntity(EntityRef& entity) {
    RemoveEntityInternal(entity);
    entity_data_.FreeElement(GetEntityDataIndex(entity));
    entity->SetComponentDataIndex(GetComponentId(), kUnusedComponentIndex);
  }

  // Same as RemoveEntity() above, but returns an iterator to the entity after
  // the one we've just removed.
  virtual EntityIterator RemoveEntity(EntityIterator iter) {
    RemoveEntityInternal(iter->entity);
    auto new_iter = entity_data_.FreeElement(iter);
    iter->entity->SetComponentDataIndex(GetComponentId(),
                                        kUnusedComponentIndex);
    return new_iter;
  }

  // Gets an iterator that will iterate over every entity in the component.
  virtual EntityIterator begin() { return entity_data_.begin(); }

  // Gets an iterator which points to the end of the list of all entities in the
  // component.
  virtual EntityIterator end() { return entity_data_.end(); }

  // Updates all entities.  Normally called by EntityManager, once per frame.
  virtual void UpdateAllEntities(WorldTime /*delta_time*/) {}

  // Returns the data for an entity as a void pointer.  The calling function
  // is expected to know what to do with it.
  // Returns null if the data does not exist.
  virtual void* GetEntityDataAsVoid(const EntityRef& entity) {
    return GetEntityData(entity);
  }
  virtual const void* GetEntityDataAsVoid(const EntityRef& entity) const {
    return GetEntityData(entity);
  }

  // Return the data we have stored at a given index.
  // Returns null if data_index indicates this component isn't present.
  T* GetEntityData(size_t data_index) {
    if (data_index == kUnusedComponentIndex) {
      return nullptr;
    }
    EntityData* element_data = entity_data_.GetElementData(data_index);
    return (element_data != nullptr) ? &(element_data->data) : nullptr;
  }

  // Return the data we have stored at a given index.
  // Returns null if data_index indicates this component isn't present.
  T* GetEntityData(const EntityRef& entity) {
    size_t data_index = GetEntityDataIndex(entity);
    if (data_index >= entity_data_.Size()) {
      return nullptr;
    }
    return GetEntityData(data_index);
  }

  // Return the data we have stored at a given index.
  // Returns null if the data does not exist.
  const T* GetEntityData(size_t data_index) const {
    return const_cast<Component*>(this)->GetEntityData(data_index);
  }

  // Return our data for a given entity.
  // Returns null if the data does not exist.
  const T* GetEntityData(const EntityRef& entity) const {
    return const_cast<Component*>(this)->GetEntityData(entity);
  }

  // Clears all tracked entity data.
  void virtual ClearEntityData() {
    for (auto iter = entity_data_.begin(); iter != entity_data_.end();
         iter = RemoveEntity(iter)) {
    }
  }

  // Utility function for getting the component data for a specific component.
  template <typename ComponentDataType>
  ComponentDataType* Data(EntityRef& entity) {
    return entity_manager_->GetComponentData<ComponentDataType>(entity);
  }

  // Utility function for getting the component object for a specific component.
  template <typename ComponentDataType>
  ComponentDataType* GetComponent() {
    return static_cast<ComponentDataType*>(entity_manager_->GetComponent(
        ComponentIdLookup<ComponentDataType>::kComponentId));
  }

  // Virtual methods we inherrited from component_interface:

  // Override this with any code that we want to execute when the component
  // is added to the entity manager.  (i. e. once, at the beginning of the
  // game, before any entities are added.)
  virtual void Init() {}

  // Override this with code that we want to execute when an entity is added.
  virtual void InitEntity(EntityRef& /*entity*/) {}

  // Override this with code that executes when this component is removed from
  // the entity manager.  (i. e. usually when the game/state is over and
  // everythingis shutting down.)
  virtual void Cleanup() {}

  // Override this with any code that needs to execute when an entity is
  // removed from this component.
  virtual void CleanupEntity(EntityRef& /*entity*/) {}

  // Set the entity manager for this component.  (used as the main point of
  // contact for components that need to talk to other things.)
  virtual void SetEntityManager(EntityManager* entity_manager) {
    entity_manager_ = entity_manager;
  }

  // Returns the ID of this component.
  static ComponentId GetComponentId() {
    return ComponentIdLookup<T>::kComponentId;
  }

 private:
  void RemoveEntityInternal(EntityRef& entity) {
    // Allow components to handle any per-entity cleanup that it needs to do.
    CleanupEntity(entity);

    // Manually call the destructor on the data, since it is not actually being
    // freed, just returned to the pool.
    const size_t data_index = GetEntityDataIndex(entity);
    EntityData* entity_data = entity_data_.GetElementData(data_index);
    entity_data->data.~T();

    // Add a reference destructor is empty and the line above is implicitly
    // removed.
    (void)entity_data;
  }

 protected:
  size_t GetEntityDataIndex(const EntityRef& entity) const {
    return entity->GetComponentDataIndex(GetComponentId());
  }

  VectorPool<EntityData> entity_data_;
  EntityManager* entity_manager_;
};

}  // entity
}  // fpl

#endif  // FPL_COMPONENT_H_
