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
#include "character_state_machine_def_generated.h"
#include "motive/io/flatbuffers.h"
#include "motive/init.h"
#include "motive/util.h"
#include "pie_noon_common_generated.h"
#include "scoring_rules_generated.h"
#include "timeline_generated.h"
#include "utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace pie_noon {

static const float kMaxPosition = 20.0f;

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
      just_joined_game_(false),
      state_machine_(character_state_machine_def),
      victory_state_(kResultUnknown) {
  ResetStats();
}

void Character::Reset(CharacterId target, CharacterHealth health,
                      Angle face_angle, const vec3& position,
                      motive::MotiveEngine* engine) {
  target_ = target;
  health_ = health;
  pie_damage_ = 0;
  position_ = position;
  state_machine_.Reset();
  victory_state_ = kResultUnknown;

  motive::OvershootInit init;
  OvershootInitFromFlatBuffers(*config_->face_angle_def(), &init);

  face_angle_.InitializeWithTarget(init, engine,
                                   motive::Current1f(face_angle.ToRadians()));
}

void Character::SetTarget(CharacterId target, Angle angle_to_target) {
  target_ = target;
  face_angle_.SetTarget(motive::Target1f(angle_to_target.ToRadians(), 0.0f, 1));
}

void Character::TwitchFaceAngle(motive::TwitchDirection twitch) {
  motive::Settled1f settled;
  motive::Settled1fFromFlatBuffers(*config_->face_angle_twitch(), &settled);
  const float velocity = config_->face_angle_twitch_velocity();
  motive::Twitch(twitch, velocity, settled, &face_angle_);
}

uint16_t Character::RenderableId(WorldTime anim_time) const {
  // The timeline for the current state has an array of renderable ids.
  const Timeline* timeline = state_machine_.current_state()->timeline();
  if (!timeline || !timeline->renderables()) return RenderableId_Invalid;

  // Grab the TimelineRenderable for 'anim_time', from the timeline.
  const int renderable_index =
      TimelineIndexBeforeTime(timeline->renderables(), anim_time);
  const TimelineRenderable* renderable =
      timeline->renderables()->Get(renderable_index);
  if (!renderable) return RenderableId_Invalid;

  // Return the renderable id for 'anim_time'.
  return renderable->renderable();
}

mathfu::vec4 Character::Color() const {
  const bool ai = controller_->controller_type() == Controller::kTypeAI;
  const vec3 color = ai ? LoadVec3(config_->ai_color())
                        : Lerp(mathfu::kOnes3f,
                               LoadVec3(config_->character_colors()->Get(id_)),
                               config_->character_global_brightness_factor());
  return vec4(color, 1.0);
}

mathfu::vec4 Character::ButtonColor() const {
  const vec3 color =
      Lerp(mathfu::kOnes3f, LoadVec3(config_->character_colors()->Get(id_)),
           config_->character_global_brightness_factor_buttons());
  return vec4(color, 1.0);
}

void Character::IncrementStat(PlayerStats stat) { player_stats_[stat]++; }

void Character::ResetStats() {
  for (int i = 0; i < kMaxStats; i++) player_stats_[i] = 0;
}

// orientation_ and position_ are set each frame in GameState::Advance.
AirbornePie::AirbornePie(CharacterId original_source, const Character& source,
                         const Character& target, WorldTime start_time,
                         WorldTime flight_time, CharacterHealth original_damage,
                         CharacterHealth damage, float start_height,
                         float peak_height, int rotations,
                         motive::MotiveEngine* engine)
    : original_source_(original_source),
      source_(source.id()),
      target_(target.id()),
      start_time_(start_time),
      flight_time_(flight_time),
      original_damage_(original_damage),
      damage_(damage) {
  // x,z positions are within a reasonable bound.
  // Rotations are anglular values.
  const motive::SmoothInit position_init(
      fpl::Range(-kMaxPosition, kMaxPosition), false);
  const motive::SmoothInit rotation_init(fpl::Range(-kPi, kPi), true);

  // Move x,z at constant speed from source to target.
  const motive::MotiveTarget1f x_target(motive::CurrentToTargetConstVelocity1f(
      source.position().x(), target.position().x(), flight_time));
  const motive::MotiveTarget1f z_target(motive::CurrentToTargetConstVelocity1f(
      source.position().z(), target.position().z(), flight_time));

  // Move y along a trajectory that starts and ends at 'start_height' and
  // tops out at 'peak_height' half way through.
  // Since deceleration is constant, and velocity at the peak is zero,
  // the average velocity from start to peak is,
  //       0.5(start_velocity + 0)
  //
  // At peak, height is average velocity times travel time, so
  //       peak_height = 0.5(start_velocity + 0)*peak_time
  // Which implies,
  //    start_velocity = 2 * delta_height / peak_time
  const float peak_time = 0.5f * flight_time;
  const float delta_height = peak_height - start_height;
  const float start_velocity = 2.0f * delta_height / peak_time;
  const motive::MotiveTarget1f y_target(motive::CurrentToTargetToTarget1f(
      start_height, start_velocity,                  // Initial node.
      peak_height, 0.0f, peak_time,                  // Peak node.
      start_height, -start_velocity, flight_time));  // End node.

  // The pie is rotated about Y a constant amount so that it's facing the
  // target.
  const vec3 vector_to_target = target.position() - source.position();
  const Angle angle_to_target = Angle::FromXZVector(vector_to_target);

  // The pie rotates top to bottom a fixed number of times. Rotation speed
  // is constant.
  const motive::MotiveTarget1f z_rotation_target(
      motive::CurrentToTargetConstVelocity1f(0.0f, rotations * kTwoPi,
                                             flight_time));

  motive::MatrixInit init(5);
  init.AddOp(motive::kTranslateX, position_init, x_target);
  init.AddOp(motive::kTranslateY, position_init, y_target);
  init.AddOp(motive::kTranslateZ, position_init, z_target);
  init.AddOp(motive::kRotateAboutY, -angle_to_target.ToRadians());
  init.AddOp(motive::kRotateAboutZ, rotation_init, z_rotation_target);
  motivator_.Initialize(init, engine);
}

void ApplyScoringRule(const ScoringRules* scoring_rules, ScoreEvent event,
                      unsigned int damage, Character* character) {
  const auto* rule = scoring_rules->rules()->Get(event);
  switch (rule->reward_type()) {
    case RewardType_None: {
      break;
    }
    case RewardType_AddDamage: {
      character->set_score(character->score() + damage);
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i got %i %s!\n",
                  character->id(), damage, damage == 1 ? "point" : "points");
      break;
    }
    case RewardType_SubtractDamage: {
      character->set_score(character->score() - damage);
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i lost %i %s!\n",
                  character->id(), damage, damage == 1 ? "point" : "points");
      break;
    }
    case RewardType_AddPointValue: {
      character->set_score(character->score() + rule->point_value());
      if (rule->point_value()) {
        int points = rule->point_value();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i %s %i %s!\n",
                    character->id(), points > 0 ? "got" : "lost",
                    std::abs(points),
                    std::abs(points) == 1 ? "point" : "points");
      }
      break;
    }
  }
}

}  //  namespace fpl
}  //  namespace pie_noon
