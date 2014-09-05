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
#include "audio_config_generated.h"
#include "config_generated.h"
#include "controller.h"
#include "scene_description.h"
#include "utilities.h"
#include "audio_engine.h"

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

// The data on a pie that just hit a player this frame
struct ReceivedPie {
  CharacterId source_id;
  CharacterId target_id;
  int damage;
};

struct EventData {
  std::vector<ReceivedPie> received_pies;
  int pie_damage;
};

GameState::GameState()
    : time_(0),
      camera_position_(mathfu::kZeros3f),
      camera_target_(mathfu::kZeros3f),
      characters_(),
      pies_(),
      config_(),
      arrangement_() {
}

// Calculate the direction a character is facing at the start of the game.
// We want the characters to face their initial target.
static Angle InitialFaceAngle(const CharacterArrangement* arrangement,
                              const CharacterId id,
                              const CharacterId target_id) {
  const vec3 characterPosition =
      LoadVec3(arrangement->character_data()->Get(id)->position());
  const vec3 targetPosition =
      LoadVec3(arrangement->character_data()->Get(target_id)->position());
  return Angle::FromXZVector(targetPosition - characterPosition);
}

// Find the character arrangement that has room for enough characters with the
// least wasted space.
static const CharacterArrangement* GetBestArrangement(const Config* config,
                                                      unsigned int count) {
  unsigned int num_arrangements = config->character_arrangements()->Length();
  const CharacterArrangement* best_arrangement = nullptr;
  unsigned int best_character_slots = std::numeric_limits<unsigned int>::max();
  for (unsigned int i = 0; i < num_arrangements; ++i) {
    const CharacterArrangement* arrangement =
        config->character_arrangements()->Get(i);
    unsigned int character_slots = arrangement->character_data()->Length();
    if (character_slots >= count && character_slots < best_character_slots) {
      best_arrangement = arrangement;
      best_character_slots = character_slots;
    }
  }
  assert(best_arrangement);
  return best_arrangement;
}

// Reset the game back to initial configuration.
void GameState::Reset() {
  time_ = 0;
  camera_position_ = LoadVec3(config_->camera_position());
  camera_target_ = LoadVec3(config_->camera_target());
  pies_.clear();
  arrangement_ = GetBestArrangement(config_, characters_.size());

  // Reset characters to their initial state.
  const CharacterId num_ids = static_cast<CharacterId>(characters_.size());
  // Initially, everyone targets the character across from themself.
  const unsigned int target_step = num_ids / 2;
  for (CharacterId id = 0; id < num_ids; ++id) {
    CharacterId target_id = (id + target_step) % num_ids;
    characters_[id].Reset(
        target_id,
        config_->character_health(),
        InitialFaceAngle(arrangement_, id, target_id),
        LoadVec3(arrangement_->character_data()->Get(id)->position()));
  }
}

WorldTime GameState::GetAnimationTime(const Character& character) const {
  return time_ - character.state_machine()->current_state_start_time();
}

void GameState::ProcessSounds(AudioEngine* audio_engine,
                              Character* character,
                              WorldTime delta_time) const {
  // Process sounds in timeline.
  const Timeline* const timeline = character->CurrentTimeline();
  if (!timeline)
    return;

  const WorldTime anim_time = GetAnimationTime(*character);
  const auto sounds = timeline->sounds();
  const int start_index = TimelineIndexAfterTime(sounds, 0, anim_time);
  const int end_index = TimelineIndexAfterTime(sounds, start_index,
                                               anim_time + delta_time);
  for (int i = start_index; i < end_index; ++i) {
    const TimelineSound& timeline_sound = *sounds->Get(i);
    audio_engine->PlaySoundId(timeline_sound.sound());
  }
}

void GameState::CreatePie(CharacterId source_id,
                          CharacterId target_id,
                          int damage) {
  float height = config_->pie_arc_height();
  height += config_->pie_arc_height_variance() *
            (mathfu::Random<float>() * 2 - 1);
  int rotations = config_->pie_rotations();
  int variance = config_->pie_rotation_variance();
  rotations += variance ? (rand() % (variance * 2)) - variance : 0;
  pies_.push_back(AirbornePie(source_id, target_id,
                              time_, config_->pie_flight_time(),
                              damage, height, rotations));
  UpdatePiePosition(&pies_.back());
}

