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

#include "precompiled.h"
#include "components_generated.h"
#include "motive/init.h"
#include "motive/math/angle.h"
#include "scene_object.h"

namespace fpl {
namespace pie_noon {

using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;
using motive::kDegreesToRadians;

// Basic matrix operation for each component of the 'transform_'
// Matrix Motivator.
static const motive::MatrixOperationType kTransformOperations[] = {
    motive::kTranslateX,    // kTranslateX
    motive::kTranslateY,    // kTranslateY
    motive::kTranslateZ,    // kTranslateZ
    motive::kRotateAboutX,  // kRotateAboutX
    motive::kRotateAboutY,  // kRotateAboutY
    motive::kRotateAboutZ,  // kRotateAboutZ
    motive::kRotateAboutX,  // kPreRotateAboutX
    motive::kRotateAboutY,  // kPreRotateAboutY
    motive::kRotateAboutZ,  // kPreRotateAboutZ
    motive::kTranslateX,    // kTranslateToOriginX
    motive::kTranslateY,    // kTranslateToOriginY
    motive::kTranslateZ,    // kTranslateToOriginZ
    motive::kScaleX,        // kScaleX
    motive::kScaleY,        // kScaleY
    motive::kScaleZ,        // kScaleZ
};

void SceneObjectData::Initialize(motive::MotiveEngine* engine) {
  MATHFU_STATIC_ASSERT(PIE_ARRAYSIZE(kTransformOperations) ==
                       kNumTransformMatrixOperations);

  // Create init structure for the 'transform_' Matrix Motivator.
  // TODO: This structure is the same every time. Change MatrixInit to be
  // constructable from POD so that this can be a constexpr.
  motive::MatrixOpArray ops(PIE_ARRAYSIZE(kTransformOperations));
  for (size_t i = 0; i < PIE_ARRAYSIZE(kTransformOperations); ++i) {
    motive::MatrixOperationType op = kTransformOperations[i];
    const float default_value =
        motive::kScaleX <= op && op <= motive::kScaleZ ? 1.0f : 0.0f;
    ops.AddOp(op, default_value);
  }
  motive::MatrixInit init(ops);
  transform_.Initialize(init, engine);
}

void SceneObjectComponent::AddFromRawData(entity::EntityRef& entity,
                                          const void* raw_data) {
  auto component_data = static_cast<const ComponentDefInstance*>(raw_data);
  assert(component_data->data_type() == ComponentDataUnion_SceneObjectDef);

  SceneObjectData* entity_data = AddEntity(entity);
  const SceneObjectDef* scene_object_data =
      static_cast<const SceneObjectDef*>(component_data->data());

  mathfu::vec3 orientation_in_degrees =
      LoadVec3(scene_object_data->orientation());

  entity_data->SetTranslation(LoadVec3(scene_object_data->position()));
  entity_data->SetRotation(orientation_in_degrees * kDegreesToRadians);
  entity_data->SetScale(LoadVec3(scene_object_data->scale()));
  entity_data->SetOriginPoint(LoadVec3(scene_object_data->origin_point()));

  entity_data->set_renderable_id(scene_object_data->renderable_id());
  entity_data->set_variant(scene_object_data->variant());
  entity_data->set_tint(LoadVec4(scene_object_data->tint()));
  entity_data->set_visible(scene_object_data->visible() != 0);
}

void SceneObjectComponent::InitEntity(entity::EntityRef& entity) {
  SceneObjectData* data = GetEntityData(entity);
  data->Initialize(engine_);
}

void SceneObjectComponent::UpdateGlobalMatrix(
    entity::EntityRef& entity, std::vector<bool>& matrix_updated) {
  const size_t data_index = GetEntityDataIndex(entity);
  SceneObjectData* data = GetEntityData(data_index);

  if (data->HasParent()) {
    // Recurse into parent if its matrix has not been calculated.
    const size_t parent_index = GetEntityDataIndex(data->parent());
    if (!matrix_updated[parent_index]) {
      UpdateGlobalMatrix(data->parent(), matrix_updated);
    }

    // Multiply our local matrix by our parent's global matrix to get our
    // global matrix.
    SceneObjectData* parent = GetEntityData(parent_index);
    data->set_global_matrix(parent->global_matrix() * data->LocalMatrix());
  } else {
    // No parent means that our local matrix equals the global matrix.
    data->set_global_matrix(data->LocalMatrix());
  }

  // Remember that we've calculated this matrix so we don't try to calculate
  // it again.
  matrix_updated[data_index] = true;
}

// Traverse scene hierarchy convert local matrices into global matrices.
void SceneObjectComponent::UpdateGlobalMatrices() {
  std::vector<bool> matrix_updated(entity_data_.Size(), false);

  // Loop through every entity and update its global matrix.
  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    // The update process is recursive, so we may have already calculated a
    // matrix by the time we get there. If so, skip over it.
    if (!matrix_updated[iter.index()] && iter->data.visible()) {
      UpdateGlobalMatrix(iter->entity, matrix_updated);
    }
  }
}

bool SceneObjectComponent::VisibleInHierarchy(
    const entity::EntityRef& entity) const {
  const SceneObjectData* data = GetEntityData(entity);
  if (!data->HasParent()) {
    return data->visible();
  } else {
    return data->visible() && VisibleInHierarchy(data->parent());
  }
}

void SceneObjectComponent::PopulateScene(SceneDescription* scene) {
  UpdateGlobalMatrices();

  for (auto iter = entity_data_.begin(); iter != entity_data_.end(); ++iter) {
    entity::EntityRef entity = iter->entity;
    if (VisibleInHierarchy(entity)) {
      SceneObjectData* data = GetEntityData(entity);
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(data->renderable_id(), data->variant(),
                         data->global_matrix(), data->tint())));
    }
  }
}

}  // pie noon
}  // fpl
