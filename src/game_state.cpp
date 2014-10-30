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
#include "audio_config_generated.h"
#include "audio_engine.h"
#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "common.h"
#include "config_generated.h"
#include "controller.h"
#include "game_state.h"
#include "impel_flatbuffers.h"
#include "impel_processor_overshoot.h"
#include "impel_processor_smooth.h"
#include "impel_util.h"
#include "scene_description.h"
#include "pie_noon_common_generated.h"
#include "timeline_generated.h"
#include "utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace pie_noon {

static const mat4 kRotate90DegreesAboutXAxis(1,  0, 0, 0,
                                             0,  0, 1, 0,
                                             0, -1, 0, 0,
                                             0,  0, 0, 1);

// The data on a pie that just hit a player this frame
struct ReceivedPie {
  CharacterId original_source_id;
  CharacterId source_id;
  CharacterId target_id;
  CharacterHealth damage;
};

struct EventData {
  std::vector<ReceivedPie> received_pies;
  CharacterHealth pie_damage;
};

// Look up a value in a vector based upon pie damage.
template<typename T>
static T EnumerationValueForPieDamage(
    CharacterHealth damage,
    const flatbuffers::Vector<uint16_t> &lookup_vector) {
  const CharacterHealth clamped_damage = mathfu::Clamp<CharacterHealth>(
      damage, 0, lookup_vector.Length() - 1);
  return static_cast<T>(lookup_vector.Get(clamped_damage));
}


GameState::GameState()
    : time_(0),
      characters_(),
      pies_(),
      config_(),
      arrangement_() {
}