CharacterId GameState::DetermineDeflectionTarget(const ReceivedPie& pie) const {
  switch (config_->pie_deflection_mode()) {
    case PieDeflectionMode_ToTargetOfTarget: {
      return characters_[pie.target_id].target();
    }
    case PieDeflectionMode_ToSource: {
      return pie.source_id;
    }
    case PieDeflectionMode_ToRandom: {
      return rand() % characters_.size();
    }
    default: {
      assert(0);
      return 0;
    }
  }
}

void GameState::ProcessEvent(Character* character,
                             unsigned int event,
                             EventData* event_data) {
  switch (event) {
    case EventId_TakeDamage: {
      for (unsigned int i = 0; i < event_data->received_pies.size(); ++i) {
        character->set_health(character->health() -
                              event_data->received_pies[i].damage);
        characters_[event_data->received_pies[i].source_id].IncrementStat(kHits);
      }
      break;
    }
    case EventId_ReleasePie: {
      CreatePie(character->id(), character->target(), character->pie_damage());
      character->IncrementStat(kAttacks);
      break;
    }
    case EventId_DeflectPie: {
      for (unsigned int i = 0; i < event_data->received_pies.size(); ++i) {
          ReceivedPie& pie = event_data->received_pies[i];
          CreatePie(character->id(),
                    DetermineDeflectionTarget(pie),
                    pie.damage);
      }
    }
    case EventId_LoadPie: {
      character->set_pie_damage(event_data->pie_damage);
      break;
    }
    default: {
      assert(0);
    }
  }
}

void GameState::ProcessEvents(Character* character,
                              EventData* event_data,
                              WorldTime delta_time) {
  // Process events in timeline.
  const Timeline* const timeline = character->CurrentTimeline();
  if (!timeline)
    return;

  const WorldTime anim_time = GetAnimationTime(*character);
  const auto events = timeline->events();
  const int start_index = TimelineIndexAfterTime(events, 0, anim_time);
  const int end_index = TimelineIndexAfterTime(events, start_index,
                                               anim_time + delta_time);

  for (int i = start_index; i < end_index; ++i) {
    const TimelineEvent* event = events->Get(i);
    event_data->pie_damage = event->modifier();
    ProcessEvent(character, event->event(), event_data);
  }
}

void GameState::PopulateConditionInputs(ConditionInputs* condition_inputs,
                                        const Character* character) const {
  condition_inputs->logical_inputs = character->controller()->logical_inputs();
  condition_inputs->animation_time = GetAnimationTime(*character);
  condition_inputs->current_time = time_;
}

void GameState::ProcessConditionalEvents(Character* character,
                                         EventData* event_data) {
  auto current_state = character->state_machine()->current_state();
  if (current_state && current_state->conditional_events()) {
    ConditionInputs condition_inputs;
    PopulateConditionInputs(&condition_inputs, character);

    for (auto it = current_state->conditional_events()->begin();
         it != current_state->conditional_events()->end(); ++it) {
      const ConditionalEvent* conditional_event = *it;
      if (EvaluateCondition(conditional_event->condition(), condition_inputs)) {
        unsigned int event = conditional_event->event();
        event_data->pie_damage = conditional_event->modifier();
        ProcessEvent(character, event, event_data);
      }
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
  Quat pie_direction = Quat::FromAngleAxis(pie_angle.ToRadians(),
                                           mathfu::kAxisY3f);
  Quat pie_rotation = Quat::FromAngleAxis(rotation_angle.ToRadians(),
                                          mathfu::kAxisZ3f);
  return pie_direction * pie_rotation;
}

static vec3 CalculatePiePosition(const Character& source,
                                 const Character& target,
                                 float percent, float pie_height,
                                 const Config* config) {
  vec3 result = vec3::Lerp(source.position(), target.position(), percent);

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
  return characters_[id].State();
}

// Return the id of the character who has won the game, if such a character
// exists. If no character has won, return -1.
CharacterId GameState::WinningCharacterId() const {
  // Find the id of the one-and-only character who is still active.
  CharacterId win_id = -1;
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    if (it->Active()) {
      if (win_id < 0)
        win_id = it->id();
      else
        return -1; // More than one character still active, so no winner yet.
    }
  }
  return win_id;
}

int GameState::NumActiveCharacters() const {
  int num_active = 0;
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    if (it->Active())
      num_active++;
  }
  return num_active;
}

