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

namespace fpl {

class AudioEngine;
class InputSystem;
class SceneDescription;

namespace splat {

struct Config;
struct CharacterArrangement;
struct EventData;
struct ReceivedPie;

class GameState {
 public:
  GameState();

  // Return to default configuration.
  void Reset();

  // Update controller and state machine for each character.
  void AdvanceFrame(WorldTime delta_time, AudioEngine* audio_engine);

  // Fill in the position of the characters and pies.
  void PopulateScene(SceneDescription* scene) const;

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
  int NumActiveCharacters() const;

  // Returns the id of the winning character, or -1 if no one has won the game.
  // Note that not all games have a winner. Some games may end with everyone
  // KO'd.
  CharacterId WinningCharacterId() const;

  // Returns true if the character cannot turn, either because the
  // character has only one valid direction to face, or because the
  // character is incapacitated.
  bool IsImmobile(CharacterId id) const;

  const mathfu::vec3& camera_position() const { return camera_position_; }
  void set_camera_position(const mathfu::vec3& position) {
    camera_position_ = position;
  }

  const mathfu::vec3& camera_target() const { return camera_target_; }
  void set_camera_target(const mathfu::vec3& target) {
    camera_target_ = target;
  }

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

private:
  WorldTime GetAnimationTime(const Character& character) const;
  void ProcessSounds(AudioEngine* audio_engine,
                     const Character& character,
                     WorldTime delta_time) const;
  void CreatePie(CharacterId source_id, CharacterId target_id, int damage);
  CharacterId DetermineDeflectionTarget(const ReceivedPie& pie) const;
  void ProcessEvent(Character* character,
                    unsigned int event,
                    const EventData& event_data);
  void PopulateConditionInputs(ConditionInputs* condition_inputs,
                               const Character& character) const;
  void PopulateCharacterAccessories(SceneDescription* scene, uint16_t renderable_id,
                                    const mathfu::mat4& character_matrix,
                                    int num_accessories, int damage,
                                    int health) const;
  void ProcessConditionalEvents(Character* character, EventData* event_data);
  void ProcessEvents(Character* character,
                     EventData* data,
                     WorldTime delta_time);
  void UpdatePiePosition(AirbornePie* pie) const;
  CharacterId CalculateCharacterTarget(CharacterId id) const;
  float CalculateCharacterFacingAngleVelocity(const Character* character,
                                              WorldTime delta_time) const;
  mathfu::mat4 CameraMatrix() const;
  int RequestedTurn(CharacterId id) const;
  Angle TiltTowardsStageFront(const Angle angle) const;
  MagnetTwitch FakeResponseToTurn(CharacterId id) const;

  WorldTime time_;
  mathfu::vec3 camera_position_;
  mathfu::vec3 camera_target_;
  std::vector<std::unique_ptr<Character>> characters_;
  std::vector<std::unique_ptr<AirbornePie>> pies_;
  const Config* config_;
  const CharacterArrangement* arrangement_;
};

}  // splat
}  // fpl

#endif  // GAME_STATE_H_
