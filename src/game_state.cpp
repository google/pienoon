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
#include "game_state.h"
#include "character_state_machine.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "controller.h"
#include "scene_description.h"

namespace fpl {
namespace splat {

// Time between pie being launched and hitting target.
// TODO: This should be in a configuration file.
static const WorldTime kPieFlightTime = 30;

WorldTime GameState::GetAnimationTime(const Character& c) const {
  return time_ - c.state_machine()->current_state_start_time();
}

void GameState::ProcessEvents(Character& c, WorldTime delta_time) {
  // Process events in timeline.
  const Timeline* const timeline =
      c.state_machine()->current_state()->timeline();
  if (!timeline)
    return;

  const WorldTime animTime = GetAnimationTime(c);
  const auto events = timeline->events();
  const int start_index = TimelineIndexAfterTime(events, 0, animTime);
  const int end_index = TimelineIndexAfterTime(events, start_index,
                                               animTime + delta_time);

  for (int i = start_index; i < end_index; ++i) {
    const TimelineEvent& timeline_event = *events->Get(i);
    switch (timeline_event.event())
    {
      case EventId_TakeDamage:
        c.set_health(std::max(c.health() + timeline_event.modifier(), 0));
        break;

      case EventId_ReleasePie:
        pies_.push_back(AirbornePie(c.id(), c.target(), time_,
                                    c.pie_damage()));
        break;

      case EventId_LoadPie:
        c.set_pie_damage(std::max(c.pie_damage() + timeline_event.modifier(),
                                  0));
        break;

      default:
        assert(0);
    }
  }
}

static Quat CalculatePieOrientation(const Character& source,
    const Character& target, WorldTime time_since_launch) {
  (void)source; (void)target; (void)time_since_launch;

  // TODO: Make pie spin about axis of rotation.
  return Quat(0.0f, mathfu::vec3(0.0f, 1.0f, 0.0f));
}

static mathfu::vec3 CalculatePiePosition(const Character& source,
    const Character& target, WorldTime time_since_launch) {
  // TODO: Make the pie follow a trajectory.
  const float percent = static_cast<float>(time_since_launch) / kPieFlightTime;
  return mathfu::vec3::Lerp(source.position(), target.position(), percent);
}

void GameState::UpdatePiePosition(AirbornePie& pie) const {
  const WorldTime time_since_launch = time_ - pie.start_time();
  const Character& source = characters_[pie.source()];
  const Character& target = characters_[pie.target()];

  const Quat pie_orientation = CalculatePieOrientation(source, target,
                                                       time_since_launch);
  const mathfu::vec3 pie_position = CalculatePiePosition(source, target,
                                                         time_since_launch);

  pie.set_orientation(pie_orientation);
  pie.set_position(pie_position);
}

static CharacterId CalculateCharacterTarget(const Character& c,
                                            const int character_count) {
  const uint32_t logical_inputs = c.controller()->logical_inputs();

  if (logical_inputs & LogicalInputs_Left)
    return c.target() == 0 ? character_count - 1 : c.target() - 1;

  if (logical_inputs & LogicalInputs_Right)
    return c.target() == character_count - 1 ? 0 : c.target() + 1;

  return c.target();
}

// Angle to the character's target.
Angle GameState::TargetFaceAngle(CharacterId id) const {
  const Character& c = characters_[id];
  const Character& target = characters_[c.target()];
  const mathfu::vec3 vector_to_target = target.position() - c.position();
  const Angle angle_to_target = Angle::FromXZVector(vector_to_target);
  return angle_to_target;
}

// Difference between target face angle and current face angle.
Angle GameState::FaceAngleError(CharacterId id) const {
  const Character& c = characters_[id];
  const Angle angle_to_target = TargetFaceAngle(id);
  const Angle face_angle_error = angle_to_target - c.face_angle();
  return face_angle_error;
}

// The character's face angle accelerates towards the
float GameState::CalculateCharacterFacingAngleVelocity(
    const Character& c, WorldTime delta_time) const {
  // TODO: Read these constants from a configuration FlatBuffer.
  static const float kDeltaToAccel = 0.001f / kPi;
  static const float kWrongDirectionAccelBonus = 3.0f;
  static const float kMaxVelocity = kPi / 200.0f;
  static const float kNearTargetAngularVelocity = kPi / 1500.0f;
  static const float kNearTargetAngle = kPi / 60.0f;

  // Calculate the current error in our facing angle.
  const Angle delta_angle = FaceAngleError(c.id());

  // Increment our current face angle velocity.
  const bool wrong_direction =
      c.face_angle_velocity() * delta_angle.angle() < 0.0f;
  const float angular_acceleration =
      delta_angle.angle() * kDeltaToAccel *
      (wrong_direction ? kWrongDirectionAccelBonus : 1.0f);
  const float angular_velocity_unclamped =
      c.face_angle_velocity() + delta_time * angular_acceleration;
  const float angular_velocity =
      mathfu::Clamp(angular_velocity_unclamped, -kMaxVelocity, kMaxVelocity);

  // If we're close to facing the target, just snap to it.
  const bool snap_to_target =
      fabs(angular_velocity) <= kNearTargetAngularVelocity &&
      fabs(delta_angle.angle()) <= kNearTargetAngle;
  return snap_to_target
       ? mathfu::Clamp(delta_angle.angle() / delta_time,
            -kNearTargetAngularVelocity, kNearTargetAngularVelocity)
       : angular_velocity;
}

void GameState::AdvanceFrame(WorldTime delta_time) {
  // Increment the world time counter. This happens at the start of the function
  // so that functions that reference the current world time will include the
  // delta_time. For example, GetAnimationTime needs to compare against the
  // time for *this* frame, not last frame.
  time_ += delta_time;

  // Update controller to gather state machine inputs.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    Controller* controller = it->controller();
    controller->AdvanceFrame();
    controller->SetLogicalInputs(LogicalInputs_JustHit, false);
    controller->SetLogicalInputs(LogicalInputs_NoHealth, it->health() <= 0);
    const Timeline* timeline = it->state_machine()->current_state()->timeline();
    controller->SetLogicalInputs(LogicalInputs_AnimationEnd,
        timeline && (GetAnimationTime(*it) >= timeline->end_time()));
  }

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end(); ++it) {
    UpdatePiePosition(*it);

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - it->start_time();
    if (time_since_launch >= kPieFlightTime) {
      Character& c = characters_[it->target()];
      c.controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      it = pies_.erase(it);
    }
  }
  int index = 0;
  // Update the character state machines and the facing angles.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    // Update state machines.
    TransitionInputs transition_inputs;
    transition_inputs.logical_inputs = it->controller()->logical_inputs();
    transition_inputs.animation_time = GetAnimationTime(*it);
    it->state_machine()->Update(transition_inputs);