GameState::~GameState() {
  // Invalidate all the prop Impellers before their processor is deleted.
  // TODO: processors should invalidate their Impellers when they are destroyed.
  for (size_t i = 0; i < prop_shake_.size(); ++i) {
    prop_shake_[i].Invalidate();
  }
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

// Given some amount of damage happening at a
void GameState::ShakeProps(float damage_percent, const vec3& damage_position) {
  for (size_t i = 0; i < config_->props()->Length(); ++i) {
    const auto prop = config_->props()->Get(i);
    const float shake_scale = prop->shake_scale();
    if (shake_scale == 0.0f)
      continue;

    // We always want to add to the speed, so if the current velocity is
    // negative, we add a negative amount.
    const float current_velocity = prop_shake_[i].Velocity();
    const float current_direction = current_velocity >= 0.0f ? 1.0f : -1.0f;

    // The closer the prop is to the damage_position, the more it should shake.
    // The effect trails off with distance squared.
    const vec3 prop_position = LoadVec3(prop->position());
    const float closeness = mathfu::Clamp(
        config_->prop_shake_identity_distance_sq() /
        (damage_position - prop_position).LengthSquared(), 0.01f, 1.0f);

    // Velocity added is the product of all the factors.
    const float delta_velocity = current_direction * damage_percent *
                                 closeness * shake_scale *
                                 config_->prop_shake_velocity();
    const float new_velocity = current_velocity + delta_velocity;
    prop_shake_[i].SetVelocity(new_velocity);
  }
}

// Returns true if the game is over.
bool GameState::IsGameOver() const {
  switch (config_->game_mode()) {
    case GameMode_Survival: {
      return pies_.size() == 0 &&
          (NumActiveCharacters(true) == 0 || NumActiveCharacters(false) <= 1);
    }
    case GameMode_HighScore: {
      return time_ >= config_->game_time();
    }
    case GameMode_ReachTarget: {
      const CharacterId num_ids = static_cast<CharacterId>(characters_.size());
      for (CharacterId id = 0; id < num_ids; ++id) {
        const auto& character = characters_[id];
        if (character->score() >= config_->target_score()) {
          return true;
        }
      }
      break;
    }
  }
  return false;
}

// Reset the game back to initial configuration.
void GameState::Reset() {
  time_ = 0;
  camera_base_.position = LoadVec3(config_->camera_position());
  camera_base_.target = LoadVec3(config_->camera_target());
  camera_.Initialize(camera_base_, &impel_engine_);
  pies_.clear();
  arrangement_ = GetBestArrangement(config_, characters_.size());

  // Load the impeller specifications. Skip over "None".
  impel::OvershootImpelInit impeller_inits[ImpellerSpecification_Count];
  auto impeller_specifications = config_->impeller_specifications();
  assert(impeller_specifications->Length() == ImpellerSpecification_Count);
  for (int i = ImpellerSpecification_None + 1; i < ImpellerSpecification_Count;
       ++i) {
    auto specification = impeller_specifications->Get(i);
    impel::OvershootInitFromFlatBuffers(*specification, &impeller_inits[i]);
  }

  // Initialize the prop shake Impellers.
  const int num_props = config_->props()->Length();
  prop_shake_.resize(num_props);
  for (int i = 0; i < num_props; ++i) {
    const auto prop = config_->props()->Get(i);
    const ImpellerSpecification impeller_spec = prop->shake_impeller();
    if (impeller_spec == ImpellerSpecification_None)
      continue;

    // Bigger props have a smaller shake scale. We want them to shake more
    // slowly, and with less amplitude.
    const float shake_scale = prop->shake_scale();
    impel::OvershootImpelInit scaled_shake_init =
        impeller_inits[impeller_spec];
    scaled_shake_init.min *= shake_scale;
    scaled_shake_init.max *= shake_scale;
    scaled_shake_init.accel_per_difference *= shake_scale;
    prop_shake_[i].Initialize(scaled_shake_init, &impel_engine_);
  }

  // Reset characters to their initial state.
  const CharacterId num_ids = static_cast<CharacterId>(characters_.size());
  // Initially, everyone targets the character across from themself.
  const unsigned int target_step = num_ids / 2;
  for (CharacterId id = 0; id < num_ids; ++id) {
    CharacterId target_id = (id + target_step) % num_ids;
    characters_[id]->Reset(
        target_id,
        config_->character_health(),
        InitialFaceAngle(arrangement_, id, target_id),
        LoadVec3(arrangement_->character_data()->Get(id)->position()),
        &impel_engine_);
  }
}

WorldTime GameState::GetAnimationTime(const Character& character) const {
  return time_ - character.state_machine()->current_state_start_time();
}

void GameState::ProcessSounds(AudioEngine* audio_engine,
                              const Character& character,
                              WorldTime delta_time) const {
  (void)audio_engine;
  // Process sounds in timeline.
  const Timeline* const timeline = character.CurrentTimeline();
  if (!timeline)
    return;

  const WorldTime anim_time = GetAnimationTime(character);
  const auto sounds = timeline->sounds();
  const int start_index = TimelineIndexAfterTime(sounds, 0, anim_time);
  const int end_index = TimelineIndexAfterTime(sounds, start_index,
                                               anim_time + delta_time);
  for (int i = start_index; i < end_index; ++i) {
    const TimelineSound& timeline_sound = *sounds->Get(i);
    character.PlaySound(timeline_sound.sound());
  }

  // If the character is trying to turn, play the turn sound.
  if (RequestedTurn(character.id())) character.PlaySound(SoundId_Turning);
}

void GameState::CreatePie(CharacterId original_source_id,
                          CharacterId source_id,
                          CharacterId target_id,
                          CharacterHealth damage) {
  float height = config_->pie_arc_height();
  height += config_->pie_arc_height_variance() *
            (mathfu::Random<float>() * 2 - 1);
  int rotations = config_->pie_rotations();
  int variance = config_->pie_rotation_variance();
  rotations += variance ? (rand() % (variance * 2)) - variance : 0;
  pies_.push_back(std::unique_ptr<AirbornePie>(
      new AirbornePie(original_source_id, source_id, target_id, time_,
                      config_->pie_flight_time(), damage, height, rotations)));
  UpdatePiePosition(pies_.back().get());
}

CharacterId GameState::DetermineDeflectionTarget(
    const ReceivedPie& pie) const {
  switch (config_->pie_deflection_mode()) {
    case PieDeflectionMode_ToTargetOfTarget: {
      return characters_[pie.target_id]->target();
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

// Translate movement defined in 'm' into a motion that can be enqueued in the
// GameCamera. Center the movement about 'subject_position'.
static GameCameraMovement CalculateCameraMovement(
    const CameraMovementToSubject& m, const vec3& subject_position,
    const GameCameraState& base) {
  GameCameraMovement movement;
  movement.end.position =
      subject_position * LoadVec3(m.position_from_subject()) +
      base.position * LoadVec3(m.position_from_base());
  movement.end.target =
      subject_position * LoadVec3(m.target_from_subject()) +
      base.target * LoadVec3(m.target_from_base());
  movement.start_velocity = m.start_velocity();
  movement.time = static_cast<float>(m.time());
  SmoothInitFromFlatBuffers(*m.def(), &movement.init);
  return movement;
}

void GameState::ProcessEvent(Character* character,
                             unsigned int event,
                             const EventData& event_data) {
  switch (event) {
    case EventId_TakeDamage: {
      CharacterHealth total_damage = 0;
      for (unsigned int i = 0; i < event_data.received_pies.size(); ++i) {
        const ReceivedPie& pie = event_data.received_pies[i];
        characters_[pie.source_id]->IncrementStat(kHits);
        total_damage += pie.damage;
        if (config_->game_mode() == GameMode_Survival) {
          character->set_health(character->health() - pie.damage);
        }
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_HitByPie,
                         pie.damage, character);
        ApplyScoringRule(config_->scoring_rules(),
                         ScoreEvent_HitSomeoneWithPie,
                         pie.damage, characters_[pie.source_id].get());
        ApplyScoringRule(config_->scoring_rules(),
                         ScoreEvent_YourPieHitSomeone,
                         pie.damage,
                         characters_[pie.original_source_id].get());
      }

      // Shake the nearby props. Amount of shake is a function of damage.
      const float shake_percent = mathfu::Clamp(
          total_damage * config_->prop_shake_percent_per_damage(), 0.0f, 1.0f);
      ShakeProps(shake_percent, character->position());

      // Move the camera.
      if (total_damage >= config_->camera_move_on_damage_min_damage()) {
        camera_.TerminateMovements();
        camera_.QueueMovement(CalculateCameraMovement(
                                  *config_->camera_move_on_damage(),
                                  character->position(), camera_base_));
        camera_.QueueMovement(CalculateCameraMovement(
                                  *config_->camera_move_to_base(),
                                  character->position(), camera_base_));
      }
      break;
    }
    case EventId_ReleasePie: {
      CreatePie(character->id(), character->id(), character->target(),
                character->pie_damage());
      character->IncrementStat(kAttacks);
      ApplyScoringRule(config_->scoring_rules(), ScoreEvent_ThrewPie,
                       character->pie_damage(), character);
      break;
    }
    case EventId_DeflectPie: {
      for (unsigned int i = 0; i < event_data.received_pies.size(); ++i) {
        const ReceivedPie& pie = event_data.received_pies[i];
        character->PlaySound(EnumerationValueForPieDamage<SoundId>(
            pie.damage, *(config_->blocked_sound_id_for_pie_damage())));

        const CharacterHealth deflected_pie_damage =
            pie.damage + config_->pie_damage_change_when_deflected();
        if (deflected_pie_damage > 0) {
          CreatePie(pie.source_id, character->id(),
                    DetermineDeflectionTarget(pie), deflected_pie_damage);
        }
        CreatePieSplatter(*character, 1);
        character->IncrementStat(kBlocks);
        characters_[pie.source_id]->IncrementStat(kMisses);
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_DeflectedPie,
                         character->pie_damage(), character);
      }
    }
    case EventId_LoadPie: {
      character->set_pie_damage(event_data.pie_damage);
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
    ProcessEvent(character, event->event(), *event_data);
  }
}

void GameState::PopulateConditionInputs(ConditionInputs* condition_inputs,
                                        const Character& character) const {
  condition_inputs->is_down = character.controller()->is_down();
  condition_inputs->went_down = character.controller()->went_down();
  condition_inputs->went_up = character.controller()->went_up();
  condition_inputs->animation_time = GetAnimationTime(character);
  condition_inputs->current_time = time_;
}

void GameState::ProcessConditionalEvents(Character* character,
                                         EventData* event_data) {
  auto current_state = character->state_machine()->current_state();
  if (current_state && current_state->conditional_events()) {
    ConditionInputs condition_inputs;
    PopulateConditionInputs(&condition_inputs, *character);

    for (auto it = current_state->conditional_events()->begin();
         it != current_state->conditional_events()->end(); ++it) {
      const ConditionalEvent* conditional_event = *it;
      if (EvaluateCondition(conditional_event->condition(),
                            condition_inputs)) {
        unsigned int event = conditional_event->event();
        event_data->pie_damage = conditional_event->modifier();
        ProcessEvent(character, event, *event_data);
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
  const auto& source = characters_[pie->source()];
  const auto& target = characters_[pie->target()];

  const float time_since_launch =
      static_cast<float>(time_ - pie->start_time());
  float percent = time_since_launch / config_->pie_flight_time();
  percent = mathfu::Clamp(percent, 0.0f, 1.0f);

  Angle pie_angle = -AngleBetweenCharacters(pie->source(), pie->target());

  const Quat pie_orientation = CalculatePieOrientation(
      pie_angle, percent, pie->rotations(), config_);
  const vec3 pie_position = CalculatePiePosition(
      *source.get(), *target.get(), percent, pie->height(), config_);

  pie->set_orientation(pie_orientation);
  pie->set_position(pie_position);
}

uint16_t GameState::CharacterState(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  return characters_[id]->State();
}

static bool CharacterScore(const std::unique_ptr<Character>& a,
                           const std::unique_ptr<Character>& b) {
  return a->score() < b->score();
}

static bool CharacterIsVictorious(
    const std::unique_ptr<Character>& character) {
  return character->victory_state() == kVictorious;
}

void GameState::DetermineWinnersAndLosers() {
  // This code assumes we've verified that the game is over.
  switch (config_->game_mode()) {
    case GameMode_Survival: {
      for (size_t i = 0; i < characters_.size(); ++i) {
        const auto& character = characters_[i];
        if (character->Active()) {
          character->set_victory_state(kVictorious);
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Player %i wins!\n", static_cast<int>(i) + 1);
        } else {
          character->set_victory_state(kFailure);
        }
      }
      break;
    }
    case GameMode_HighScore: {
      if (time_ >= config_->game_time()) {
        const auto it = std::max_element(
            characters_.begin(), characters_.end(), CharacterScore);
        int high_score = (*it)->score();
        for (size_t i = 0; i < characters_.size(); ++i) {
          const auto& character = characters_[i];
          if (character->score() == high_score) {
            character->set_victory_state(kVictorious);
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "Player %i wins!\n", static_cast<int>(i) + 1);
          } else {
            character->set_victory_state(kFailure);
          }
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Final scores:\n");
        for (size_t i = 0; i < characters_.size(); ++i) {
          const auto& character = characters_[i];
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "  Player %i: %i\n", static_cast<int>(i) + 1,
                      character->score());
        }
      }
      break;
    }
    case GameMode_ReachTarget: {
      for (size_t i = 0; i < characters_.size(); ++i) {
        const auto& character = characters_[i];
        if (character->score() >= config_->target_score()) {
          character->set_victory_state(kVictorious);
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Player %i wins!\n", static_cast<int>(i) + 1);
        } else {
          character->set_victory_state(kFailure);
        }
      }
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Final scores:\n");
      for (size_t i = 0; i < characters_.size(); ++i) {
        const auto& character = characters_[i];
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "  Player %i: %i\n", static_cast<int>(i) + 1,
                    character->score());
      }
      break;
    }
  }
  int winner_count = std::count_if(characters_.begin(), characters_.end(),
                                   CharacterIsVictorious);
  for (size_t i = 0; i < characters_.size(); ++i) {
    auto& character = characters_[i];
    switch(winner_count) {
      // If there's no winners at all, everyone draws.
      case 0: {
        character->IncrementStat(kDraws);
        break;
      }
      // If there is only one winner, grant that player a victory.
      case 1: {
        if (character->victory_state() == kVictorious) {
          character->IncrementStat(kWins);
        } else {
          character->IncrementStat(kLosses);
        }
        break;
      }
      // If there's more than one winner, they draw.
      default: {
        if (character->victory_state() == kVictorious) {
          character->IncrementStat(kDraws);
        } else {
          character->IncrementStat(kLosses);
        }
        break;
      }
    }
  }
}

int GameState::NumActiveCharacters(bool human_only) const {
  int num_active = 0;
  for (size_t i = 0; i < characters_.size(); ++i) {
    const auto& character = characters_[i];
    if (character->Active() &&
        (!human_only ||
         character->controller()->controller_type() != Controller::kTypeAI)) {
      num_active++;
    }
  }
  return num_active;
}

// Determine which direction the user wants to turn.
// Returns 0 if no turn requested. 1 if requesting we target the next character
// id. -1 if requesting we target the previous character id.
int GameState::RequestedTurn(CharacterId id) const {
  const auto& character = characters_[id];
  const uint32_t logical_inputs = character->controller()->went_down();
  const int left_jump = arrangement_->character_data()->Get(id)->left_jump();
  const int target_delta =
      (logical_inputs & LogicalInputs_Left) ? left_jump :
      (logical_inputs & LogicalInputs_Right) ? -left_jump : 0;
  return target_delta;
}

CharacterId GameState::CalculateCharacterTarget(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  const auto& character = characters_[id];
  const CharacterId current_target = character->target();

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
  for (CharacterId target_id = current_target + requested_turn; ;
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
  const auto& source = characters_[source_id];
  const auto& target = characters_[target_id];
  const vec3 vector_to_target = target->position() - source->position();
  const Angle angle_to_target = Angle::FromXZVector(vector_to_target);
  return angle_to_target;
}

// Angle to the character's target.
Angle GameState::TargetFaceAngle(CharacterId id) const {
  const auto& character = characters_[id];
  return AngleBetweenCharacters(id, character->target());
}

Angle GameState::TiltTowardsStageFront(const Angle angle) const {
    // Bias characters to face towards the camera.
    vec3 angle_vec = angle.ToXZVector();
    angle_vec.x() *= config_->cardboard_bias_towards_stage_front();
    angle_vec.Normalize();
    Angle result = Angle::FromXZVector(angle_vec);
    return result;
}

// Return true if the character cannot turn left or right.
bool GameState::IsImmobile(CharacterId id) const {
  return CharacterState(id) == StateId_KO || NumActiveCharacters() <= 2;
}

// Calculate if we should fake turning this frame. We fake a turn to ensure
// that we provide feedback to a user's turn request, even if the game state
// forbids turning at this moment.
// Returns 0 if we should not fake a turn. 1 if we should fake turn towards the
// next character id. -1 if we should fake turn towards the previous character
// id.
impel::TwitchDirection GameState::FakeResponseToTurn(CharacterId id) const {
  // We only want to fake the turn response when the character is immobile.
  // If the character can move, we'll just let the move happen normally.
  if (!IsImmobile(id)) return impel::kTwitchDirectionNone;

  // If the user has not requested any movement, then no need to move.
  const int requested_turn = RequestedTurn(id);
  if (requested_turn == 0) return impel::kTwitchDirectionNone;

  return requested_turn > 0 ? impel::kTwitchDirectionPositive :
                              impel::kTwitchDirectionNegative;
}

uint32_t GameState::AllLogicalInputs() const {
  uint32_t inputs = 0;
  for (size_t i = 0; i < characters_.size(); ++i) {
    const auto& character = characters_[i];
    const Controller* controller = character->controller();
    if (controller->controller_type() != Controller::kTypeAI) {
      inputs |= controller->is_down();
    }
  }
  return inputs;
}

// Creates a bunch of particles when a character gets hit by a pie.
void GameState::CreatePieSplatter(const Character& character,
                                  CharacterHealth damage) {
  const ParticleDef * def = config_->pie_splatter_def();
  SpawnParticles(character.position(), def, static_cast<int>(damage) *
                 config_->pie_noon_particles_per_damage());
  // Play a pie hit sound based upon the amount of damage applied (size of the
  // pie).
  character.PlaySound(EnumerationValueForPieDamage<SoundId>(
      damage, *(config_->hit_sound_id_for_pie_damage())));
}

// Spawns a particle at the given position, using a particle definition.
void GameState::SpawnParticles(const mathfu::vec3 &position,
                               const ParticleDef * def,
                               const int particle_count) {
  const vec3 min_scale = LoadVec3(def->min_scale());
  const vec3 max_scale = LoadVec3(def->max_scale());
  const vec3 min_velocity = LoadVec3(def->min_velocity());
  const vec3 max_velocity = LoadVec3(def->max_velocity());
  const vec3 min_angular_velocity = LoadVec3(def->min_angular_velocity());
  const vec3 max_angular_velocity = LoadVec3(def->max_angular_velocity());
  const vec3 min_position_offset = LoadVec3(def->min_position_offset());
  const vec3 max_position_offset = LoadVec3(def->max_position_offset());
  const vec3 min_orientation_offset = LoadVec3(def->min_orientation_offset());
  const vec3 max_orientation_offset = LoadVec3(def->max_orientation_offset());

  for (int i = 0; i<particle_count; i++) {
    Particle * p = particle_manager_.CreateParticle();
    p->set_base_scale(def->preserve_aspect() ?
        vec3(mathfu::RandomInRange(min_scale.x(), max_scale.x())) :
        vec3::RandomInRange(min_scale, max_scale));

    p->set_base_velocity(vec3::RandomInRange(min_velocity, max_velocity));
    p->set_acceleration(LoadVec3(def->acceleration()));
    p->set_renderable_id(def->renderable()->Get(
        mathfu::RandomInRange<int>(0, def->renderable()->size())));
    p->set_base_tint(LoadVec4(def->tint()->Get(
        mathfu::RandomInRange<int>(0, def->tint()->size()))));
    p->set_duration(static_cast<float>(mathfu::RandomInRange<int32_t>(
        def->min_duration(), def->max_duration())));
    p->set_base_position(position + vec3::RandomInRange(min_position_offset,
        max_position_offset));
    p->set_base_orientation(vec3::RandomInRange(min_orientation_offset,
                            max_orientation_offset));
    p->set_rotational_velocity(vec3::RandomInRange(
                               min_angular_velocity,
                               max_angular_velocity));
    p->set_duration_of_shrink_out(
        static_cast<TimeStep>(def->shrink_duration()));
    p->set_duration_of_fade_out(static_cast<TimeStep>(def->fade_duration()));
  }
}

void GameState::AdvanceFrame(WorldTime delta_time, AudioEngine* audio_engine) {
  // Increment the world time counter. This happens at the start of the
  // function so that functions that reference the current world time will
  // include the delta_time. For example, GetAnimationTime needs to compare
  // against the time for *this* frame, not last frame.
  time_ += delta_time;
  if (config_->game_mode() == GameMode_HighScore) {
    int countdown = (config_->game_time() - time_) / kMillisecondsPerSecond;
    if (countdown != countdown_timer_) {
      countdown_timer_ = countdown;
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "Timer remaining: %i\n", countdown_timer_);
    }
  }
  if (NumActiveCharacters(true) == 0) {
    SpawnParticles(mathfu::vec3(0, 10, 0), config_->confetti_def(), 1);
  }

  // Damage is queued up per character then applied during event processing.
  std::vector<EventData> event_data(characters_.size());

  // Update controller to gather state machine inputs.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    auto& character = characters_[i];
    Controller* controller = character->controller();
    const Timeline* timeline =
        character->state_machine()->current_state()->timeline();
    controller->SetLogicalInputs(LogicalInputs_JustHit, false);
    controller->SetLogicalInputs(LogicalInputs_NoHealth,
                                 config_->game_mode() == GameMode_Survival &&
                                 character->health() <= 0);
    controller->SetLogicalInputs(LogicalInputs_AnimationEnd, timeline &&
        (GetAnimationTime(*character.get()) >= timeline->end_time()));
    controller->SetLogicalInputs(LogicalInputs_Won,
                                 character->victory_state() == kVictorious);
    controller->SetLogicalInputs(LogicalInputs_Lost,
                                 character->victory_state() == kFailure);
  }

  // Update all the particles.
  particle_manager_.AdvanceFrame(static_cast<TimeStep>(delta_time));

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end(); ) {
    auto& pie = *it;
    UpdatePiePosition(pie.get());

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - pie->start_time();
    if (time_since_launch >= pie->flight_time()) {
      auto& character = characters_[pie->target()];
      ReceivedPie received_pie = {
        pie->original_source(), pie->source(), pie->target(), pie->damage()
      };
      event_data[pie->target()].received_pies.push_back(received_pie);
      character->controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      if (character->State() != StateId_Blocking)
        CreatePieSplatter(*character, pie->damage());
      it = pies_.erase(it);
    }
    else {
      ++it;
    }
  }

  // Update the character state machines and the facing angles.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    auto& character = characters_[i];

    // Update state machines.
    ConditionInputs condition_inputs;
    PopulateConditionInputs(&condition_inputs, *character.get());
    character->state_machine()->Update(condition_inputs);

    // Update character's target.
    const CharacterId target_id = CalculateCharacterTarget(character->id());
    const Angle target_angle =
        AngleBetweenCharacters(character->id(), target_id);
    const Angle tilted_angle = TiltTowardsStageFront(target_angle);
    character->SetTarget(target_id, tilted_angle);

    // If we're requesting a turn but can't turn, move the face angle
    // anyway to fake a response.
    const impel::TwitchDirection twitch = FakeResponseToTurn(character->id());
    character->TwitchFaceAngle(twitch);
  }

  // Update all Impellers. Impeller updates are done in bulk for scalability.
  impel_engine_.AdvanceFrame(delta_time);

  // Look to timeline to see what's happening. Make it happen.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessEvents(characters_[i].get(), &event_data[i], delta_time);
  }

  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessConditionalEvents(characters_[i].get(), &event_data[i]);
  }

  // Play the sounds that need to be played at this point in time.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessSounds(audio_engine, *characters_[i].get(), delta_time);
  }

  camera_.AdvanceFrame(delta_time);
}

