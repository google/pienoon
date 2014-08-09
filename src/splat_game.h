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

#ifndef SPLAT_GAME_H
#define SPLAT_GAME_H

#include "game_state.h"
#include "input.h"
#include "material_manager.h"
#include "render_scene.h"
#include "renderer.h"
#include "sdl_controller.h"

namespace fpl {
namespace splat {

class SplatGame {
 public:
  SplatGame();
  bool Initialize();
  void Run();

 private:
  bool InitializeRenderer();
  bool InitializeMaterials();
  bool InitializeGameState();
  void Render(const RenderScene& scene);
  void DebugPlayerStates();
  void DebugRenderExampleTriangle();

  InputSystem input_;
  Renderer renderer_;
  MaterialManager matman_;
  std::vector<Material*> materials_;
  std::string state_machine_source_;
  splat::GameState game_state_;
  splat::SdlController* controllers_[splat::kPlayerCount];
  RenderScene scene_;

  // World time of previous update. We use this to calculate the delta_time
  // of the current update. This value is tied to the real-world clock.
  // Note that it is distict from game_state_.time_, which is *not* tied to the
  // real-world clock. If the game is paused, game_state.time_ will pause, but
  // prev_world_time_ will keep chugging.
  WorldTime prev_world_time_;

  // Debug data. For displaying when a character's state has changed.
  std::vector<int> debug_previous_states_;
};

}  // splat
}  // fpl

#endif  // SPLAT_GAME_H

