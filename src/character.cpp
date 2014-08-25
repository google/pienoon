// Copyright 2014 Google Inc. All rights reserved.
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
#include <math.h>
#include "character.h"
#include "character_state_machine.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "splat_common_generated.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace splat {

Character::Character(
    CharacterId id, Controller* controller,
    const CharacterStateMachineDef* character_state_machine_def)
    : id_(id),
      target_(0),
      health_(0),
      pie_damage_(0),
      face_angle_(Angle(0.0f)),
      face_angle_velocity_(0.0f),
      position_(vec3(0.0f, 0.0f, 0.0f)),
      controller_(controller),
      state_machine_(character_state_machine_def) {
}

void Character::Reset(CharacterId target, CharacterHealth health,
                      Angle face_angle, const vec3& position) {
  target_ = target;
  health_ = health;
  pie_damage_ = 0;
  face_angle_ = face_angle;
  face_angle_velocity_ = 0.0f;
  position_ = position;
}

mat4 Character::CalculateMatrix(bool facing_camera) const {
  return mat4::FromTranslationVector(position_) *
         mat4::FromRotationMatrix(face_angle_.ToXZRotationMatrix()) *
         mat4::FromScaleVector(vec3(1.0f, 1.0f, facing_camera ? 1.0f : -1.0f));
}

uint16_t Character::RenderableId(WorldTime anim_time) const {
  // The timeline for the current state has an array of renderable ids.
  const Timeline* timeline = state_machine_.current_state()->timeline();
  if (!timeline || !timeline->renderables())
    return RenderableId_Invalid;

  // Grab the TimelineRenderable for 'anim_time', from the timeline.
  const int renderable_index =
      TimelineIndexBeforeTime(timeline->renderables(), anim_time);
  const TimelineRenderable* renderable =
      timeline->renderables()->Get(renderable_index);
  if (!renderable)
    return RenderableId_Invalid;

  // Return the renderable id for 'anim_time'.
  return renderable->renderable();
}

// orientation_ and position_ are set each frame in GameState::Advance.
AirbornePie::AirbornePie(CharacterId source, CharacterId target,
                         WorldTime start_time, WorldTime flight_time,
                         CharacterHealth damage, float height, int rotations)
    : source_(source),
      target_(target),
      start_time_(start_time),
      flight_time_(flight_time),
      damage_(damage),
      height_(height),
      rotations_(rotations),
      orientation_(0.0f, 0.0f, 1.0f, 0.0f),
      position_(0.0f) {
}

mat4 AirbornePie::CalculateMatrix() const {
  return mat4::FromTranslationVector(position_) *
         mat4::FromRotationMatrix(orientation_.ToMatrix());
}

} //  namespace fpl
} //  namespace splat

