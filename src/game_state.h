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

class GameState {
 public:
  GameState();

  // Update controller and state machine for each character.
  void AdvanceFrame(WorldTime delta_time);

  // Fill in the position of the characters and pies.
  void PopulateScene(SceneDescription* scene) const;

  // Angle to the character's target.
  Angle TargetFaceAngle(CharacterId id) const;

  // Difference between target face angle and current face angle.
  Angle FaceAngleError(CharacterId id) const;

  std::vector<Character>& characters() { return characters_; }
  const std::vector<Character>& characters() const { return characters_; }

  std::vector<AirbornePie>& pies() { return pies_; }
  const std::vector<AirbornePie>& pies() const { return pies_; }

  WorldTime time() const { return time_; }

  const mathfu::mat4& camera_matrix() const { return camera_matrix_; }
  void set_camera_matrix(const mathfu::mat4& matrix) {
    camera_matrix_ = matrix;
  }

private:
  WorldTime GetAnimationTime(const Character& c) const;
  void ProcessEvents(Character& c, WorldTime delta_time);
  void UpdatePiePosition(AirbornePie& pie) const;
  float CalculateCharacterFacingAngleVelocity(
      const Character& c, WorldTime delta_time) const;

  WorldTime time_;
  mathfu::mat4 camera_matrix_;
  std::vector<Character> characters_;
  std::vector<AirbornePie> pies_;
};

}  // splat
}  // fpl

#endif  // GAME_STATE_H_