// Determine which direction the user wants to turn.
// Returns 0 if no turn requested. 1 if requesting we target the next character
// id. -1 if requesting we target the previous character id.
int GameState::RequestedTurn(CharacterId id) const {
  const Character& c = characters_[id];
  const uint32_t logical_inputs = c.controller()->logical_inputs();
  const int left_jump = arrangement_->character_data()->Get(id)->left_jump();
  const int target_delta = (logical_inputs & LogicalInputs_Left) ? left_jump :
                           (logical_inputs & LogicalInputs_Right) ? -left_jump :
                           0;
  return target_delta;
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
  const int requested_turn = RequestedTurn(id);
  if (requested_turn == 0)
    return current_target;

  const CharacterId character_count =
      static_cast<CharacterId>(characters_.size());
  for (CharacterId target_id = current_target + requested_turn;;
       target_id += requested_turn) {
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

Angle GameState::TiltTowardsStageFront(const Angle angle) const
{
    // Bias characters to face towards the camera.
    vec3 angle_vec = angle.ToXZVector();
    angle_vec.x() *= config_->cardboard_bias_towards_stage_front();
    angle_vec.Normalize();
    Angle result = Angle::FromXZVector(angle_vec);
    return result;
}

// Difference between target face angle and current face angle.
Angle GameState::FaceAngleError(CharacterId id) const {
  const Character& c = characters_[id];
  const Angle target_angle = TargetFaceAngle(id);
  const Angle tilted_angle = TiltTowardsStageFront(target_angle);
  const Angle face_angle_error = tilted_angle - c.face_angle();
  return face_angle_error;
}

// Return true if the character cannot turn left or right.
bool GameState::IsImmobile(CharacterId id) const {
  return CharacterState(id) == StateId_KO || NumActiveCharacters() <= 2;
}

// Calculate if we should fake turning this frame. We fake a turn to ensure that
// we provide feedback to a user's turn request, even if the game state forbids
// turning at this moment.
// Returns 0 if we should not fake a turn. 1 if we should fake turn towards the
// next character id. -1 if we should fake turn towards the previous character
// id.
int GameState::FakeResponseToTurn(CharacterId id, Angle delta_angle,
                                  float angular_velocity) const {
  // We only want to fake the turn response when the character is immobile.
  // If the character can move, we'll just let the move happen normally.
  if (!IsImmobile(id))
    return 0;

  // If the user has not requested any movement, then no need to move.
  const int requested_turn = RequestedTurn(id);
  if (requested_turn == 0)
    return 0;

  // Only fake a movement if we're mostly stopped, and already facing the
  // target.
  const float stopped_velocity =
      config_->face_fake_response_angular_velocity() * kDegreesToRadians;
  const Angle stopped_angle =
      Angle::FromDegrees(config_->face_fake_response_angle());
  const bool still_moving = fabs(angular_velocity) > stopped_velocity &&
                            delta_angle.Abs() > stopped_angle;
  if (still_moving)
    return 0;

  // Return 1 or -1, representing the direction we want to turn.
  return requested_turn;
}

// The character's face angle accelerates towards the
float GameState::CalculateCharacterFacingAngleVelocity(
    const Character& c, WorldTime delta_time) const {

  // Calculate the current error in our facing angle.
  const Angle delta_angle = FaceAngleError(c.id());

  // Increment our current face angle velocity.
  const bool wrong_direction =
      c.face_angle_velocity() * delta_angle.ToRadians() < 0.0f;
  const float angular_acceleration =
      delta_angle.ToRadians() * config_->face_delta_to_accel() *
      (wrong_direction ? config_->face_wrong_direction_accel_bonus() : 1.0f);
  const float angular_velocity_unclamped =
      c.face_angle_velocity() + delta_time * angular_acceleration;
  const float max_velocity = config_->face_max_angular_velocity() *
                             kDegreesToRadians;
  const float angular_velocity =
      mathfu::Clamp(angular_velocity_unclamped, -max_velocity, max_velocity);

  // We're close to facing the target, so check to see if we're immobile
  // and requesting motion. If so, give a motion push in the requested
  // direction. This ensures that the character is always respecting turn
  // inputs, which is important for input feedback.
  const int fake_response = FakeResponseToTurn(c.id(), delta_angle,
                                               angular_velocity);
  if (fake_response != 0) {
    const float boost_velocity =
        config_->face_fake_response_boost_angular_velocity() *
        kDegreesToRadians;
    return fake_response < 0 ? -boost_velocity : boost_velocity;
  }

  // If we're far from facing the target, use the velocity calculated above.
  const float snap_velocity =
      config_->face_snap_to_target_angular_velocity() * kDegreesToRadians;
  const Angle snap_angle =
      Angle::FromDegrees(config_->face_snap_to_target_angle());
  const bool snap_to_target = fabs(angular_velocity) <= snap_velocity &&
                              delta_angle.Abs() <= snap_angle;
  if (!snap_to_target)
    return angular_velocity;

  // Set the velocity so that next time through we're exactly on the target
  // angle. Note that if we're already on the target angle, the velocity will
  // be zero.
  return mathfu::Clamp(delta_angle.ToRadians() / delta_time,
                       -snap_velocity, snap_velocity);
}

uint32_t GameState::AllLogicalInputs() const {
  uint32_t inputs = 0;
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    const Controller* controller = it->controller();
    inputs |= controller->logical_inputs();
  }
  return inputs;
}

void GameState::AdvanceFrame(WorldTime delta_time, AudioEngine* audio_engine) {
  // Increment the world time counter. This happens at the start of the function
  // so that functions that reference the current world time will include the
  // delta_time. For example, GetAnimationTime needs to compare against the
  // time for *this* frame, not last frame.
  time_ += delta_time;

  // Damage is queued up per character then applied during event processing.
  std::vector<EventData> event_data(characters_.size());

  // Update controller to gather state machine inputs.
  const CharacterId win_id = WinningCharacterId();
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    Character& character = characters_[i];
    Controller* controller = character.controller();
    const Timeline* timeline =
        character.state_machine()->current_state()->timeline();
    controller->AdvanceFrame(delta_time);
    controller->SetLogicalInputs(LogicalInputs_JustHit, false);
    controller->SetLogicalInputs(LogicalInputs_NoHealth,
                                 character.health() <= 0);
    controller->SetLogicalInputs(LogicalInputs_AnimationEnd,
        timeline && (GetAnimationTime(character) >= timeline->end_time()));
    controller->SetLogicalInputs(LogicalInputs_Winning,
                                 character.id() == win_id);
  }

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end(); ) {
    AirbornePie& pie = *it;
    UpdatePiePosition(&pie);

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - pie.start_time();
    if (time_since_launch >= pie.flight_time()) {
      Character& character = characters_[pie.target()];
      ReceivedPie received_pie = {
        pie.source(), pie.target(), pie.damage()
      };
      event_data[pie.target()].received_pies.push_back(received_pie);
      character.controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      it = pies_.erase(it);
    }
    else {
      ++it;
    }
  }

  // Update the character state machines and the facing angles.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    Character& character = characters_[i];

    // Update state machines.
    ConditionInputs condition_inputs;
    PopulateConditionInputs(&condition_inputs, &character);
    character.state_machine()->Update(condition_inputs);

    // Update target.
    const CharacterId target_id = CalculateCharacterTarget(character.id());
    character.set_target(target_id);

    // Update facing/aiming angles.
    const float face_angle_velocity =
        CalculateCharacterFacingAngleVelocity(character, delta_time);
    const Angle face_angle = Angle::FromWithinThreePi(
        character.face_angle().ToRadians() + delta_time * face_angle_velocity);
    character.set_face_angle_velocity(face_angle_velocity);
    character.set_face_angle(face_angle);
    character.set_aim_angle(face_angle);
  }

  // Look to timeline to see what's happening. Make it happen.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessEvents(&characters_[i], &event_data[i], delta_time);
  }

  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessConditionalEvents(&characters_[i], &event_data[i]);
  }

  // Play the sounds that need to be played at this point in time.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessSounds(audio_engine, &characters_[i], delta_time);
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
  return mat4::LookAt(camera_target_, camera_position_, mathfu::kAxisY3f);
}

