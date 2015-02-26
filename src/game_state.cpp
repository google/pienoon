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

#include <math.h>
#include "precompiled.h"
#include "analytics_tracking.h"
#include "audio_config_generated.h"
#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "common.h"
#include "config_generated.h"
#include "controller.h"
#include "game_state.h"
#include "impel_flatbuffers.h"
#include "impel_init.h"
#include "impel_util.h"
#include "pie_noon_common_generated.h"
#include "pindrop/audio_engine.h"
#include "scene_description.h"
#include "timeline_generated.h"
#include "utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace pie_noon {

static const char* kCategoryGame = "Game";
static const char* kActionStartedGame = "Started game";
static const char* kActionFinishedGameDuration = "Finished game duration";
static const char* kActionFinishedGameHumanWinners =
    "Finished game human winners";
static const char* kActionAiThrewPie = "AI threw pie";
static const char* kActionHumanThrewPie = "Human threw pie";
static const char* kActionHitAi = "Hit Ai";
static const char* kActionHitPlayer = "Hit player";
static const char* kActionAiDeflected = "AI deflected pie";
static const char* kActionPlayerDeflected = "Player deflected pie";
static const char* kLabelKnockOut = "Knock out";
static const char* kLabelDirectHit = "Direct";
static const char* kLabelIndirectHit = "Indirect";
static const char* kLabelHitSelf = "Hit self";
static const char* kLabelHitOther = "Hit other";
static const char* kLabelSize = "Size";
static const char* kLabelSizeDelta = "Size delta";

static const mat4 kRotate90DegreesAboutXAxis(1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0,
                                             0, 0, 0, 0, 1);

// The data on a pie that just hit a player this frame
struct ReceivedPie {
  CharacterId original_source_id;
  CharacterId source_id;
  CharacterId target_id;
  CharacterHealth original_damage;
  CharacterHealth damage;
};

struct EventData {
  std::vector<ReceivedPie> received_pies;
  CharacterHealth pie_damage;
};

// Look up a value in a vector based upon pie damage.
template <typename T>
static T EnumerationValueForPieDamage(
    CharacterHealth damage,
    const flatbuffers::Vector<uint16_t>& lookup_vector) {
  const CharacterHealth clamped_damage =
      mathfu::Clamp<CharacterHealth>(damage, 0, lookup_vector.Length() - 1);
  return static_cast<T>(lookup_vector.Get(clamped_damage));
}

// Factory method for the entity manager, for converting data (in our case.
// flatbuffer definitions) into entities and sticking them into the system.
entity::EntityRef PieNoonEntityFactory::CreateEntityFromData(
    const void* data, entity::EntityManager* entity_manager) {
  const EntityDefinition* def = static_cast<const EntityDefinition*>(data);
  assert(def != nullptr);
  entity::EntityRef entity = entity_manager->AllocateNewEntity();
  for (size_t i = 0; i < def->component_list()->size(); i++) {
    const ComponentDefInstance* currentInstance = def->component_list()->Get(i);
    entity::ComponentInterface* component =
        entity_manager->GetComponent(currentInstance->data_type());
    assert(component != nullptr);
    component->AddFromRawData(entity, currentInstance);
  }
  return entity;
}

GameState::GameState()
    : time_(0),
      config_(nullptr),
      arrangement_(nullptr),
      sceneobject_component_(&impel_engine_) {}

GameState::~GameState() {}

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

// All shakeable props are tracked and handled by the shakeable prop component.
void GameState::ShakeProps(float damage_percent, const vec3& damage_position) {
  shakeable_prop_component_.ShakeProps(damage_percent, damage_position);

  for (auto iter = shakeable_prop_component_.begin();
       iter != shakeable_prop_component_.end(); ++iter) {
    const SceneObjectData* so_data =
        entity_manager_.GetComponentData<SceneObjectData>(iter->entity);
    assert(so_data != nullptr);
    float dist_squared =
        (so_data->GlobalPosition() - damage_position).LengthSquared();
    if (dist_squared < config_->splatter_radius_squared()) {
      AddSplatterToProp(iter->entity);
    }
  }
}

