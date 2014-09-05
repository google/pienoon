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
    CharacterId id, Controller* controller, const Config& config,
    const CharacterStateMachineDef* character_state_machine_def)
    : config_(&config),
      id_(id),
      target_(0),
      health_(0),
      pie_damage_(0),
      position_(mathfu::kZeros3f),
      controller_(controller),
      state_machine_(character_state_machine_def) {
  for (int i = 0; i < kMaxStats; i++) player_stats_[i] = 0;
}

void Character::Reset(CharacterId target, CharacterHealth health,
                      Angle face_angle, const vec3& position) {
  target_ = target;
  health_ = health;
  pie_damage_ = 0;
  position_ = position;
  state_machine_.Reset();

  face_angle_.Initialize(*config_->face_angle_constraints(),
                         *config_->face_angle_magnet_def(),
                         MagnetState1f(face_angle.ToRadians(), 0.0f));
}

void Character::SetTarget(CharacterId target, Angle angle_to_target) {
  target_ = target;
  face_angle_.SetTargetPosition(angle_to_target.ToRadians());
}

void Character::TwitchFaceAngle(MagnetTwitch twitch) {
  face_angle_.Twitch(twitch);
}

void Character::AdvanceFrame(WorldTime delta_time) {
  face_angle_.AdvanceFrame(delta_time);
}

mat4 Character::CalculateMatrix(bool facing_camera) const {
  const Angle face_angle = FaceAngle();
  return mat4::FromTranslationVector(position_) *
         mat4::FromRotationMatrix(face_angle.ToXZRotationMatrix()) *
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

void Character::IncrementStat(PlayerStats stat) {
  player_stats_[stat]++;
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

