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
#include "character.h"

namespace fpl {

class InputSystem;
class SceneDescription;

namespace splat {

struct Config;

class GameState {
 public:
  GameState();

  // Return to default configuration.
  void Reset();

  // Update controller and state machine for each character.
  void AdvanceFrame(WorldTime delta_time);

  // Fill in the position of the characters and pies.
  void PopulateScene(SceneDescription* scene) const;

  // Angle between two characters.
  Angle AngleBetweenCharacters(CharacterId source_id,
                               CharacterId target_id) const;

  // Angle to the character's target.
  Angle TargetFaceAngle(CharacterId id) const;

  // Difference between target face angle and current face angle.
  Angle FaceAngleError(CharacterId id) const;

  // Returns one of the RenderableId enums.
  uint16_t CharacterState(CharacterId id) const;

  const mathfu::vec3& camera_position() const { return camera_position_; }
  void set_camera_position(const mathfu::vec3& position) {
    camera_position_ = position;
  }

  const mathfu::vec3& camera_target() const { return camera_target_; }
  void set_camera_target(const mathfu::vec3& target) {
    camera_target_ = target;
  }

  std::vector<Character>& characters() { return characters_; }
  const std::vector<Character>& characters() const { return characters_; }

  std::vector<AirbornePie>& pies() { return pies_; }
  const std::vector<AirbornePie>& pies() const { return pies_; }

  WorldTime time() const { return time_; }

  void set_config(const Config* config) { config_ = config; }

private:
  WorldTime GetAnimationTime(const Character& character) const;
  void ProcessEvents(Character* character,
                     WorldTime delta_time,
                     int queued_damage);
  void UpdatePiePosition(AirbornePie* pie) const;
  CharacterId CalculateCharacterTarget(CharacterId id) const;
  float CalculateCharacterFacingAngleVelocity(const Character& character,
                                              WorldTime delta_time) const;
  mathfu::mat4 CameraMatrix() const;

  WorldTime time_;
  mathfu::vec3 camera_position_;
  mathfu::vec3 camera_target_;
  std::vector<Character> characters_;
  std::vector<AirbornePie> pies_;
  const Config* config_;
};

}  // splat
}  // fpl

#endif  // GAME_STATE_H_