// Returns true if the game is over.
bool GameState::IsGameOver() const {
  switch (config_->game_mode()) {
    case GameMode_Survival: {
      return pies_.size() == 0 && (NumActiveCharacters(true) == 0 ||
                                   NumActiveCharacters(false) <= 1);
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
void GameState::Reset(AnalyticsMode analytics_mode) {
  time_ = 0;
  camera_base_.position = LoadVec3(config_->camera_position());
  camera_base_.target = LoadVec3(config_->camera_target());
  camera_.Initialize(camera_base_, &impel_engine_);
  pies_.clear();
  arrangement_ = GetBestArrangement(config_, characters_.size());
  analytics_mode_ = analytics_mode;

  entity_manager_.Clear();
  entity_manager_.RegisterComponent<SceneObjectComponent>(
      &sceneobject_component_);
  entity_manager_.RegisterComponent<ShakeablePropComponent>(
      &shakeable_prop_component_);
  entity_manager_.RegisterComponent<DripAndVanishComponent>(
      &drip_and_vanish_component_);
  entity_manager_.RegisterComponent<PlayerCharacterComponent>(
      &player_character_component_);

  // Shakable Prop Component needs to know about some of our structures:
  shakeable_prop_component_.set_impel_engine(&impel_engine_);
  shakeable_prop_component_.set_config(config_);
  shakeable_prop_component_.LoadImpellerSpecs();
  player_character_component_.set_config(config_);

  entity_manager_.set_entity_factory(&pie_noon_entity_factory_);
  player_character_component_.set_gamestate_ptr(this);
  // Load Entities from flatbuffer!
  for (size_t i = 0; i < config_->entity_list()->size(); i++) {
    entity_manager_.CreateEntityFromData(config_->entity_list()->Get(i));
  }

  // Reset characters to their initial state.
  const CharacterId num_ids = static_cast<CharacterId>(characters_.size());
  // Initially, everyone targets the character across from themself.
  const unsigned int target_step = num_ids / 2;
  for (CharacterId id = 0; id < num_ids; ++id) {
    CharacterId target_id = (id + target_step) % num_ids;
    characters_[id]->Reset(
        target_id, config_->character_health(),
        InitialFaceAngle(arrangement_, id, target_id),
        LoadVec3(arrangement_->character_data()->Get(id)->position()),
        &impel_engine_);
  }

  // Create player character entities:
  for (CharacterId id = 0; id < static_cast<CharacterId>(characters_.size());
       ++id) {
    entity::EntityRef entity = entity_manager_.AllocateNewEntity();
    PlayerCharacterData* pc_data =
        player_character_component_.AddEntity(entity);
    pc_data->character_id = id;
  }

  particle_manager_.RemoveAllParticles();
}

// Sets up the players in joining mode, where all they can do is jump up
// and down.
void GameState::EnterJoiningMode() {
  Reset(kNoAnalytics);
  for (CharacterId id = 0; id < static_cast<CharacterId>(characters_.size());
       ++id) {
    characters_[id]->state_machine()->SetCurrentState(StateId_Joining, time_);
  }
}

WorldTime GameState::GetAnimationTime(const Character& character) const {
  return time_ - character.state_machine()->current_state_start_time();
}

void GameState::ProcessSounds(pindrop::AudioEngine* audio_engine,
                              const Character& character,
                              WorldTime delta_time) const {
  // Process sounds in timeline.
  const Timeline* const timeline = character.CurrentTimeline();
  if (!timeline) return;

  const WorldTime anim_time = GetAnimationTime(character);
  const auto sounds = timeline->sounds();
  const int start_index = TimelineIndexAfterTime(sounds, 0, anim_time);
  const int end_index =
      TimelineIndexAfterTime(sounds, start_index, anim_time + delta_time);
  for (int i = start_index; i < end_index; ++i) {
    const TimelineSound& timeline_sound = *sounds->Get(i);
    audio_engine->PlaySound(timeline_sound.sound()->c_str());
  }

  // If the character is trying to turn, play the turn sound.
  if (RequestedTurn(character.id())) {
    audio_engine->PlaySound("Turning");
  }
}

static float CalculatePieHeight(const Config& config) {
  return config.pie_arc_height() + config.pie_arc_height_variance() *
                                   (mathfu::Random<float>() * 2 - 1);
}

static float CalculatePieRotations(const Config& config) {
  const int variance = config.pie_rotation_variance();
  const int bonus = variance == 0 ? 0 : (rand() % (variance * 2)) - variance;
  return config.pie_rotations() + bonus;
}

void GameState::CreatePie(CharacterId original_source_id, CharacterId source_id,
                          CharacterId target_id,
                          CharacterHealth original_damage,
                          CharacterHealth damage) {
  const float peak_height = CalculatePieHeight(*config_);
  const int rotations = CalculatePieRotations(*config_);
  pies_.push_back(std::unique_ptr<AirbornePie>(new AirbornePie(
      original_source_id, *characters_[source_id], *characters_[target_id],
      time_, config_->pie_flight_time(), original_damage, damage,
      config_->pie_initial_height(), peak_height, rotations, &impel_engine_)));
}

CharacterId GameState::DetermineDeflectionTarget(const ReceivedPie& pie) const {
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
  movement.end.target = subject_position * LoadVec3(m.target_from_subject()) +
                        base.target * LoadVec3(m.target_from_base());
  movement.start_velocity = m.start_velocity();
  movement.time = static_cast<float>(m.time());
  SmoothInitFromFlatBuffers(*m.def(), &movement.init);
  return movement;
}

void GameState::ProcessEvent(pindrop::AudioEngine* audio_engine,
                             Character* character, unsigned int event,
                             const EventData& event_data) {
  bool is_ai_player =
      character->controller()->controller_type() == Controller::kTypeAI;
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
        if (analytics_mode_ == kTrackAnalytics) {
          bool hit_self = pie.original_source_id == pie.target_id;
          bool direct = pie.original_damage == pie.damage;
          const char* action = is_ai_player ? kActionHitAi : kActionHitPlayer;
          SendTrackerEvent(kCategoryGame, action,
                           hit_self ? kLabelHitSelf : kLabelHitOther,
                           pie.damage);
          SendTrackerEvent(kCategoryGame, action,
                           direct ? kLabelDirectHit : kLabelIndirectHit,
                           pie.damage);
          SendTrackerEvent(kCategoryGame, action, kLabelSizeDelta,
                           pie.original_damage - pie.damage);
          if (character->health() <= 0) {
            SendTrackerEvent(kCategoryGame, action, kLabelKnockOut, time_);
          }
        }
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_HitByPie,
                         pie.damage, character);
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_HitSomeoneWithPie,
                         pie.damage, characters_[pie.source_id].get());
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_YourPieHitSomeone,
                         pie.damage, characters_[pie.original_source_id].get());
      }

      // Shake the nearby props. Amount of shake is a function of damage.
      const float shake_percent = mathfu::Clamp(
          total_damage * config_->prop_shake_percent_per_damage(), 0.0f, 1.0f);
      ShakeProps(shake_percent, character->position());

      // Move the camera.
      if (total_damage >= config_->camera_move_on_damage_min_damage()) {
        camera_.TerminateMovements();
        camera_.QueueMovement(
            CalculateCameraMovement(*config_->camera_move_on_damage(),
                                    character->position(), camera_base_));
        camera_.QueueMovement(
            CalculateCameraMovement(*config_->camera_move_to_base(),
                                    character->position(), camera_base_));
      }
      break;
    }
    case EventId_ReleasePie: {
      CreatePie(character->id(), character->id(), character->target(),
                character->pie_damage(), character->pie_damage());
      character->IncrementStat(kAttacks);
      if (analytics_mode_ == kTrackAnalytics) {
        const char* action =
            is_ai_player ? kActionAiThrewPie : kActionHumanThrewPie;
        SendTrackerEvent(kCategoryGame, action, kLabelSize,
                         character->pie_damage());
      }
      ApplyScoringRule(config_->scoring_rules(), ScoreEvent_ThrewPie,
                       character->pie_damage(), character);
      break;
    }
    case EventId_DeflectPie: {
      for (unsigned int i = 0; i < event_data.received_pies.size(); ++i) {
        const ReceivedPie& pie = event_data.received_pies[i];

        const CharacterHealth index = mathfu::Clamp<CharacterHealth>(
            pie.damage, 0,
            config_->blocked_sound_id_for_pie_damage()->Length() - 1);
        const auto& sound_name =
            config_->blocked_sound_id_for_pie_damage()->Get(index);
        audio_engine->PlaySound(sound_name->c_str());

        const CharacterHealth deflected_pie_damage =
            pie.damage + config_->pie_damage_change_when_deflected();
        if (deflected_pie_damage > 0) {
          CreatePie(pie.source_id, character->id(),
                    DetermineDeflectionTarget(pie), pie.original_damage,
                    deflected_pie_damage);
        }
        CreatePieSplatter(audio_engine, *character, 1);
        character->IncrementStat(kBlocks);
        characters_[pie.source_id]->IncrementStat(kMisses);
        if (analytics_mode_ == kTrackAnalytics) {
          const char* action =
              is_ai_player ? kActionAiDeflected : kActionPlayerDeflected;
          SendTrackerEvent(kCategoryGame, action, kLabelSize, pie.damage);
        }
        ApplyScoringRule(config_->scoring_rules(), ScoreEvent_DeflectedPie,
                         character->pie_damage(), character);
      }
    }
    case EventId_LoadPie: {
      character->set_pie_damage(event_data.pie_damage);
      break;
    }
    case EventId_JumpWhileJoining: {
      CreateJoinConfettiBurst(*character);
      break;
    }
    default: { assert(0); }
  }
}