static const mat4 CalculateAccessoryMatrix(
    const vec2& location, const vec2& scale, const mat4& character_matrix,
    uint16_t renderable_id, int num_accessories, const Config& config) {
  // Grab the offset of the base renderable. The renderable's texture is moved
  // by this amount, so we have to move the same to match.
  auto renderable = config.renderables()->Get(renderable_id);
  const vec3 renderable_offset = renderable->offset() == nullptr ?
                                 mathfu::kZeros3f :
                                 LoadVec3(renderable->offset());

  // Calculate the accessory offset, in character space.
  // Ensure accessories don't z-fight by rendering them at slightly different z
  // depths. Note that the character_matrix has it's z axis oriented so that
  // it is always pointing towards the camera. Therefore, our z-offset should
  // always be positive here.
  const vec3 accessory_offset = vec3(
      location.x() * config.pixel_to_world_scale(),
      location.y() * config.pixel_to_world_scale(),
      config.accessory_z_offset() +
          num_accessories * config.accessory_z_increment());

  // Apply offset to character matrix.
  const vec3 offset = renderable_offset + accessory_offset;
  const vec3 scale3d(scale[0], scale[1], 1.0f);
  const mat4 offset_matrix = mat4::FromTranslationVector(offset);
  const mat4 scale_matrix = mat4::FromScaleVector(scale3d);
  const mat4 accessory_matrix = character_matrix * offset_matrix * scale_matrix;
  return accessory_matrix;
}

