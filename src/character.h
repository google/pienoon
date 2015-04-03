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

#ifndef PIE_NOON_CHARACTER_H_
#define PIE_NOON_CHARACTER_H_

#include "motive/math/angle.h"
#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "config_generated.h"
#include "motive/util.h"
#include "motive/motivator.h"
#include "player_controller.h"
#include "pie_noon_common_generated.h"
#include "timeline_generated.h"

namespace motive {
class MotiveEngine;
}  // motive

namespace fpl {
namespace pie_noon {

class Controller;

typedef int CharacterHealth;

// The different kinds of statistics we're gathering for each player:
enum PlayerStats {
  kWins = 0,  // Last man standing in round.
  kLosses,    // Died during round.
  kDraws,     // Simultanuous kill.
  kAttacks,   // Pies launched.
  kHits,      // Pies launched that hit without being blocked.
  kBlocks,    // Pies from others that were blocked.
  kMisses,    // Pies launched that were blocked.
  kMaxStats
};

enum VictoryState { kResultUnknown, kVictorious, kFailure };

// The current state of the character. This class tracks information external
// to the state machine, like health.
class Character {
 public:
  // The Character does not take ownership of the controller or
  // character_state_machine_def pointers.
  Character(CharacterId id, Controller* controller, const Config& config,
            const CharacterStateMachineDef* character_state_machine_def);

  // Resets the character to the start-of-game state.
  void Reset(CharacterId target, CharacterHealth health, Angle face_angle,
             const mathfu::vec3& position, motive::MotiveEngine* engine);

  // Fake a reaction to input by making the character's face angle
  // jitter slightly in the requested direction. Does not change the
  // target face angle.
  void TwitchFaceAngle(motive::TwitchDirection twitch);

  // Force the character's target, not worrying about facing angle.
  void force_target(CharacterId target) { target_ = target; }

  // Gets the character's current face angle.
  Angle FaceAngle() const { return Angle(face_angle_.Value()); }

  // Sets the character's target and our target face angle.
  void SetTarget(CharacterId target, Angle angle_to_target);

  // Calculate the renderable id for the character at 'anim_time'.
  uint16_t RenderableId(WorldTime anim_time) const;

  // On-screen color. Used in shader when rendering.
  mathfu::vec4 Color() const;

  // Color to tint a button corresponding to this character.
  mathfu::vec4 ButtonColor() const;

  // Returns the timeline of the current state.
  const Timeline* CurrentTimeline() const {
    return state_machine_.current_state()->timeline();
  }

  // Returns the current state from the character-state-machine.
  uint16_t State() const {
    return static_cast<uint16_t>(state_machine_.current_state()->id());
  }

  uint16_t state_last_update() const { return state_last_update_; }

  // Saves off whatever our current state is.  Should be called once per frame,
  // just before (potentially) modifying the state.
  void UpdatePreviousState() { state_last_update_ = State(); }

  // Returns true if the character is still in the game.
  bool Active() const { return State() != StateId_KO; }

  // Play a sound associated with this character.
  void PlaySound(const char* sound) const;

  CharacterHealth health() const { return health_; }
  void set_health(CharacterHealth health) { health_ = health; }

  CharacterId id() const { return id_; }

  CharacterId target() const { return target_; }

  CharacterHealth pie_damage() const { return pie_damage_; }
  void set_pie_damage(CharacterHealth pie_damage) { pie_damage_ = pie_damage; }

  mathfu::vec3 position() const { return position_; }
  void set_position(const mathfu::vec3& position) { position_ = position; }

  const Controller* controller() const { return controller_; }
  Controller* controller() { return controller_; }
  void set_controller(Controller* controller) { controller_ = controller; }

  const CharacterStateMachine* state_machine() const { return &state_machine_; }

  CharacterStateMachine* state_machine() { return &state_machine_; }

  void IncrementStat(PlayerStats stat);
  uint64_t& GetStat(PlayerStats stat) { return player_stats_[stat]; }

  void set_score(int score) { score_ = score; }
  int score() { return score_; }

  void set_just_joined_game(bool just_joined_game) {
    just_joined_game_ = just_joined_game;
  }
  bool just_joined_game() { return just_joined_game_; }