void GameState::AddSplatterToProp(entity::EntityRef prop) {
  static RenderableId id_list[] = {
      RenderableId_Splatter1, RenderableId_Splatter2, RenderableId_Splatter3};
  if (prop->IsRegisteredForComponent(ComponentDataUnion_SceneObjectDef)) {
    entity::EntityRef splatter =
        entity_manager_.CreateEntityFromData(config_->splatter_def());
    auto so_data = entity_manager_.GetComponentData<SceneObjectData>(splatter);

    so_data->set_renderable_id(id_list[mathfu::RandomInRange(0, 3)]);
    so_data->set_parent(prop);

    vec3 min_range = LoadVec3(config_->splatter_range_min());
    vec3 max_range = LoadVec3(config_->splatter_range_max());

    const vec3 offset = mathfu::RandomInRange(min_range, max_range);
    so_data->SetTranslation(offset);

    const Angle rotation_angle =
        Angle::FromWithinThreePi(mathfu::RandomInRange(-M_PI_2, M_PI_2));
    so_data->SetRotationAboutZ(rotation_angle.ToRadians());

    float scale = mathfu::RandomInRange(config_->splatter_scale_min(),
                                        config_->splatter_scale_max());
    so_data->SetScale(vec3(scale));

    drip_and_vanish_component_.SetStartingValues(splatter);
  }
}

