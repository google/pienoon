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

namespace fpl {
namespace splat {

Character::Character(
    CharacterId id, CharacterId target, CharacterHealth health,
    float face_angle, const mathfu::vec3& position, Controller* controller,
    const CharacterStateMachineDef* character_state_machine_def)
  : id_(id),
    target_(target),
    health_(health),
    pie_damage_(0),
    face_angle_(face_angle),
    position_(position),
    controller_(controller),
    state_machine_(character_state_machine_def)
{}

mathfu::mat4 Character::CalculateMatrix() const {
  return
    mathfu::mat4::FromRotationMatrix(AngleToXZRotationMatrix(face_angle_)) +
    mathfu::mat4::FromTranslationVector(position_);
}

uint16_t Character::RenderableId(WorldTime anim_time) const {
  // The timeline for the current state has an array of renderable ids.
  const Timeline* timeline = state_machine_.current_state()->timeline();
  if (!timeline)
    return RenderableId_CharacterInvalid;

  // Grab the TimelineRenderable for 'anim_time', from the timeline.
  const int renderable_index =
      TimelineIndexBeforeTime(timeline->renderables(), anim_time);
  const TimelineRenderable* renderable =
      timeline->renderables()->Get(renderable_index);
  if (!renderable)
    return RenderableId_CharacterInvalid;

  // Return the renderable id for 'anim_time'.
  return renderable->renderable();
}

// orientation_ and position_ are set each frame in GameState::Advance.
AirbornePie::AirbornePie(CharacterId source, CharacterId target,
                         WorldTime start_time, CharacterHealth damage)
  : source_(source),
    target_(target),
    start_time_(start_time),
    damage_(damage),
    orientation_(0.0f, 0.0f, 1.0f, 0.0f),
    position_(0.0f)
{}

mathfu::mat4 AirbornePie::CalculateMatrix() const {
  return mathfu::mat4::FromRotationMatrix(orientation_.ToMatrix()) +
         mathfu::mat4::FromTranslationVector(position_);
}


} //  namespace fpl
} //  namespace splat