// Get the camera matrix used for rendering.
mat4 GameState::CameraMatrix() const {
  return mat4::LookAt(camera_.Target(), camera_.Position(), mathfu::kAxisY3f);
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
  const mat4 accessory_matrix =
      character_matrix * offset_matrix * scale_matrix;
  return accessory_matrix;
}

static mat4 CalculatePropWorldMatrix(const Prop& prop, Angle shake) {
  const vec3 scale = LoadVec3(prop.scale());
  const vec3 position = LoadVec3(prop.position());
  const Angle rotation = Angle::FromDegrees(prop.rotation());
  const Quat quat = Quat::FromAngleAxis(rotation.ToRadians(),
                                        mathfu::kAxisY3f);
  const vec3 shake_axis = LoadAxis(prop.shake_axis());
  const Quat shake_quat = Quat::FromAngleAxis(shake.ToRadians(), shake_axis);
  const vec3 shake_center(prop.shake_center() == nullptr ? mathfu::kZeros3f :
                          LoadVec3(prop.shake_center()));
  const mat4 vertical_orientation_matrix =
      mat4::FromTranslationVector(position) *
      mat4::FromRotationMatrix(quat.ToMatrix()) *
      mat4::FromTranslationVector(shake_center) *
      mat4::FromRotationMatrix(shake_quat.ToMatrix()) *
      mat4::FromTranslationVector(-shake_center) *
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

  bool operator()(const Character* a, const Character* b) const {
    const float a_dist_sq = (camera_position_ - a->position()).LengthSquared();
    const float b_dist_sq = (camera_position_ - b->position()).LengthSquared();
    return a_dist_sq > b_dist_sq;
  }

private:
  vec3 camera_position_;
};