static mat4 CalculatePropWorldMatrix(const Prop& prop) {
  const vec3 scale = LoadVec3(prop.scale());
  const vec3 position = LoadVec3(prop.position());
  const Angle rotation = Angle::FromDegrees(prop.rotation());
  const Quat quat = Quat::FromAngleAxis(rotation.ToRadians(), mathfu::kAxisY3f);
  const mat4 vertical_orientation_matrix =
      mat4::FromTranslationVector(position) *
      mat4::FromRotationMatrix(quat.ToMatrix()) *
      mat4::FromScaleVector(scale);
  return prop.orientation() == Orientation_Horizontal ?
         vertical_orientation_matrix * kRotate90DegreesAboutXAxis :
         vertical_orientation_matrix;
}

static mat4 CalculateUiArrowMatrix(const vec3& position, const Angle& angle,
                                   const Config& config) {
  // First rotate to horizontal, then scale to correct size, then center and
  // translate forward slightly.
  const vec3 offset = LoadVec3(config.ui_arrow_offset());
  const vec3 scale = LoadVec3(config.ui_arrow_scale());
  return mat4::FromTranslationVector(position) *
         mat4::FromRotationMatrix(angle.ToXZRotationMatrix()) *
         mat4::FromTranslationVector(offset) *
         mat4::FromScaleVector(scale) *
         kRotate90DegreesAboutXAxis;
}

// Helper class for std::sort. Sorts Characters by distance from camera.
class CharacterDepthComparer {
public:
  CharacterDepthComparer(const vec3& camera_position) :
    camera_position_(camera_position) {
  }

