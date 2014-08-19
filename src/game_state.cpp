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
#include "splat_common_generated.h" // TODO: put in alphabetical order when
                                    // FlatBuffers predeclare bug fixed.
#include "config_generated.h"
#include "controller.h"
#include "scene_description.h"
#include "utilities.h"

namespace fpl {
namespace splat {

GameState::GameState()
  : time_(0),
    camera_position_(0.0f, 0.0f, 0.0f),
    camera_target_(0.0f, 0.0f, 0.0f),
    characters_(),
    pies_(),
    config_() {
}

// Calculate the direction a character is facing at the start of the game.
// We want the characters to face their initial target.
static Angle InitialFaceAngle(const CharacterId id, const Config& config) {
  const mathfu::vec3 characterPosition =
      LoadVec3(config.character_positions()->Get(id));
  const CharacterId target_id = config.character_targets()->Get(id);
  const mathfu::vec3 targetPosition =
      LoadVec3(config.character_positions()->Get(target_id));
  return Angle::FromXZVector(targetPosition - characterPosition);
}

// Reset the game back to initial configuration.
void GameState::Reset() {
  time_ = 0;
  camera_position_ = LoadVec3(config_->camera_position());
  camera_target_ = LoadVec3(config_->camera_target());
  pies_.empty();

  // Reset characters to their initial state.
  const CharacterId num_ids = static_cast<CharacterId>(characters_.size());
  for (CharacterId id = 0; id < num_ids; ++id) {
    characters_[id].Reset(config_->character_targets()->Get(id),
                          config_->character_health(),
                          InitialFaceAngle(id, *config_),
                          LoadVec3(config_->character_positions()->Get(id)));
  }
}

WorldTime GameState::GetAnimationTime(const Character& character) const {
  return time_ - character.state_machine()->current_state_start_time();
}

void GameState::ProcessEvents(Character* character, WorldTime delta_time,
                              int queued_damage) {
  // Process events in timeline.
  const Timeline* const timeline = character->CurrentTimeline();
  if (!timeline)
    return;

  const WorldTime animTime = GetAnimationTime(*character);
  const auto events = timeline->events();
  const int start_index = TimelineIndexAfterTime(events, 0, animTime);
  const int end_index = TimelineIndexAfterTime(events, start_index,
                                               animTime + delta_time);

  for (int i = start_index; i < end_index; ++i) {
    const TimelineEvent& timeline_event = *events->Get(i);
    switch (timeline_event.event())
    {
      case EventId_TakeDamage: {
        character->set_health(character->health() - queued_damage);
        break;
      }
      case EventId_ReleasePie: {
        float height = config_->pie_height();
        height += config_->pie_height_variance() *
                  (mathfu::Random<float>() * 2 - 1);
        pies_.push_back(AirbornePie(character->id(), character->target(),
                                    time_, config_->pie_flight_time(),
                                    character->pie_damage(), height));
        UpdatePiePosition(&pies_.back());
        break;
      }
      case EventId_LoadPie: {
        character->set_pie_damage(timeline_event.modifier());
        break;
      }
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
    const Character& target, WorldTime time_since_launch,
    WorldTime flight_time, float pie_height) {
  float percent = static_cast<float>(time_since_launch) / flight_time;
  percent = mathfu::Clamp(percent, 0.0f, 1.0f);
  mathfu::vec3 result =
      mathfu::vec3::Lerp(source.position(), target.position(), percent);

  // Pie height follows a parabola such that y = -4a * (x)(x - 1)
  //
  // (x)(x - 1) gives a parabola with the x intercepts at 0 and 1 (where 0
  // represents the origin, and 1 represents the target). The height of the pie
  // would only be .25 units maximum, so we multiply by 4 to make the peak 1
  // unit. Finally, we multiply by an arbitrary coeffecient supplied in a config
  // file to make the pies fly higher or lower.
  result.y() += -4 * pie_height * (percent * (percent - 1.0f));

  return result;
}

void GameState::UpdatePiePosition(AirbornePie* pie) const {
  const WorldTime time_since_launch = time_ - pie->start_time();
  const Character& source = characters_[pie->source()];
  const Character& target = characters_[pie->target()];

  const Quat pie_orientation = CalculatePieOrientation(source, target,
                                                       time_since_launch);
  const mathfu::vec3 pie_position = CalculatePiePosition(source, target,
                                                         time_since_launch,
                                                         pie->flight_time(),
                                                         pie->height());

  pie->set_orientation(pie_orientation);
  pie->set_position(pie_position);
}

uint16_t GameState::CharacterState(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  return characters_[id].state_machine()->current_state()->id();
}

CharacterId GameState::CalculateCharacterTarget(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  const Character& c = characters_[id];
  const CharacterId current_target = c.target();

  // If you yourself are KO'd, then you can't change target.
  const int target_state = CharacterState(id);
  if (target_state == StateId_KO)
    return current_target;

  // Check the inputs to see how requests for target change.
  const uint32_t logical_inputs = c.controller()->logical_inputs();
  const int target_delta = (logical_inputs & LogicalInputs_Left) ? -1 :
                           (logical_inputs & LogicalInputs_Right) ? 1 : 0;
  if (target_delta == 0)
    return current_target;

  const CharacterId character_count =
      static_cast<CharacterId>(characters_.size());
  for (CharacterId target_id = current_target + target_delta;;
       target_id += target_delta) {
    // Wrap around.
    if (target_id >= character_count) {
      target_id = 0;
    } else if (target_id < 0) {
      target_id = character_count - 1;
    }

    // If we've looped around, no one else to target.
    if (target_id == current_target)
      return current_target;

    // Avoid targeting yourself.
    // Avoid looping around to the other side.
    if (target_id == id)
      return current_target;

    // Don't target KO'd characters.
    const int target_state = CharacterState(target_id);
    if (target_state == StateId_KO)
      continue;

    // All targetting criteria satisfied.
    return target_id;
  }
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

  // Calculate the current error in our facing angle.
  const Angle delta_angle = FaceAngleError(c.id());

  // Increment our current face angle velocity.
  const bool wrong_direction =
      c.face_angle_velocity() * delta_angle.angle() < 0.0f;
  const float angular_acceleration =
      delta_angle.angle() * config_->face_delta_to_accel() *
      (wrong_direction ? config_->face_wrong_direction_accel_bonus() : 1.0f);
  const float angular_velocity_unclamped =
      c.face_angle_velocity() + delta_time * angular_acceleration;
  const float max_velocity = config_->face_max_velocity();
  const float angular_velocity =
      mathfu::Clamp(angular_velocity_unclamped, -max_velocity, max_velocity);

  // If we're close to facing the target, just snap to it.
  const float near_target_angular_velocity =
      config_->face_near_target_angular_velocity();
  const bool snap_to_target =
      fabs(angular_velocity) <= near_target_angular_velocity &&
      fabs(delta_angle.angle()) <= config_->face_near_target_angle();
  return snap_to_target
       ? mathfu::Clamp(delta_angle.angle() / delta_time,
            -near_target_angular_velocity, near_target_angular_velocity)
       : angular_velocity;
}

void GameState::AdvanceFrame(WorldTime delta_time) {
  // Increment the world time counter. This happens at the start of the function
  // so that functions that reference the current world time will include the
  // delta_time. For example, GetAnimationTime needs to compare against the
  // time for *this* frame, not last frame.
  time_ += delta_time;

  // Damage is queued up per character then applied during event processing.
  std::vector<int> queued_damage(characters_.size(), 0);

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
  for (auto it = pies_.begin(); it != pies_.end(); ) {
    AirbornePie& pie = *it;
    UpdatePiePosition(&pie);

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - pie.start_time();
    if (time_since_launch >= pie.flight_time()) {
      Character& character = characters_[pie.target()];
      queued_damage[character.id()] += pie.damage();
      character.controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      it = pies_.erase(it);
    }
    else {
      ++it;
    }
  }

  // Update the character state machines and the facing angles.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    // Update state machines.
    TransitionInputs transition_inputs;
    transition_inputs.logical_inputs = it->controller()->logical_inputs();
    transition_inputs.animation_time = GetAnimationTime(*it);
    transition_inputs.current_time = time_;
    it->state_machine()->Update(transition_inputs);

    // Update target.
    const CharacterId target_id = CalculateCharacterTarget(it->id());
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
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessEvents(&characters_[i], delta_time, queued_damage[i]);
  }
}

static uint16_t RenderableIdForPieDamage(CharacterHealth damage,
                                         const Config& config) {
  const int first_id = config.first_airborne_pie_renderable();
  const int last_id = config.last_airborne_pie_renderable();
  const int max_damage = last_id - first_id;
  const int clamped_damage = std::min(damage, max_damage);
  return static_cast<uint16_t>(first_id + clamped_damage);
}

// Get the camera matrix used for rendering.
mathfu::mat4 GameState::CameraMatrix() const {
  return mathfu::mat4::LookAt(camera_target_, camera_position_,
                              mathfu::vec3(0.0f, 1.0f, 0.0f));
}

static const mathfu::mat4 CalculateAccessoryMatrix(
    const TimelineAccessory& accessory, const mathfu::mat4& character_matrix,
    const Config& config) {

  // Calculate the accessory offset, in character space.
  const mathfu::vec3 offset = mathfu::vec3(
      accessory.offset().x() * config.pixel_to_world_scale(),
      accessory.offset().y() * config.pixel_to_world_scale(),
      config.accessory_z_offset());

  // Apply the offset to the character matrix to move the object relative to
  // the character.
  const mathfu::mat4 offset_matrix =
      mathfu::mat4::FromTranslationVector(offset);
  const mathfu::mat4 accessory_matrix = character_matrix * offset_matrix;
  return accessory_matrix;
}

static const mathfu::mat4 CalculateSplatterMatrix(
    const Vec2i* location, const mathfu::mat4& character_matrix,
    const Config& config) {
  // Calculate the accessory offset, in character space.
  const mathfu::vec3 offset = mathfu::vec3(
      static_cast<float>(location->x()) * config.pixel_to_world_scale(),
      static_cast<float>(location->y()) * config.pixel_to_world_scale(),
      config.accessory_z_offset());
  const mathfu::mat4 offset_matrix =
      mathfu::mat4::FromTranslationVector(offset);
  const mathfu::mat4 accessory_matrix = character_matrix * offset_matrix;
  return accessory_matrix;
}

static mathfu::mat4 CalculatePropWorldMatrix(const Prop& prop) {
  const mathfu::vec3& position = LoadVec3(&prop.position());
  const Angle rotation = Angle::FromDegrees(prop.rotation());
  const mathfu::vec3& rotation_axis = LoadVec3(&prop.rotation_axis());
  const Quat quat = Quat::FromAngleAxis(rotation.angle(), rotation_axis);
  return mathfu::mat4::FromTranslationVector(position) *
         mathfu::mat4::FromRotationMatrix(quat.ToMatrix());
}

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(SceneDescription* scene) const {
  scene->Clear();

  // Camera.
  scene->set_camera(CameraMatrix());

  // Characters and accessories.
  if (config_->draw_characters()) {
    for (auto c = characters_.begin(); c != characters_.end(); ++c) {
      // Character.
      const WorldTime anim_time = GetAnimationTime(*c);
      const mathfu::mat4 character_matrix = c->CalculateMatrix();
      scene->renderables().push_back(
          Renderable(c->RenderableId(anim_time), character_matrix));

      // Accessories.
      const Timeline* const timeline = c->CurrentTimeline();
      if (!timeline)
        continue;

      // Get accessories that are valid for the current time.
      const std::vector<int> accessory_indices =
          TimelineIndicesWithTime(timeline->accessories(), anim_time);

      // Create a renderable for each accessory.
      for (auto it = accessory_indices.begin();
           it != accessory_indices.end(); ++it) {
        const TimelineAccessory& accessory = *timeline->accessories()->Get(*it);
        scene->renderables().push_back(
            Renderable(accessory.renderable(),
              CalculateAccessoryMatrix(accessory, character_matrix, *config_)));
      }

      // Splatter
      unsigned int damage = config_->character_health() - c->health();
      for (unsigned int i = 0;
           i < damage && i < config_->splatter()->Length(); ++i) {
        const Splatter* splatter = config_->splatter()->Get(i);
        scene->renderables().push_back(
            Renderable(splatter->renderable(),
                CalculateSplatterMatrix(splatter->location(),
                                        character_matrix,
                                        *config_)));
      }
    }
  }

  // Pies.
  if (config_->draw_pies()) {
    for (auto it = pies_.begin(); it != pies_.end(); ++it) {
      const AirbornePie& pie = *it;
      scene->renderables().push_back(
          Renderable(RenderableIdForPieDamage(pie.damage(), *config_),
                     pie.CalculateMatrix()));
    }
  }

  // Populate scene description with environment items.
  if (config_->draw_props()) {
    auto props = config_->props();
    for (size_t i = 0; i < props->Length(); ++i) {
      const Prop& prop = *props->Get(i);
      scene->renderables().push_back(
          Renderable(static_cast<uint16_t>(prop.renderable()),
                     CalculatePropWorldMatrix(prop)));
    }
  }

  // Axes. Useful for debugging.
  // Positive x axis is long. Positive z axis is short.
  // Positive y axis is shortest.
  if (config_->draw_axes()) {
    // TODO: add an arrow renderable instead of drawing with pies.
    for (int i = 0; i < 8; ++i) {
      const mathfu::mat4 axis_dot = mathfu::mat4::FromTranslationVector(
          mathfu::vec3(static_cast<float>(i), 0.0f, 0.0f));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
    for (int i = 0; i < 4; ++i) {
      const mathfu::mat4 axis_dot = mathfu::mat4::FromTranslationVector(
          mathfu::vec3(0.0f, 0.0f, static_cast<float>(i)));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
    for (int i = 0; i < 2; ++i) {
      const mathfu::mat4 axis_dot = mathfu::mat4::FromTranslationVector(
          mathfu::vec3(0.0f, static_cast<float>(i), 0.0f));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
  }

  // Draw one renderable right in the middle of the world, for debugging.
  if (config_->draw_fixed_renderable() != RenderableId_Invalid) {
    scene->renderables().push_back(
        Renderable(config_->draw_fixed_renderable(), mathfu::mat4::Identity()));
  }

  // Lights. Push all lights from configuration file.
  const auto lights = config_->light_positions();
  for (auto it = lights->begin(); it != lights->end(); ++it) {
    const mathfu::vec3 light_position = LoadVec3(*it);
    scene->lights().push_back(light_position);
  }
}

}  // splat
}  // fpl