void GameState::PopulateCharacterAccessories(
    SceneDescription* scene, uint16_t renderable_id,
    const mat4& character_matrix, int num_accessories, CharacterHealth damage,
    CharacterHealth health) const {
  auto renderable = config_->renderables()->Get(renderable_id);

  // Loop twice. First for damage splatters, then for health hearts.
  struct {
    int key;
    vec2i offset;
    const flatbuffers::Vector<flatbuffers::Offset<AccessoryGroup>>*
        indices;
    const flatbuffers::Vector<flatbuffers::Offset<FixedAccessory>>*
        fixed_accessories;
  } accessories[] = {
    {
      damage,
      LoadVec2i(renderable->splatter_offset()),
      config_->splatter_map(),
      config_->splatter_accessories()
    },
    {
      health,
      LoadVec2i(renderable->health_offset()),
      config_->health_map(),
      config_->health_accessories()
    }
  };

  for (size_t j = 0; j < ARRAYSIZE(accessories); ++j) {
    // Get the set of indices into the fixed_accessories array.
    const int max_key = accessories[j].indices->Length() - 1;
    const int key = mathfu::Clamp(accessories[j].key, 0, max_key);
    auto indices = accessories[j].indices->Get(key)->indices();
    const int num_fixed_accessories = static_cast<int>(indices->Length());

    // Add each accessory slightly in front of the character, with a slight
    // z-offset so that they don't z-fight when they overlap, and for a
    // nice parallax look.
    for (int i = 0; i < num_fixed_accessories; ++i) {
      const int index = indices->Get(i);
      const FixedAccessory* accessory =
          accessories[j].fixed_accessories->Get(index);
      const vec2 location(LoadVec2i(accessory->location())
                          + accessories[j].offset);
      const vec2 scale(LoadVec2(accessory->scale()));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(static_cast<uint16_t>(accessory->renderable()),
              CalculateAccessoryMatrix(location, scale, character_matrix,
                                       renderable_id, num_accessories,
                                       *config_))));
      num_accessories++;
    }
  }
}