void GameState::ProcessEvents(pindrop::AudioEngine* audio_engine,
                              Character* character, EventData* event_data,
                              WorldTime delta_time) {
  // Process events in timeline.
  const Timeline* const timeline = character->CurrentTimeline();
  if (!timeline) return;

  const WorldTime anim_time = GetAnimationTime(*character);
  const auto events = timeline->events();
  const int start_index = TimelineIndexAfterTime(events, 0, anim_time);
  const int end_index =
      TimelineIndexAfterTime(events, start_index, anim_time + delta_time);

  for (int i = start_index; i < end_index; ++i) {
    const TimelineEvent* event = events->Get(i);
    event_data->pie_damage = event->modifier();
    ProcessEvent(audio_engine, character, event->event(), *event_data);
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

void GameState::ProcessConditionalEvents(pindrop::AudioEngine* audio_engine,
                                         Character* character,
                                         EventData* event_data) {
  auto current_state = character->state_machine()->current_state();
  if (current_state && current_state->conditional_events()) {
    ConditionInputs condition_inputs;
    PopulateConditionInputs(&condition_inputs, *character);

    for (auto it = current_state->conditional_events()->begin();
         it != current_state->conditional_events()->end(); ++it) {
      const ConditionalEvent* conditional_event = *it;
      if (EvaluateCondition(conditional_event->condition(), condition_inputs)) {
        unsigned int event = conditional_event->event();
        event_data->pie_damage = conditional_event->modifier();
        ProcessEvent(audio_engine, character, event, *event_data);
      }
    }
  }
}











uint16_t GameState::CharacterState(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  return characters_[id]->State();
}

static bool CharacterScore(const std::unique_ptr<Character>& a,
                           const std::unique_ptr<Character>& b) {
  return a->score() < b->score();
}

static bool CharacterIsVictorious(const std::unique_ptr<Character>& character) {
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
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i wins!\n",
                      static_cast<int>(i) + 1);
        } else {
          character->set_victory_state(kFailure);
        }
      }
      break;
    }
    case GameMode_HighScore: {
      if (time_ >= config_->game_time()) {
        const auto it = std::max_element(characters_.begin(), characters_.end(),
                                         CharacterScore);
        int high_score = (*it)->score();
        for (size_t i = 0; i < characters_.size(); ++i) {
          const auto& character = characters_[i];
          if (character->score() == high_score) {
            character->set_victory_state(kVictorious);
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i wins!\n",
                        static_cast<int>(i) + 1);
          } else {
            character->set_victory_state(kFailure);
          }
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Final scores:\n");
        for (size_t i = 0; i < characters_.size(); ++i) {
          const auto& character = characters_[i];
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "  Player %i: %i\n",
                      static_cast<int>(i) + 1, character->score());
        }
      }
      break;
    }
    case GameMode_ReachTarget: {
      for (size_t i = 0; i < characters_.size(); ++i) {
        const auto& character = characters_[i];
        if (character->score() >= config_->target_score()) {
          character->set_victory_state(kVictorious);
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Player %i wins!\n",
                      static_cast<int>(i) + 1);
        } else {
          character->set_victory_state(kFailure);
        }
      }
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Final scores:\n");
      for (size_t i = 0; i < characters_.size(); ++i) {
        const auto& character = characters_[i];
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "  Player %i: %i\n",
                    static_cast<int>(i) + 1, character->score());
      }
      break;
    }
  }
  int winner_count = std::count_if(characters_.begin(), characters_.end(),
                                   CharacterIsVictorious);
  for (size_t i = 0; i < characters_.size(); ++i) {
    auto& character = characters_[i];
    switch (winner_count) {
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
      (logical_inputs & LogicalInputs_Left)
          ? left_jump
          : (logical_inputs & LogicalInputs_Right) ? -left_jump : 0;
  return target_delta;
}

CharacterId GameState::CalculateCharacterTarget(CharacterId id) const {
  assert(0 <= id && id < static_cast<CharacterId>(characters_.size()));
  const auto& character = characters_[id];
  const CharacterId current_target = character->target();

  // If you yourself are KO'd, then you can't change target.
  const int target_state = CharacterState(id);
  if (target_state == StateId_KO) return current_target;

  // Check the inputs to see how requests for target change.
  const int requested_turn = RequestedTurn(id);
  if (requested_turn == 0) return current_target;

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
    if (target_id == current_target) return current_target;

    // Avoid targeting yourself.
    // Avoid looping around to the other side.
    if (target_id == id) return current_target;

    // Don't target KO'd characters.
    const int target_state = CharacterState(target_id);
    if (target_state == StateId_KO) continue;

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

  return requested_turn > 0 ? impel::kTwitchDirectionPositive
                            : impel::kTwitchDirectionNegative;
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
void GameState::CreatePieSplatter(pindrop::AudioEngine* audio_engine,
                                  const Character& character,
                                  CharacterHealth damage) {
  const ParticleDef* def = config_->pie_splatter_def();
  SpawnParticles(
      character.position(), def,
      static_cast<int>(damage) * config_->pie_noon_particles_per_damage());
  // Play a pie hit sound based upon the amount of damage applied (size of the
  // pie).
  const CharacterHealth index = mathfu::Clamp<CharacterHealth>(
      damage, 0, config_->hit_sound_id_for_pie_damage()->Length() - 1);
  const auto& sound_name = config_->hit_sound_id_for_pie_damage()->Get(index);
  audio_engine->PlaySound(sound_name->c_str());
}

// Creates confetti when a character presses buttons on the join screen.
void GameState::CreateJoinConfettiBurst(const Character& character) {
  const ParticleDef* def = config_->joining_confetti_def();
  vec3 character_color =
      LoadVec3(config_->character_colors()->Get(character.id()));

  SpawnParticles(
      character.position(), def, config_->joining_confetti_count(),
      vec4(character_color.x(), character_color.y(), character_color.z(), 1));
}

// Spawns a particle at the given position, using a particle definition.
void GameState::SpawnParticles(const mathfu::vec3& position,
                               const ParticleDef* def, const int particle_count,
                               const mathfu::vec4& base_tint) {
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

  for (int i = 0; i < particle_count; i++) {
    Particle* p = particle_manager_.CreateParticle();
    // if we got back a null, it means new particles can't be spawned right now.
    if (p == nullptr) {
      break;
    }
    p->set_base_scale(
        def->preserve_aspect()
            ? vec3(mathfu::RandomInRange(min_scale.x(), max_scale.x()))
            : vec3::RandomInRange(min_scale, max_scale));

    p->set_base_velocity(vec3::RandomInRange(min_velocity, max_velocity));
    p->set_acceleration(LoadVec3(def->acceleration()));
    p->set_renderable_id(def->renderable()->Get(
        mathfu::RandomInRange<int>(0, def->renderable()->size())));
    mathfu::vec4 tint = LoadVec4(
        def->tint()->Get(mathfu::RandomInRange<int>(0, def->tint()->size())));
    p->set_base_tint(
        mathfu::vec4(tint.x() * base_tint.x(), tint.y() * base_tint.y(),
                     tint.z() * base_tint.z(), tint.w() * base_tint.w()));
    p->set_duration(static_cast<float>(mathfu::RandomInRange<int32_t>(
        def->min_duration(), def->max_duration())));
    p->set_base_position(position + vec3::RandomInRange(min_position_offset,
                                                        max_position_offset));
    p->set_base_orientation(
        vec3::RandomInRange(min_orientation_offset, max_orientation_offset));
    p->set_rotational_velocity(
        vec3::RandomInRange(min_angular_velocity, max_angular_velocity));
    p->set_duration_of_shrink_out(
        static_cast<TimeStep>(def->shrink_duration()));
    p->set_duration_of_fade_out(static_cast<TimeStep>(def->fade_duration()));
  }
}

void GameState::AdvanceFrame(WorldTime delta_time,
                             pindrop::AudioEngine* audio_engine) {
  // Increment the world time counter. This happens at the start of the
  // function so that functions that reference the current world time will
  // include the delta_time. For example, GetAnimationTime needs to compare
  // against the time for *this* frame, not last frame.
  time_ += delta_time;
  if (config_->game_mode() == GameMode_HighScore) {
    int countdown = (config_->game_time() - time_) / kMillisecondsPerSecond;
    if (countdown != countdown_timer_) {
      countdown_timer_ = countdown;
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Timer remaining: %i\n",
                  countdown_timer_);
    }
  }
  if (NumActiveCharacters(true) == 0) {
    SpawnParticles(mathfu::vec3(0, 10, 0), config_->confetti_def(), 1);
  }

  // Damage is queued up per character then applied during event processing.
  std::vector<EventData> event_data(characters_.size());

  // Update controller to gather state machine inputs.
  for (size_t i = 0; i < characters_.size(); ++i) {
    auto& character = characters_[i];
    character->UpdatePreviousState();
    Controller* controller = character->controller();
    const Timeline* timeline =
        character->state_machine()->current_state()->timeline();
    controller->SetLogicalInputs(LogicalInputs_JustHit, false);
    controller->SetLogicalInputs(
        LogicalInputs_NoHealth,
        config_->game_mode() == GameMode_Survival && character->health() <= 0);
    controller->SetLogicalInputs(
        LogicalInputs_AnimationEnd,
        timeline &&
            (GetAnimationTime(*character.get()) >= timeline->end_time()));
    controller->SetLogicalInputs(LogicalInputs_Won,
                                 character->victory_state() == kVictorious);
    controller->SetLogicalInputs(LogicalInputs_Lost,
                                 character->victory_state() == kFailure);

    bool just_joined = character->just_joined_game();
    controller->SetLogicalInputs(LogicalInputs_JoinedGame, just_joined);
    if (just_joined && character->State() != StateId_Joining) {
      character->set_just_joined_game(false);
    }
  }

  // Update all the particles.
  particle_manager_.AdvanceFrame(static_cast<TimeStep>(delta_time));

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end();) {
    auto& pie = *it;

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - pie->start_time();
    if (time_since_launch >= pie->flight_time()) {
      auto& character = characters_[pie->target()];
      ReceivedPie received_pie = {pie->original_source(), pie->source(),
                                  pie->target(), pie->original_damage(),
                                  pie->damage()};
      event_data[pie->target()].received_pies.push_back(received_pie);
      character->controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      if (character->State() != StateId_Blocking)
        CreatePieSplatter(audio_engine, *character, pie->damage());
      it = pies_.erase(it);
    } else {
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

  // Look to timeline to see what's happening. Make it happen.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessEvents(audio_engine, characters_[i].get(), &event_data[i],
                  delta_time);
  }

  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessConditionalEvents(audio_engine, characters_[i].get(),
                             &event_data[i]);
  }

  // Play the sounds that need to be played at this point in time.
  for (unsigned int i = 0; i < characters_.size(); ++i) {
    ProcessSounds(audio_engine, *characters_[i].get(), delta_time);
  }

  // Update entities.
  entity_manager_.UpdateComponents(delta_time);

  // Update all Impellers. Impeller updates are done in bulk for scalability.
  // Must come after entity_manager_'s update because matrix Impellers are
  // modified by Components.
  impel_engine_.AdvanceFrame(delta_time);

  camera_.AdvanceFrame(delta_time);
}

