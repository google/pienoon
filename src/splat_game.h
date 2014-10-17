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

#include "ai_controller.h"
#include "audio_engine.h"
#include "full_screen_fader.h"
#include "gamepad_controller.h"
#include "game_state.h"
#include "input.h"
#include "material_manager.h"
#include "player_controller.h"
#include "renderer.h"
#include "scene_description.h"
#include "touchscreen_button.h"
#include "touchscreen_controller.h"
#ifdef PLATFORM_MOBILE
#include "gpg_manager.h"
#endif

namespace fpl {
namespace splat {

struct Config;
class CharacterStateMachine;
struct RenderingAssets;

enum SplatState {
  kUninitialized = 0,
  kLoadingInitialMaterials,
  kLoading,
  kPlaying,
  kFinished
};

class SplatGame {
 public:
  SplatGame();
  ~SplatGame();
  bool Initialize();
  void Run();

 private:
  bool InitializeConfig();
  bool InitializeRenderer();
  Mesh* CreateVerticalQuadMesh(const flatbuffers::String* material_name,
                               const vec3& offset, const vec2& pixel_bounds,
                               float pixel_to_world_scale);
  bool InitializeRenderingAssets();
  bool InitializeGameState();
  void RenderCardboard(const SceneDescription& scene,
                       const mat4& camera_transform);
  void Render(const SceneDescription& scene);
  void Render2DElements();
  void DebugPrintCharacterStates();
  void DebugPrintPieStates();
  void DebugCamera();
  const Config& GetConfig() const;
  const CharacterStateMachineDef* GetStateMachine() const;
  Mesh* GetCardboardFront(int renderable_id);
  SplatState UpdateSplatState();
  void TransitionToSplatState(SplatState next_state);
  SplatState UpdateSplatStateAndTransition();
  void FadeToSplatState(SplatState next_state, const WorldTime& fade_time,
                        const mathfu::vec4& color, const bool fade_in);
  bool Fading() const { return fade_exit_state_ != kUninitialized; }
  void UploadEvents();
  void UploadAndShowLeaderboards();
  void UpdateGamepadControllers();
  int FindAiPlayer();
  ControllerId AddController(Controller* new_controller);
  Controller * GetController(ControllerId id);
  ControllerId FindNextUniqueControllerId();
  void HandlePlayersJoining();
  void HandlePlayersMenu();
  void UpdateControllers(WorldTime delta_time);
  void UpdateTouchButtons();

  // The overall operating mode of our game. See CalculateSplatState for the
  // state machine definition.
  SplatState state_;

  // The elapsed time when we entered state_. In the same time system as
  // prev_world_time_.
  WorldTime state_entry_time_;

  // Hold configuration binary data.
  std::string config_source_;

  // Report touches, button presses, keyboard presses.
  InputSystem input_;

  // Hold rendering context.
  Renderer renderer_;

  // Load and own rendering resources.
  MaterialManager matman_;

  // Manage ownership and playing of audio assets.
  AudioEngine audio_engine_;

  // Map RenderableId to rendering mesh.
  std::vector<Mesh*> cardboard_fronts_;
  std::vector<Mesh*> cardboard_backs_;

  // Rendering mesh for front and back of the stick that props cardboard.
  Mesh* stick_front_;
  Mesh* stick_back_;

  // Shaders we use.
  Shader* shader_cardboard;
  Shader* shader_lit_textured_normal_;
  Shader* shader_simple_shadow_;
  Shader* shader_textured_;

  // Shadow material.
  Material* shadow_mat_;

  // Splash screen and splash screen text materials.
  std::vector<Material*> materials_for_finished_state_;

  // Final matrix that applies the view frustum to bring into screen space.
  mat4 perspective_matrix_;

  // Hold state machine binary data.
  std::string state_machine_source_;

  // Hold characters, pies, camera state.
  GameState game_state_;

  // Map containing every active controller, referenced by a unique,
  // unchanging ID.
  std::vector<std::unique_ptr<Controller>> active_controllers_;

  // Description of the scene to be rendered. Isolates gameplay and rendering
  // code with a type-light structure. Recreated every frame.
  SceneDescription scene_;

  // World time of previous update. We use this to calculate the delta_time
  // of the current update. This value is tied to the real-world clock.
  // Note that it is distict from game_state_.time_, which is *not* tied to the
  // real-world clock. If the game is paused, game_state.time_ will pause, but
  // prev_world_time_ will keep chugging.
  WorldTime prev_world_time_;

  // Debug data. For displaying when a character's state has changed.
  std::vector<int> debug_previous_states_;
  std::vector<Angle> debug_previous_angles_;

  TouchscreenController* touch_controller_;

  std::vector<TouchscreenButton>touch_controls_;

  std::map<SDL_JoystickID, ControllerId> joystick_to_controller_map_;

  // Used to render an overlay to fade the screen.
  FullScreenFader full_screen_fader_;
  // State to enter after the fade is complete.
  SplatState fade_exit_state_;

# ifdef PLATFORM_MOBILE
  GPGManager gpg_manager;
# endif
};

}  // splat
}  // fpl

#endif  // SPLAT_GAME_H

