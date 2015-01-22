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

#ifndef FPL_ENTITY_H_
#define FPL_ENTITY_H_

#include "entity_common.h"

namespace fpl {
namespace entity {

// Basic entity class for the entity system.  Contains an array of index
// values used by components for tracking their data associated with an entity
// and a boolean for tracking if this entity is marked for deletion.
class Entity {
 public:
  Entity() : marked_for_deletion_(false) {
    for (int i = 0; i < kMaxComponentCount; i++) {
      componentDataIndex_[i] = kUnusedComponentIndex;
    }
  }

  // Returns the index of the data in the corresponding component system.
  int GetComponentDataIndex(ComponentId componentId) const {
    return componentDataIndex_[componentId];
  }

  // Sets the index for the data associated with this entity in the specified
  // component.
  void SetComponentDataIndex(ComponentId componentId, ComponentIndex value) {
    componentDataIndex_[componentId] = value;
  }

  // Utility function for checking if this entity is associated with a
  // specific component.
  bool IsRegisteredForComponent(ComponentId componentId) const {
    return componentDataIndex_[componentId] != kUnusedComponentIndex;
  }

  // Member variable getter
  bool marked_for_deletion() const { return marked_for_deletion_; }

  // Member variable setter
  void set_marked_for_deletion(bool marked_for_deletion) {
    marked_for_deletion_ = marked_for_deletion;
  }

 private:
  ComponentIndex componentDataIndex_[kMaxComponentCount];
  bool marked_for_deletion_;
};

}  // entity
}  // fpl
#endif  // FPL_ENTITY_H_