void GameState::PreGameLogging() const {
  SendTrackerEvent(kCategoryGame, kActionStartedGame,
                   EnumNameGameMode(config_->game_mode()),
                   NumActiveCharacters(true));
}

void GameState::PostGameLogging() const {
  SendTrackerEvent(kCategoryGame, kActionFinishedGameDuration,
                   EnumNameGameMode(config_->game_mode()), time());
  SendTrackerEvent(kCategoryGame, kActionFinishedGameHumanWinners,
                   EnumNameGameMode(config_->game_mode()),
                   NumActiveCharacters(true));
}

// Get the camera matrix used for rendering.
mat4 GameState::CameraMatrix() const {
  return mat4::LookAt(camera_.Target(), camera_.Position(), mathfu::kAxisY3f);
}

// Helper class for std::sort. Sorts Characters by distance from camera.
class CharacterDepthComparer {
 public:
  CharacterDepthComparer(const vec3& camera_position)
      : camera_position_(new vec3(camera_position)) {}

  CharacterDepthComparer(const CharacterDepthComparer& comparer)
      : camera_position_(new vec3(*comparer.camera_position_)) {}

  bool operator()(const Character* a, const Character* b) const {
    const float a_dist_sq = (*camera_position_ - a->position()).LengthSquared();
    const float b_dist_sq = (*camera_position_ - b->position()).LengthSquared();
    return a_dist_sq > b_dist_sq;
  }

