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

#ifndef COMPONENTS_CHILDOBJECT_H_
#define COMPONENTS_CHILDOBJECT_H_

#include "entity/component.h"
#include "common.h"
#include "scene_description.h"
#include "mathfu/constants.h"

namespace fpl {
namespace pie_noon {

// Data for accessory components.
struct ChildObjectData {
  ChildObjectData()
      : relative_scale(mathfu::kOnes3f),
        relative_offset(mathfu::kOnes3f),
        last_update(0) {
    relative_orientation = Quat::FromAngleAxis(0, mathfu::kOnes3f);
  }
  // Scale of this object, relative to its parent:
  mathfu::vec3_packed relative_scale;
  // offset of this object, relative to its parent's origin point:
  mathfu::vec3_packed relative_offset;
  // orientation of this object, relative to the parent's orientation.
  // TODO: Change this to a quat_packed once we make one.
  Quat relative_orientation;
  // identifier for whether the object has been updated this frame or not
  char last_update;
  // Our parent object.
  entity::EntityRef parent;
};

// An accessory is basically anything that hangs off of a scene-object as a
// child.  Accessories inherit transformations from their parent.
class ChildObjectComponent : public entity::Component<ChildObjectData> {
 public:
  ChildObjectComponent() : current_update_id_(0) {}
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(entity::WorldTime /*delta_time*/);
  virtual void InitEntity(entity::EntityRef& entity);

 private:
  void PositionAccessory(entity::EntityRef& entity);
  // Used to track the last time this entity's position was updated by the
  // parent/child heiarchy.  (The order ends up being weird because children
  // need to force their parents to update before they can update.  We use
  // this to make sure we don't update anything more than once in the same
  // loop.)
  char current_update_id_;
};

}  // pie_noon
}  // fpl
#endif  // COMPONENTS_CHILDOBJECT_H_
