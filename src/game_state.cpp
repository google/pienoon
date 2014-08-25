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

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace splat {

static const mat4 kRotate90DegreesAboutXAxis(1,  0, 0, 0,
                                             0,  0, 1, 0,
                                             0, -1, 0, 0,
                                             0,  0, 0, 1);

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
  const vec3 characterPosition =
      LoadVec3(&config.character_data()->Get(id)->position());
  const CharacterId target_id =
      config.character_data()->Get(id)->initial_target_id();
  const vec3 targetPosition =
      LoadVec3(&config.character_data()->Get(target_id)->position());
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
    characters_[id].Reset(
        config_->character_data()->Get(id)->initial_target_id(),
        config_->character_health(),
        InitialFaceAngle(id, *config_),
        LoadVec3(&config_->character_data()->Get(id)->position()));
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
        float height = config_->pie_arc_height();
        height += config_->pie_arc_height_variance() *
                  (mathfu::Random<float>() * 2 - 1);
        int rotations = config_->pie_rotations();
        int variance = config_->pie_rotation_variance();
        rotations += variance ? (rand() % (variance * 2)) - variance : 0;
        pies_.push_back(AirbornePie(character->id(), character->target(),
                                    time_, config_->pie_flight_time(),
                                    character->pie_damage(), height,
                                    rotations));
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

static Quat CalculatePieOrientation(Angle pie_angle, float percent,
                                    int rotations, const Config* config) {
  // These are floats instead of Angles because they may need to rotate more
  // than 360 degrees. Values are negative so that they rotate in the correct
  // direction.
  float initial_angle = -config->pie_initial_angle();
  float target_angle = -(config->pie_target_angle() +
                         rotations * kDegreesPerCircle);
  float delta = target_angle - initial_angle;

  Angle rotation_angle = Angle::FromDegrees(initial_angle + (delta * percent));
  Quat pie_direction = Quat::FromAngleAxis(pie_angle.angle(),
                                           vec3(0.0f, 1.0f, 0.0f));
  Quat pie_rotation = Quat::FromAngleAxis(rotation_angle.angle(),
                                          vec3(0.0f, 0.0f, 1.0f));
  return pie_direction * pie_rotation;
}

static vec3 CalculatePiePosition(const Character& source,
                                         const Character& target,
                                         float percent, float pie_height,
                                         const Config* config) {
  vec3 result =
      vec3::Lerp(source.position(), target.position(), percent);

  // Pie height follows a parabola such that y = -4a * (x)(x - 1)
  //
  // (x)(x - 1) gives a parabola with the x intercepts at 0 and 1 (where 0
  // represents the origin, and 1 represents the target). The height of the pie
  // would only be .25 units maximum, so we multiply by 4 to make the peak 1
  // unit. Finally, we multiply by an arbitrary coeffecient supplied in a config
  // file to make the pies fly higher or lower.
  result.y() += -4 * pie_height * (percent * (percent - 1.0f));
  result.y() += config->pie_initial_height();

  return result;
}

void GameState::UpdatePiePosition(AirbornePie* pie) const {
  const Character& source = characters_[pie->source()];
  const Character& target = characters_[pie->target()];

  const float time_since_launch = static_cast<float>(time_ - pie->start_time());
  float percent = time_since_launch / config_->pie_flight_time();
  percent = mathfu::Clamp(percent, 0.0f, 1.0f);

  Angle pie_angle = -AngleBetweenCharacters(pie->source(), pie->target());

  const Quat pie_orientation = CalculatePieOrientation(
      pie_angle, percent, pie->rotations(), config_);
  const vec3 pie_position = CalculatePiePosition(
      source, target, percent, pie->height(), config_);

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
  const int left_jump = config_->character_data()->Get(id)->left_jump();
  const int target_delta = (logical_inputs & LogicalInputs_Left) ? left_jump :
                           (logical_inputs & LogicalInputs_Right) ? -left_jump :
                           0;
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

// The angle between two characters.
Angle GameState::AngleBetweenCharacters(CharacterId source_id,
                                        CharacterId target_id) const {
  const Character& source = characters_[source_id];
  const Character& target = characters_[target_id];
  const vec3 vector_to_target = target.position() - source.position();
  const Angle angle_to_target = Angle::FromXZVector(vector_to_target);
  return angle_to_target;
}

// Angle to the character's target.
Angle GameState::TargetFaceAngle(CharacterId id) const {
  const Character& c = characters_[id];
  return AngleBetweenCharacters(id, c.target());
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
mat4 GameState::CameraMatrix() const {
  return mat4::LookAt(camera_target_, camera_position_,
                              vec3(0.0f, 1.0f, 0.0f));
}

static const mat4 CalculateAccessoryMatrix(
    const vec2& location, const mat4& character_matrix,
    int num_accessories, const Config& config) {
  // Calculate the accessory offset, in character space.
  // Ensure accessories don't z-fight by rendering them at slightly different z
  // depths. Note that the character_matrix has it's z axis oriented so that
  // it is always pointing towards the camera. Therefore, our z-offset should
  // always be positive here.
  const vec3 offset = vec3(
      location.x() * config.pixel_to_world_scale(),
      location.y() * config.pixel_to_world_scale(),
      config.accessory_z_offset() +
          num_accessories * config.accessory_z_increment());

  // Apply offset to character matrix.
  const mat4 offset_matrix = mat4::FromTranslationVector(offset);
  const mat4 accessory_matrix = character_matrix * offset_matrix;
  return accessory_matrix;
}

static mat4 CalculatePropWorldMatrix(const Prop& prop) {
  const vec3& scale = LoadVec3(&prop.scale());
  const vec3& position = LoadVec3(&prop.position());
  const Angle rotation = Angle::FromDegrees(prop.rotation());
  const Quat quat = Quat::FromAngleAxis(rotation.angle(),
                                        vec3(0.0f, 1.0f, 0.0f));
  const mat4 vertical_orientation_matrix =
      mat4::FromTranslationVector(position) *
      mat4::FromRotationMatrix(quat.ToMatrix()) *
      mat4::FromScaleVector(scale);
  return prop.orientation() == Orientation_Horizontal ?
         vertical_orientation_matrix * kRotate90DegreesAboutXAxis :
         vertical_orientation_matrix;
}

static mat4 CalculateUiArrowOffsetMatrix(
    const vec3& offset, const vec3& scale) {
  // First rotate to horizontal, then scale to correct size, then center and
  // translate forward slightly.
  return mat4::FromTranslationVector(offset) *
         mat4::FromScaleVector(scale) *
         mat4::FromRotationMatrix(
            Quat::FromAngleAxis(kHalfPi, vec3(1.0f, 0.0f, 0.0f)).ToMatrix());
}

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(SceneDescription* scene) const {
  scene->Clear();

  // Camera.
  scene->set_camera(CameraMatrix());

  // Characters and accessories.
  if (config_->draw_characters()) {
    // Constant conversion from character matrix to UI arrow matrix.
    const mat4 ui_arrow_offset_matrix =
        CalculateUiArrowOffsetMatrix(LoadVec3(config_->ui_arrow_offset()),
                                     LoadVec3(config_->ui_arrow_scale()));

    for (auto c = characters_.begin(); c != characters_.end(); ++c) {
      // Render accessories and splatters on the camera-facing side
      // of the character.
      const Angle towards_camera_angle = Angle::FromXZVector(camera_position_ -
                                                       c->position());
      const Angle face_to_camera_angle = c->face_angle() - towards_camera_angle;
      const bool facing_camera = face_to_camera_angle.angle() < 0.0f;

      // Character.
      const WorldTime anim_time = GetAnimationTime(*c);
      const mat4 character_matrix = c->CalculateMatrix(facing_camera);
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
      int num_accessories = 0;
      for (auto it = accessory_indices.begin();
           it != accessory_indices.end(); ++it) {
        const TimelineAccessory& accessory = *timeline->accessories()->Get(*it);
        const vec2 location(accessory.offset().x(), accessory.offset().y());
        scene->renderables().push_back(
            Renderable(accessory.renderable(),
                CalculateAccessoryMatrix(location, character_matrix,
                                         num_accessories, *config_)));
        num_accessories++;
      }

      // Splatter
      unsigned int damage = config_->character_health() - c->health();
      for (unsigned int i = 0;
           i < damage && i < config_->splatter()->Length(); ++i) {
        const Splatter* splatter = config_->splatter()->Get(i);
        const vec2 location(LoadVec2i(splatter->location()));
        scene->renderables().push_back(
            Renderable(splatter->renderable(),
                CalculateAccessoryMatrix(location, character_matrix,
                                         num_accessories, *config_)));
        num_accessories++;
      }

      // UI arrow
      if (config_->draw_ui_arrows()) {
        const mat4 ui_arrow_matrix = character_matrix *
                                             ui_arrow_offset_matrix;
        scene->renderables().push_back(
            Renderable(RenderableId_UiArrow, ui_arrow_matrix));
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
      const mat4 axis_dot = mat4::FromTranslationVector(
          vec3(static_cast<float>(i), 0.0f, 0.0f));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
    for (int i = 0; i < 4; ++i) {
      const mat4 axis_dot = mat4::FromTranslationVector(
          vec3(0.0f, 0.0f, static_cast<float>(i)));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
    for (int i = 0; i < 2; ++i) {
      const mat4 axis_dot = mat4::FromTranslationVector(
          vec3(0.0f, static_cast<float>(i), 0.0f));
      scene->renderables().push_back(
          Renderable(RenderableId_PieSmall, axis_dot));
    }
  }

  // Draw one renderable right in the middle of the world, for debugging.
  // Rotate about z-axis so that it faces the camera.
  if (config_->draw_fixed_renderable() != RenderableId_Invalid) {
    scene->renderables().push_back(
        Renderable(config_->draw_fixed_renderable(),
                   mat4::FromRotationMatrix(Quat::FromAngleAxis(kPi,
                                vec3(0.0f, 1.0f, 0.0f)).ToMatrix())));
  }

  // Lights. Push all lights from configuration file.
  const auto lights = config_->light_positions();
  for (auto it = lights->begin(); it != lights->end(); ++it) {
    const vec3 light_position = LoadVec3(*it);
    scene->lights().push_back(light_position);
  }
}

}  // splat
}  // fpl