  bool operator()(const Character& a, const Character& b) const {
    const float a_dist_sq = (camera_position_ - a.position()).LengthSquared();
    const float b_dist_sq = (camera_position_ - b.position()).LengthSquared();
    return a_dist_sq > b_dist_sq;
  }

private:
  vec3 camera_position_;
};

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(SceneDescription* scene) const {
  scene->Clear();

  // Camera.
  scene->set_camera(CameraMatrix());

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

  // Pies.
  if (config_->draw_pies()) {
    for (auto it = pies_.begin(); it != pies_.end(); ++it) {
      const AirbornePie& pie = *it;
      scene->renderables().push_back(
          Renderable(RenderableIdForPieDamage(pie.damage(), *config_),
                     pie.CalculateMatrix()));
    }
  }

  // Characters and accessories.
  if (config_->draw_characters()) {
    // Sort characters by farthest-to-closest to the camera.
    std::vector<Character> sorted_characters(characters_);
    const CharacterDepthComparer comparer(camera_position_);
    std::sort(sorted_characters.begin(), sorted_characters.end(), comparer);

    // Render all parts of the character. Note that order matters here. For
    // example, the arrow appears partially behind the character billboard
    // (because the arrow is flat on the ground) so it has to be rendered first.
    for (auto c = sorted_characters.begin(); c != sorted_characters.end();
         ++c) {
      // UI arrow
      if (config_->draw_ui_arrows()) {
        const Angle arrow_angle = TargetFaceAngle(c->id());
        scene->renderables().push_back(
            Renderable(RenderableId_UiArrow,
                CalculateUiArrowMatrix(c->position(), arrow_angle, *config_)));
      }

      // Render accessories and splatters on the camera-facing side
      // of the character.
      const Angle towards_camera_angle = Angle::FromXZVector(camera_position_ -
                                                       c->position());
      const Angle face_to_camera_angle = c->face_angle() - towards_camera_angle;
      const bool facing_camera = face_to_camera_angle.ToRadians() < 0.0f;

      // Character.
      const WorldTime anim_time = GetAnimationTime(*c);
      const uint16_t renderable_id = c->RenderableId(anim_time);
      const mat4 character_matrix = c->CalculateMatrix(facing_camera);
      scene->renderables().push_back(
          Renderable(renderable_id, character_matrix));

      // Accessories.
      int num_accessories = 0;
      const Timeline* const timeline = c->CurrentTimeline();
      if (timeline) {
        // Get accessories that are valid for the current time.
        const std::vector<int> accessory_indices =
            TimelineIndicesWithTime(timeline->accessories(), anim_time);

        // Create a renderable for each accessory.
        for (auto it = accessory_indices.begin();
             it != accessory_indices.end(); ++it) {
          const TimelineAccessory& accessory =
              *timeline->accessories()->Get(*it);
          const vec2 location(accessory.offset().x(), accessory.offset().y());
          scene->renderables().push_back(
              Renderable(accessory.renderable(),
                  CalculateAccessoryMatrix(location, mathfu::kOnes2f,
                                           character_matrix, renderable_id,
                                           num_accessories, *config_)));
          num_accessories++;
        }
      }

      // Splatter and health accessories.
      // First pass through renders splatter accessories.
      // Second pass through renders health accessories.
      struct {
        const int count;
        const flatbuffers::Vector<flatbuffers::Offset<FixedAccessory>>*
            fixed_accessories;
      } accessories[] = {
        {
          config_->character_health() - c->health(),
          config_->splatter_accessories()
        },
        {
          c->health(),
          config_->health_accessories()
        }
      };

      for (size_t j = 0; j < ARRAYSIZE(accessories); ++j) {
        const int num_fixed_accessories = std::min(
            accessories[j].count,
            static_cast<int>(accessories[j].fixed_accessories->Length()));

        for (int i = 0; i < num_fixed_accessories; ++i) {
          const FixedAccessory* accessory =
              accessories[j].fixed_accessories->Get(i);
          const vec2 location(LoadVec2i(accessory->location()));
          const vec2 scale(LoadVec2(accessory->scale()));
          scene->renderables().push_back(
              Renderable(accessory->renderable(),
                  CalculateAccessoryMatrix(location, scale, character_matrix,
                                           renderable_id, num_accessories,
                                           *config_)));
          num_accessories++;
        }
      }
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
                   mat4::FromRotationMatrix(Quat::FromAngleAxis(
                       kPi, mathfu::kAxisY3f).ToMatrix())));
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

