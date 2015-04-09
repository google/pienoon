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

#ifndef FPL_ENTITY_MANAGER_H_
#define FPL_ENTITY_MANAGER_H_

#include "component_id_lookup.h"
#include "component_interface.h"
#include "entity.h"
#include "entity_common.h"
#include "vector_pool.h"

namespace fpl {
namespace entity {

typedef VectorPool<Entity>::VectorPoolReference EntityRef;

class EntityFactoryInterface;
class ComponentInterface;

// Entity Manager is the main piece of code that manages all entities and
// components in the game.  Normally the game will instantiate one instance
// of this, and then use it to create and control entities.  The main
// uses for this class are:
//  * Creating/destroying new entities
//  * Keeping track of all components
//  * Spawning and populating new components from data
//  * Updating entities/components.  (Usually done once per frame.)
class EntityManager {
 public:
  EntityManager();
  typedef VectorPool<Entity> EntityStorageContainer;

  // Helper function for marshalling data from a component.
  // Returns nullptr if it couldn't find it for any reason.
  template <typename T>
  T* GetComponentData(EntityRef& entity) {
    return static_cast<T*>(GetComponentDataAsVoid(
        entity, ComponentIdLookup<T>::kComponentId));
  }

  // Helper function for marshalling data from a component.
  // Returns nullptr if it couldn't find it for any reason.
  template <typename T>
  const T* GetComponentData(const EntityRef& entity) const {
    return static_cast<const T*>(GetComponentDataAsVoid(
        entity, ComponentIdLookup<T>::kComponentId));
  }

  // Helper function for getting a particular component, given its datatype.
  // Asserts if it doesn't exist.
  template <typename T>
  T* GetComponent() {
    ComponentId id = ComponentIdLookup<T>::kComponentId;
    assert(id != kInvalidComponent);
    assert(id < kMaxComponentCount);
    return static_cast<T*>(components_[id]);
  }

  // Helper function for getting a particular component, given its datatype.
  // Asserts if it doesn't exist.
  template <typename T>
  const T* GetComponent() const {
    ComponentId id = ComponentIdLookup<T>::kComponentId;
    assert(id != kInvalidComponent);
    assert(id < kMaxComponentCount);
    return static_cast<const T*>(components_[id]);
  }

  // Helper function for getting a particular component, given the component ID.
  inline ComponentInterface* GetComponent(ComponentId id) {
    assert(id < kMaxComponentCount);
    return components_[id];
  }

  // Helper function for getting a particular component, given the component ID.
  // (Const version)
  inline const ComponentInterface* GetComponent(ComponentId id) const {
    assert(id < kMaxComponentCount);
    return components_[id];
  }

  // Allocates a new entity, which is registered with no components.
  // Returns an entityref to the new entity.
  EntityRef AllocateNewEntity();

  // Deletes an entity, removing it from our list, and clearing any component
  // data associated with it.
  // Note: Deletion is deferred until the end of the frame.  If you want to
  // delete something instantly, use DeleteEntityImmediately.
  void DeleteEntity(EntityRef entity);

  // Deletes an entity instantly.  In general, you should use DeleteEntity,
  // (which defers deletion until the end of the update cycle) unless you have
  // a very good reason for doing so.
  void DeleteEntityImmediately(EntityRef entity);

  // Adds a new component to the entity manager.  The id represents a unique
  // identifier for that component, which will other code can use to
  // reference that component.
  template <typename T>
  void RegisterComponent(ComponentInterface* new_component) {
    RegisterComponentHelper(new_component, ComponentIdLookup<T>::kComponentId);
  }

  // Removes all components from an entity, causing any associated components
  // to drop any data they have allocated for the entity.
  void RemoveAllComponents(EntityRef entity);

  // Iterates through all registered components, and causes them to update.
  // delta_time represents the timestep since last update.
  void UpdateComponents(WorldTime delta_time);

  // Clears all data from all components, then dumps the list of components
  // themselves, and then dumps the list of entities.  Basically resets
  // the entity manager into its original state.
  void Clear();

  // functions for returning iterators suitable for iterating over every
  // active entity.
  EntityStorageContainer::Iterator begin() { return entities_.begin(); }
  EntityStorageContainer::Iterator end() { return entities_.end(); }

  // Registers a factory object to be used when entities are created from
  // arbitrary data.  The entity factory is responsible for taking random
  // data, and correctly transforming it into an entity.  (Usually used
  // when loading entities from data files.)
  void set_entity_factory(EntityFactoryInterface* entity_factory) {
    entity_factory_ = entity_factory;
  }

  // Creates an entity from raw data.  Usually used by the entity factory
  // specified in set_entity_factory.
  EntityRef CreateEntityFromData(const void* data);

  // Registers an entity with a component.  This causes the component to
  // allocate data for the entity, and includes the entity in that component's
  // update routines.
  void AddEntityToComponent(EntityRef entity, ComponentId component_id);

 private:
  // Does all the real work of registering a component, aside from template fun.
  // In particular, verifies that the requested ID isn't already in use,
  // puts a pointer to the new component in our components_ array, and
  // sets starting variables and preforms initialization on the component.
  void RegisterComponentHelper(ComponentInterface* new_component,
                               ComponentId id);

  // Given a component ID and an entity, returns all data associated
  // with that entity from the given component.  Will assert if the inputs
  // are not valid.  (entity is no longer active or component is invalid)
  // Data is returned as a void pointer - the caller is expected to know how
  // to interpret it.
  void* GetComponentDataAsVoid(EntityRef entity, ComponentId component_id);

  // Given a component ID and an entity, returns all data associated
  // with that entity from the given component.  Will assert if the inputs
  // are not valid.  (entity is no longer active or component is invalid)
  // Data is returned as a void pointer - the caller is expected to know how
  // to interpret it.
  const void* GetComponentDataAsVoid(EntityRef entity,
                                     ComponentId component_id) const;

  // Delete all the entities we have marked for deletion.
  void DeleteMarkedEntities();
  // Storage of all the entities currently tracked by the entitymanager
  EntityStorageContainer entities_;

  // All the components that are tracked by the system, and are ready to
  // have entities added to them.
  ComponentInterface* components_[kMaxComponentCount];
  // Entities that we plan to delete at the end of the frame.
  std::vector<EntityRef> entities_to_delete_;
  // Factory used for spawning new entities from data.  Provided by the
  // calling program.
  EntityFactoryInterface* entity_factory_;
};

class EntityFactoryInterface {
 public:
  virtual EntityRef CreateEntityFromData(const void* data,
                                         EntityManager* entity_manager) = 0;
};

}  // entity
}  // fpl
#endif  // FPL_ENTITY_MANAGER_H_