  void set_victory_state(VictoryState state) { victory_state_ = state; }
  VictoryState victory_state() { return victory_state_; }

  // Resets all stats we've accumulated.  Usually called when we have finished
  // sending them to the server.
  void ResetStats();

  bool visible() const { return visible_; }
  void set_visible(bool visible) { visible_ = visible; }

 private:
  // Constant configuration data.
  const Config* config_;

  // Our own id.
  CharacterId id_;

  // Character that we are currently aiming the pie at.
  CharacterId target_;

  // The character's current health. They're out when their health reaches 0.
  CharacterHealth health_;

  // Size of the character's pie. Does this much damage on contact.
  // If unloaded, 0.
  CharacterHealth pie_damage_;

  // World angle. Will eventually settle on the angle towards target_.
  motive::Motivator1f face_angle_;

  // Position of the character in world space.
  mathfu::vec3 position_;

  // The controller used to translate inputs into game actions.
  Controller* controller_;

  // Used to track if this character just joined the game.
  bool just_joined_game_;

  // The current state of the character.
  CharacterStateMachine state_machine_;

  // The stats we're collecting (see PlayerStats enum above).
  uint64_t player_stats_[kMaxStats];

  // The score of the current player for this round (separate from stats because
  // it is not a persisted value.
  int score_;

  // If this character is one of the winners or losers of the match.
  VictoryState victory_state_;

  // What state the character was in last update.
  uint16_t state_last_update_;

  // If the character should be visible.
  bool visible_;
};

class AirbornePie {
 public:
  AirbornePie(CharacterId original_source, const Character& source,
              const Character& target, WorldTime start_time,
              WorldTime flight_time, CharacterHealth original_damage,
              CharacterHealth damage, float start_height, float peak_height,
              int rotations, float y_rotation, motive::MotiveEngine* engine);

  CharacterId original_source() const { return original_source_; }
  CharacterId source() const { return source_; }
  CharacterId target() const { return target_; }
  WorldTime start_time() const { return start_time_; }
  WorldTime flight_time() const { return flight_time_; }
  CharacterHealth original_damage() const { return original_damage_; }
  CharacterHealth damage() const { return damage_; }
  const mathfu::mat4& Matrix() const { return motivator_.Value(); }
  mathfu::vec3 Position() const { return motivator_.Position(); }

 private:
  CharacterId original_source_;
  CharacterId source_;
  CharacterId target_;
  WorldTime start_time_;
  WorldTime flight_time_;
  CharacterHealth original_damage_;
  CharacterHealth damage_;
  motive::MotivatorMatrix4f motivator_;
};

// Return index of first item with time >= t.
// T is a flatbuffer::Vector; one of the Timeline members.
template <class T>
inline int TimelineIndexAfterTime(const T& arr, const int start_index,
                                  const WorldTime t) {
  if (!arr) return 0;

  for (int i = start_index; i < static_cast<int>(arr->Length()); ++i) {
    if (arr->Get(i)->time() >= t) return i;
  }
  return arr->Length();
}

// Return index of last item with time <= t.
// T is a flatbuffer::Vector; one of the Timeline members.
template <class T>
inline int TimelineIndexBeforeTime(const T& arr, const WorldTime t) {
  if (!arr || arr->Length() == 0) return 0;

  for (int i = 1; i < static_cast<int>(arr->Length()); ++i) {
    if (arr->Get(i)->time() > t) return i - 1;
  }
  return arr->Length() - 1;
}

// Return array of indices with time <= t < end_time.
// T is a flatbuffer::Vector; one of the Timeline members.
template <class T>
inline std::vector<int> TimelineIndicesWithTime(const T& arr,
                                                const WorldTime t) {
  std::vector<int> ret;
  if (!arr) return ret;

  for (int i = 0; i < static_cast<int>(arr->Length()); ++i) {
    const float end_time = arr->Get(i)->end_time();
    if (arr->Get(i)->time() <= t && (t < end_time || end_time == 0.0f))
      ret.push_back(i);
  }
  return ret;
}

void ApplyScoringRule(const ScoringRules* scoring_rules, ScoreEvent event,
                      unsigned int damage, Character* character);

}  // pie_noon
}  // fpl

#endif  // PIE_NOON_CHARACTER_H_