// Add anything in the list of particles into the scene description:
void GameState::AddParticlesToScene(SceneDescription* scene) const {
  auto plist = particle_manager_.get_particle_list();
  for (auto it = plist.begin(); it != plist.end(); ++it) {
    scene->renderables().push_back(std::unique_ptr<Renderable>(
        new Renderable((*it)->renderable_id(), (*it)->CalculateMatrix(),
                       (*it)->CurrentTint())));
  }
}

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(SceneDescription* scene) const {
  scene->Clear();

  // Camera.
  scene->set_camera(CameraMatrix());

  AddParticlesToScene(scene);

  // Populate scene description with environment items.
  if (config_->draw_props()) {
    auto props = config_->props();
    for (size_t i = 0; i < props->Length(); ++i) {
      const Prop& prop = *props->Get(i);
      const Angle shake(prop_shake_[i].Valid() ?
                        prop_shake_[i].Value() : 0.0f);
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(static_cast<uint16_t>(prop.renderable()),
                         CalculatePropWorldMatrix(prop, shake))));
    }
  }

  // Pies.
  if (config_->draw_pies()) {
    for (auto it = pies_.begin(); it != pies_.end(); ++it) {
      auto& pie = *it;
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(
              EnumerationValueForPieDamage<uint16_t>(
                  pie->damage(), *(config_->renderable_id_for_pie_damage())),
              pie->CalculateMatrix())));
    }
  }

  // Characters and accessories.
  if (config_->draw_characters()) {
    // Sort characters by farthest-to-closest to the camera.
    std::vector<Character*> sorted_characters(characters_.size());
    for (size_t i = 0; i < characters_.size(); ++i) {
      sorted_characters[i] = characters_[i].get();
    }
    const CharacterDepthComparer comparer(camera_.Position());
    std::sort(sorted_characters.begin(), sorted_characters.end(), comparer);

    // Render all parts of the character. Note that order matters here. For
    // example, the arrow appears partially behind the character billboard
    // (because the arrow is flat on the ground) so it has to be rendered first.
    for (size_t i = 0; i < sorted_characters.size(); ++i) {
      Character* character = sorted_characters[i];
      // UI arrow
      if (config_->draw_ui_arrows()) {
        const Angle arrow_angle = TargetFaceAngle(character->id());
        scene->renderables().push_back(std::unique_ptr<Renderable>(
            new Renderable(RenderableId_UiArrow, CalculateUiArrowMatrix(
                character->position(), arrow_angle, *config_))));
      }

      // Render accessories and splatters on the camera-facing side
      // of the character.
      const Angle towards_camera_angle = Angle::FromXZVector(
          camera_.Position() - character->position());
      const Angle face_to_camera_angle =
          character->FaceAngle() - towards_camera_angle;
      const bool facing_camera = face_to_camera_angle.ToRadians() < 0.0f;

      // Character.
      const WorldTime anim_time = GetAnimationTime(*character);
      const uint16_t renderable_id = character->RenderableId(anim_time);
      const mat4 character_matrix = character->CalculateMatrix(facing_camera);
      const vec3 player_color = (character->controller()->controller_type() ==
          Controller::kTypeAI)
          ? LoadVec3(config_->ai_color())
          : LoadVec3(config_->character_colors()->Get(character->id())) /
                     config_->character_global_brightness_factor() +
                     (1 - 1 / config_->character_global_brightness_factor());
      scene->renderables().push_back(
          std::unique_ptr<Renderable>( new Renderable(renderable_id,
          character_matrix,
          mathfu::vec4(player_color.x(), player_color.y(), player_color.z(),
                       1.0))));

      // Accessories.
      int num_accessories = 0;
      const Timeline* const timeline = character->CurrentTimeline();
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
          scene->renderables().push_back(std::unique_ptr<Renderable>(
              new Renderable(
                  accessory.renderable(),
                  CalculateAccessoryMatrix(location, mathfu::kOnes2f,
                                           character_matrix, renderable_id,
                                           num_accessories, *config_))));
          num_accessories++;
        }
      }

      // Splatter and health accessories.
      // First pass through renders splatter accessories.
      // Second pass through renders health accessories.
      const CharacterHealth health =
          config_->game_mode() == GameMode_Survival ? character->health() : 0;
      const CharacterHealth damage =
          config_->character_health() - character->health();
      PopulateCharacterAccessories(scene, renderable_id, character_matrix,
                                   num_accessories, damage, health);

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
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
    for (int i = 0; i < 4; ++i) {
      const mat4 axis_dot = mat4::FromTranslationVector(
          vec3(0.0f, 0.0f, static_cast<float>(i)));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
    for (int i = 0; i < 2; ++i) {
      const mat4 axis_dot = mat4::FromTranslationVector(
          vec3(0.0f, static_cast<float>(i), 0.0f));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
  }

  // Draw one renderable right in the middle of the world, for debugging.
  // Rotate about z-axis so that it faces the camera.
  if (config_->draw_fixed_renderable() != RenderableId_Invalid) {
    scene->renderables().push_back(std::unique_ptr<Renderable>(
        new Renderable(
            static_cast<uint16_t>(config_->draw_fixed_renderable()),
            mat4::FromRotationMatrix(
                Quat::FromAngleAxis(kPi, mathfu::kAxisY3f).ToMatrix()))));
  }

  if (config_->draw_character_lineup()) {
    static const int kFirstRenderableId = RenderableId_CharacterIdle;
    static const int kLastRenderableId = RenderableId_CharacterWin;
    static const int kNumRenderableIds =
        kLastRenderableId - kFirstRenderableId;
    static const float kXSeparation = 2.5f;
    static const float kZSeparation = 0.5f;
    static const float kXOffset = -kXSeparation * 0.5f * kNumRenderableIds;
    static const float kZOffset = 4.0f;

    for (int i = kFirstRenderableId; i <= kLastRenderableId; ++i) {
      // Orient the characters facing the front of the stage in a line.
      const vec3 position(i * kXSeparation + kXOffset, 0.0f,
                          i * kZSeparation + kZOffset);
      const mat4 character_matrix = mat4::FromTranslationVector(position) *
              mat4::FromRotationMatrix(
                  Quat::FromAngleAxis(kPi, mathfu::kAxisY3f).ToMatrix());
      uint16_t renderable_id = static_cast<uint16_t>(i);

      // Draw the characters.
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(renderable_id, character_matrix)));

      // Draw the accessories, if requested.
      if (config_->draw_lineup_accessories()) {
        PopulateCharacterAccessories(
            scene, renderable_id, character_matrix, 0, 10, 10);
      }
    }
  }

  // Lights. Push all lights from configuration file.
  const auto lights = config_->light_positions();
  for (auto it = lights->begin(); it != lights->end(); ++it) {
    const vec3 light_position = LoadVec3(*it);
    scene->lights().push_back(std::unique_ptr<vec3>(new vec3(light_position)));
  }
}

}  // pie_noon
}  // fpl