    //  hack to give them different locations.
    it->set_position(mathfu::vec3((index - 2) * 10, 0, (index - 2) * 10));
    index++;

    // Update target.
    const CharacterId target_id = CalculateCharacterTarget(*it,
                                                           characters_.size());
    it->set_target(target_id);

    // Update facing angles.
    const float face_angle_velocity =
        CalculateCharacterFacingAngleVelocity(*it, delta_time);
    const Angle face_angle = Angle::FromWithinThreePi(
        it->face_angle().angle() + delta_time * face_angle_velocity);
    it->set_face_angle_velocity(face_angle_velocity);
    it->set_face_angle(face_angle);
  }

  // Look to timeline to see what's happening. Make it happen.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    ProcessEvents(*it, delta_time);
  }
}

static uint16_t RenderableIdForPieDamage(CharacterHealth damage) {
  const int kMaxDamageForRenderable =
    RenderableId_PieLarge - RenderableId_PieEmpty;
  const int clamped_damage = std::min(damage, kMaxDamageForRenderable);
  return static_cast<uint16_t>(RenderableId_PieEmpty + clamped_damage);
}

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(SceneDescription* scene) const {
  // Camera.
  scene->Clear();
  scene->set_camera(mathfu::mat4::FromTranslationVector(
                        mathfu::vec3(0.0f, 5.0f, -10.0f)));

  // Characters.
  for (auto c = characters_.begin(); c != characters_.end(); ++c) {
    const WorldTime anim_time = GetAnimationTime(*c);
    scene->renderables().push_back(
        Renderable(c->RenderableId(anim_time), c->CalculateMatrix()));
  }

  // Pies.
  for (auto it = pies_.begin(); it != pies_.end(); ++it) {
    const AirbornePie& pie = *it;
    scene->renderables().push_back(
        Renderable(RenderableIdForPieDamage(pie.damage()),
                                            pie.CalculateMatrix()));
  }

  // Lights.
  scene->lights().push_back(mathfu::vec3(-20.0f, 20.0f, -20.0f));
}

}  // splat
}  // fpl

