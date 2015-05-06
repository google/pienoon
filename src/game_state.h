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

#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <vector>
#include <memory>
#include "character.h"
#include "components/cardboard_player.h"
#include "components/drip_and_vanish.h"
#include "components/player_character.h"
#include "components/scene_object.h"
#include "components/shakeable_prop.h"
#include "entity/entity.h"
#include "entity/entity_manager.h"
#include "game_camera.h"
#include "motive/engine.h"
#include "motive/processor.h"
#include "motive/util.h"
#include "particles.h"

namespace pindrop {
class AudioEngine;
}  // namespace pindrop

namespace fpl {

class InputSystem;
class SceneDescription;

namespace pie_noon {

struct Config;
struct CharacterArrangement;
struct EventData;
struct ReceivedPie;
class MultiplayerDirector;

class PieNoonEntityFactory : public entity::EntityFactoryInterface {
 public:
  virtual entity::EntityRef CreateEntityFromData(
      const void* data, entity::EntityManager* entity_manager);
};

class GameState {
 public:
  enum AnalyticsMode { kNoAnalytics, kTrackAnalytics };

  GameState();
  ~GameState();

  // Returns true if the game has reached it's end-game condition.
  bool IsGameOver() const;

  // Return to default configuration.
  // If demo mode is set, no analytics tracking will be performed.
  void Reset(AnalyticsMode analytics_mode);
  void Reset();

  // Update controller and state machine for each character.
  void AdvanceFrame(WorldTime delta_time, pindrop::AudioEngine* audio_engine);

  // To be run before starting a game and after ending one to log data about
  // gameplay.
  void PreGameLogging() const;
  void PostGameLogging() const;

  // Fill in the position of the characters and pies.
  void PopulateScene(SceneDescription* scene);

  // Angle between two characters.
  Angle AngleBetweenCharacters(CharacterId source_id,
                               CharacterId target_id) const;

  // Angle to the character's target.
  Angle TargetFaceAngle(CharacterId id) const;

  // Returns one of the RenderableId enums.
  uint16_t CharacterState(CharacterId id) const;

  // Returns an OR of all the character's logical inputs.
  uint32_t AllLogicalInputs() const;

  // Returns the number of characters who are still in the game (that is,
  // are not KO'd or otherwise incapacitated).
  // By default counts both human players and AI.
  int NumActiveCharacters(bool human_only = false) const;

  // Determines which characters are the winners and losers, and increments
  // their stats appropriately.
  void DetermineWinnersAndLosers();

  // Returns true if the character cannot turn, either because the
  // character has only one valid direction to face, or because the
  // character is incapacitated.
  bool IsImmobile(CharacterId id) const;

  GameCamera& camera() { return camera_; }
  const GameCamera& camera() const { return camera_; }
  mathfu::mat4 CameraMatrix() const;

  std::vector<std::unique_ptr<Character>>& characters() { return characters_; }
  const std::vector<std::unique_ptr<Character>>& characters() const {
    return characters_;
  }

  std::vector<std::unique_ptr<AirbornePie>>& pies() { return pies_; }
  const std::vector<std::unique_ptr<AirbornePie>>& pies() const {
    return pies_;
  }

  WorldTime time() const { return time_; }

  void set_config(const Config* config) { config_ = config; }

  void set_cardboard_config(const Config* config) {
    cardboard_config_ = config;
  }

  motive::MotiveEngine& engine() { return engine_; }
  ParticleManager& particle_manager() { return particle_manager_; }

  // Sets up the players in joining mode, where all they can do is jump up
  // and down.
  void EnterJoiningMode();

  WorldTime GetAnimationTime(const Character& character) const;

  // Sets the MultiplayerDirector we can talk to to propagate some game state
  // across the network. You must ensure it stays in memory as long as GameState
  // does.
  void RegisterMultiplayerDirector(MultiplayerDirector* director) {
    multiplayer_director_ = director;
  }

  void set_is_multiscreen(bool b) { is_multiscreen_ = b; }
  bool is_multiscreen() const { return is_multiscreen_; }

  void set_is_in_cardboard(bool b) { is_in_cardboard_ = b; }
  bool is_in_cardboard() const { return is_in_cardboard_; }

