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
#include "components_generated.h"
#include "scene_description.h"
#include "mathfu/constants.h"
#include "motive/motivator.h"

namespace impel {
  class MatrixImpelInit;
}

namespace fpl {
namespace pie_noon {

// Data for scene object components.
class SceneObjectData {
 public:
  SceneObjectData()
      : global_matrix_(mathfu::mat4::Identity()),
        tint_(mathfu::kOnes4f),
        renderable_id_(0),
        visible_(true) {
  }
  void Initialize(impel::ImpelEngine* impel_engine);

  // Set components of the transformation from object-to-local space.
  // We apply a fixed transformation to objects:
  //     1. scale
  //     2. translate to the object's origin
  //     3. rotate about z, then y, then x
  //     4. rotate again about z, then y, then x
  //     5. translate to final location
  // We have two rotation steps because some objects need to rotate about x
  // before y, say. For maximum flexibility we could have three rotation
  // steps, but that seems excessive.
  //
  // See comment on TransformMatrixOperations for more information.
  // TODO: Allow callers to set up their own transformation pipeline, instead
  // of using this fixed one.
  void SetRotation(const mathfu::vec3& rotation) {
    transform_.SetChildValue3f(kRotateAboutX, rotation);
  }
  void SetRotationAboutX(float angle) {
    transform_.SetChildValue1f(kRotateAboutX, angle);
  }
  void SetRotationAboutY(float angle) {
    transform_.SetChildValue1f(kRotateAboutY, angle);
  }
  void SetRotationAboutZ(float angle) {
    transform_.SetChildValue1f(kRotateAboutZ, angle);
  }
  void SetRotationAboutAxis(float angle, Axis axis) {
    transform_.SetChildValue1f(kRotateAboutX + axis, angle);
  }
  void SetPreRotation(const mathfu::vec3& rotation) {
    transform_.SetChildValue3f(kPreRotateAboutX, rotation);
  }
  void SetPreRotationAboutX(float angle) {
    transform_.SetChildValue1f(kPreRotateAboutX, angle);
  }
  void SetPreRotationAboutY(float angle) {
    transform_.SetChildValue1f(kPreRotateAboutY, angle);
  }
  void SetPreRotationAboutZ(float angle) {
    transform_.SetChildValue1f(kPreRotateAboutZ, angle);
  }
  void SetPreRotationAboutAxis(float angle, Axis axis) {
    transform_.SetChildValue1f(kPreRotateAboutX + axis, angle);
  }
  void SetTranslation(const mathfu::vec3& translation) {
    transform_.SetChildValue3f(kTranslateX, translation);
  }
  void SetScale(const mathfu::vec3& scale) {
    transform_.SetChildValue3f(kScaleX, scale);
  }
  void SetScaleX(float scale) { transform_.SetChildValue1f(kScaleX, scale); }
  void SetScaleY(float scale) { transform_.SetChildValue1f(kScaleY, scale); }
  void SetScaleZ(float scale) { transform_.SetChildValue1f(kScaleZ, scale); }
  void SetOriginPoint(const mathfu::vec3& origin) {
    transform_.SetChildValue3f(kTranslateToOriginX, -origin);
  }

  // Get components of the transformation from object-to-local space.
  mathfu::vec3 Translation() const {
    return transform_.ChildValue3f(kTranslateX);
  }
  mathfu::vec3 Rotation() const {
    return transform_.ChildValue3f(kRotateAboutX);
  }
  mathfu::vec3 Scale() const {
    return transform_.ChildValue3f(kScaleX);
  }
  mathfu::vec3 OriginPoint() const {
    return transform_.ChildValue3f(kTranslateToOriginX);
  }

  const mathfu::mat4& LocalMatrix() const { return transform_.Value(); }
  const mathfu::vec3 GlobalPosition() const {
    return global_matrix_.TranslationVector3D();
  }
  void set_global_matrix(const mathfu::mat4& m) { global_matrix_ = m; }
  const mathfu::mat4& global_matrix() const { return global_matrix_; }

  bool HasParent() const { return parent_.IsValid(); }
  entity::EntityRef& parent() { return parent_; }
  const entity::EntityRef& parent() const { return parent_; }
  void set_parent(entity::EntityRef& parent) { parent_ = parent; }

  mathfu::vec4 tint() const { return mathfu::vec4(tint_); }
  void set_tint(const mathfu::vec4& tint) { tint_ = tint; }

  uint16_t renderable_id() const { return renderable_id_; }
  void set_renderable_id(uint16_t id) { renderable_id_ = id; }

  bool visible() const { return visible_; }
  void set_visible(bool visible) { visible_ = visible; }

 private:
  // Basic matrix operations from with 'transform_.Value()' is calculated.
  // These operations are applied last-to-first to convert the object from
  // object space (i.e. the space in which it was authored) to local space
  // (i.e. the space relative to 'parent_').
  enum TransformMatrixOperations {
    kTranslateX,
    kTranslateY,
    kTranslateZ,
    kRotateAboutX,
    kRotateAboutY,
    kRotateAboutZ,
    kPreRotateAboutX,
    kPreRotateAboutY,
    kPreRotateAboutZ,
    kTranslateToOriginX,
    kTranslateToOriginY,
    kTranslateToOriginZ,
    kScaleX,
    kScaleY,
    kScaleZ,
    kNumTransformMatrixOperations
  };

  // Position, orientation, and scale (in world-space) of the object.
  mathfu::mat4 global_matrix_;

  // Position, orientation, and scale (in local space) of the object.
  // Composed of the basic matrix operations in TransformMatrixOperations.
  impel::ImpellerMatrix4f transform_;

  // The parent defines the scene heirarchy. This scene object is positioned
  // relative to its parent. That is,
  //    global_matrix_ = parent_->global_matrix * transform_.Value()
  // If no parent is specified, the 'transform_' is assumed to be in global
  // space already.
  entity::EntityRef parent_;

  // Color of object.
  mathfu::vec4_packed tint_;

  // Id of object model to render.
  uint16_t renderable_id_;

  // Whether object is currently on-screen or not.
  bool visible_;
};

// A sceneobject is "a thing I want to place in the scene and move around."
// So it contains basic drawing info.
class SceneObjectComponent : public entity::Component<SceneObjectData> {
 public:
  explicit SceneObjectComponent(impel::ImpelEngine* impel_engine)
      : impel_engine_(impel_engine) {
  }
  virtual void AddFromRawData(entity::EntityRef& entity, const void* data);
  virtual void InitEntity(entity::EntityRef& entity);
  void PopulateScene(SceneDescription* scene);

 private:
  void UpdateGlobalMatrix(entity::EntityRef& entity,
                          std::vector<bool>& matrix_calculated);
  void UpdateGlobalMatrices();

  impel::ImpelEngine* impel_engine_;
};

}  // pie_noon
}  // fpl

FPL_ENTITY_REGISTER_COMPONENT(fpl::pie_noon::SceneObjectComponent,
                              fpl::pie_noon::SceneObjectData,
                              fpl::pie_noon::ComponentDataUnion_SceneObjectDef)

#endif  // COMPONENTS_SCENEOBJECT_H_
