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
class RenderScene;

namespace splat {

class GameState {
 public:
  // Update all players' controller and state machinel.
  void AdvanceFrame();

  // Fill in the position of the characters and pies.
  void PopulateScene(RenderScene* scene) const;

  std::vector<Character>& characters() { return characters_; }
  const std::vector<Character>& characters() const { return characters_; }

  std::vector<AirbornePie>& pies() { return pies_; }
  const std::vector<AirbornePie>& pies() const { return pies_; }

private:
  // Time consumed when AdvanceFrame is called.
  static const WorldTime kDeltaTime = 1;

  WorldTime GetAnimationTime(const Character& c) const;
  void ProcessEvents(Character& c);
  void UpdatePiePosition(AirbornePie& pie) const;

  WorldTime time_;
  std::vector<Character> characters_;
  std::vector<AirbornePie> pies_;
};

}  // splat
}  // fpl

#endif  // GAME_STATE_H_