  void set_use_undistort_rendering(bool b) { use_undistort_rendering_ = b; }
  bool use_undistort_rendering() { return use_undistort_rendering_; }

 private:
  void ProcessSounds(pindrop::AudioEngine* audio_engine,
                     const Character& character, WorldTime delta_time) const;
  void CreatePie(CharacterId original_source_id, CharacterId source_id,
                 CharacterId target_id, CharacterHealth original_damage,
                 CharacterHealth damage);
  float CalculatePieYRotation(CharacterId source_id,
                              CharacterId target_id) const;
  CharacterId DetermineDeflectionTarget(const ReceivedPie& pie) const;
  void ProcessEvent(pindrop::AudioEngine* audio_engine, Character* character,
                    unsigned int event, const EventData& event_data);
  void PopulateConditionInputs(ConditionInputs* condition_inputs,
                               const Character& character) const;
  void PopulateCharacterAccessories(SceneDescription* scene,
                                    uint16_t renderable_id,
                                    const mathfu::mat4& character_matrix,
                                    int num_accessories, int damage,
                                    int health) const;
  void ProcessConditionalEvents(pindrop::AudioEngine* audio_engine,
                                Character* character, EventData* event_data);
  void ProcessEvents(pindrop::AudioEngine* audio_engine, Character* character,
                     EventData* data, WorldTime delta_time);
  void UpdatePiePosition(AirbornePie* pie) const;
  CharacterId CalculateCharacterTarget(CharacterId id) const;
  float CalculateCharacterFacingAngleVelocity(const Character* character,
                                              WorldTime delta_time) const;
  int RequestedTurn(CharacterId id) const;
  Angle TiltTowardsStageFront(const Angle angle) const;
  Angle TiltCharacterAwayFromCamera(CharacterId id, const Angle angle) const;
  motive::TwitchDirection FakeResponseToTurn(CharacterId id) const;
  void AddParticlesToScene(SceneDescription* scene) const;
  void CreatePieSplatter(pindrop::AudioEngine* audio_engine,
                         const Character& character, int damage);
  void CreateJoinConfettiBurst(const Character& character);
  void SpawnParticles(const mathfu::vec3& position, const ParticleDef* def,
                      const int particle_count,
                      const mathfu::vec4& base_tint = mathfu::vec4(1, 1, 1, 1));
  void ShakeProps(float percent, const mathfu::vec3& damage_position);
  void AddSplatterToProp(entity::EntityRef prop);

  WorldTime time_;
  // countdown_time_ is in seconds and is derived from the length of the game
  // given in the config file minus the duration of the current game given in
  // the time_ variable.
  int countdown_timer_;
  GameCamera camera_;
  GameCameraState camera_base_;
  std::vector<std::unique_ptr<Character>> characters_;
  std::vector<std::unique_ptr<AirbornePie>> pies_;
  motive::MotiveEngine engine_;
  const Config* config_;
  const CharacterArrangement* arrangement_;
  ParticleManager particle_manager_;
  AnalyticsMode analytics_mode_;

  // Entity manager that tracks all of our entities.
  entity::EntityManager entity_manager_;
  // Entity factory for creating entities from flatbuffers:
  PieNoonEntityFactory pie_noon_entity_factory_;

  // Component for handling movable objects in the scene.
  SceneObjectComponent sceneobject_component_;
  // Component for handling static, swaying props in the background.
  ShakeablePropComponent shakeable_prop_component_;
  // Component for scenery-splatter behavior.
  DripAndVanishComponent drip_and_vanish_component_;
  // Component for drawing player characters:
  PlayerCharacterComponent player_character_component_;
  // Component for drawing Cardboard mode information.
  CardboardPlayerComponent cardboard_player_component_;

  // For multi-screen mode.
  MultiplayerDirector* multiplayer_director_;

  // Whether you are playing in multiscreen mode.
  bool is_multiscreen_;

  const Config* cardboard_config_;
  // Whether you are playing in Cardboard mode.
  bool is_in_cardboard_;
  // Whether it should use undistortion rendering in Cardboard.
  bool use_undistort_rendering_;
};

}  // pie_noon
}  // fpl

#endif  // GAME_STATE_H_
