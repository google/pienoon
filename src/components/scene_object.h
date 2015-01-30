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

#ifndef COMPONENTS_SCENEOBJECT_H_
#define COMPONENTS_SCENEOBJECT_H_

#include "entity/component.h"
#include "common.h"
#include "scene_description.h"
#include "mathfu/constants.h"

namespace fpl {
namespace pie_noon {

struct SceneObjectTransform {
  SceneObjectTransform()
    : position(mathfu::kZeros3f),
      scale(mathfu::kOnes3f) {
    orientation = Quat::FromAngleAxis(0, mathfu::kOnes3f);
  }
  mathfu::vec3_packed position;
  mathfu::vec3_packed scale;
  //TODO: Change to Quat_Packed, once it exists.
  Quat orientation;
};

// Data for scene object components.
struct SceneObjectData {
  SceneObjectData()
    : tint(mathfu::kOnes4f),
      origin_point(mathfu::kZeros3f),
      renderable_id(0),
      visible(true) {}
  // the current transform on the object.  Probably modified by a lot of other
  // things.  Reset to the base at the start of every frame.
  SceneObjectTransform current_transform;
  // The "original" transfrorm for this object.  Nothing else should touch this.
  SceneObjectTransform base_transform;
  mathfu::vec4_packed tint;
  mathfu::vec3_packed origin_point;
  uint16_t renderable_id;
  bool visible;
};


// A sceneobject is "a thing I want to place in the scene and move around."
// So it contains basic drawing info.  (position, scale, orientation, tint.)
// This is basically a helper component, that provides a friendlier interface
// on top of the much more raw BasicRenderable.
// Note: Adding this component to an entity will also add BasicRenderable.
// (Basic renderable still does the actual rendering, this just provides a more
// friendly interface than having to provide raw transformation matricies.
class SceneObjectComponent : public entity::Component<SceneObjectData> {
 public:
  virtual void UpdateAllEntities(entity::WorldTime delta_time);
  virtual void AddFromRawData(entity::EntityRef& entity,
                              const void* data);
  void PopulateScene(SceneDescription* scene);
  mathfu::mat4 CalculateMatrix(const SceneObjectData* data) const;
  mathfu::mat4 CalculateMatrix(const entity::EntityRef& entity) const;
};

}  // pie_noon
}  // fpl
#endif // COMPONENTS_SCENEOBJECT_H_