 private:
  // Pointer to force alignment, which MSVC std::sort ignores
  std::unique_ptr<vec3> camera_position_;
};

// Add anything in the list of particles into the scene description:
void GameState::AddParticlesToScene(SceneDescription* scene) const {
  auto plist = particle_manager_.get_particle_list();
  for (auto it = plist.begin(); it != plist.end(); ++it) {
    scene->renderables().push_back(std::unique_ptr<Renderable>(
        new Renderable((*it)->renderable_id(), (*it)->CalculateMatrix(),
                       (*it)->CurrentTint())));
  }
}

void GameState::PopulateScene(SceneDescription* scene) {
  scene->Clear();
  // Camera.
  scene->set_camera(CameraMatrix());
  AddParticlesToScene(scene);
  sceneobject_component_.PopulateScene(scene);

  // Add all lights from configuration file to the scene.
  // Important note: The renderer will break if there isn't at least one
  // light in the scene.
  const auto lights = config_->light_positions();
  for (auto it = lights->begin(); it != lights->end(); ++it) {
    const vec3 light_position = LoadVec3(*it);
    scene->lights().push_back(std::unique_ptr<vec3>(new vec3(light_position)));
  }

  // Pies.
  if (config_->draw_pies()) {
    for (auto it = pies_.begin(); it != pies_.end(); ++it) {
      auto& pie = *it;
      scene->renderables().push_back(std::unique_ptr<Renderable>(new Renderable(
          EnumerationValueForPieDamage<uint16_t>(
              pie->damage(), *(config_->renderable_id_for_pie_damage())),
          pie->Matrix())));
    }
  }

  // Axes. Useful for debugging.
  // Positive x axis is long. Positive z axis is short.
  // Positive y axis is shortest.
  if (config_->draw_axes()) {
    // TODO: add an arrow renderable instead of drawing with pies.
    for (int i = 0; i < 8; ++i) {
      const mat4 axis_dot =
          mat4::FromTranslationVector(vec3(static_cast<float>(i), 0.0f, 0.0f));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
    for (int i = 0; i < 4; ++i) {
      const mat4 axis_dot =
          mat4::FromTranslationVector(vec3(0.0f, 0.0f, static_cast<float>(i)));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
    for (int i = 0; i < 2; ++i) {
      const mat4 axis_dot =
          mat4::FromTranslationVector(vec3(0.0f, static_cast<float>(i), 0.0f));
      scene->renderables().push_back(std::unique_ptr<Renderable>(
          new Renderable(RenderableId_PieSmall, axis_dot)));
    }
  }

  // Draw one renderable right in the middle of the world, for debugging.
  // Rotate about z-axis so that it faces the camera.
  if (config_->draw_fixed_renderable() != RenderableId_Invalid) {
    scene->renderables().push_back(std::unique_ptr<Renderable>(new Renderable(
        static_cast<uint16_t>(config_->draw_fixed_renderable()),
        mat4::FromRotationMatrix(
            Quat::FromAngleAxis(kPi, mathfu::kAxisY3f).ToMatrix()))));
  }
}

}  // pie_noon
}  // fpl
