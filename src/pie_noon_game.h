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

#ifndef PIE_NOON_GAME_H
#define PIE_NOON_GAME_H

#ifdef PLATFORM_MOBILE
#define PIE_NOON_USES_GOOGLE_PLAY_GAMES
#endif

#include "ai_controller.h"
#include "audio_engine.h"
#include "full_screen_fader.h"
#include "game_state.h"
#include "gui_menu.h"
#include "input.h"
#include "material_manager.h"
#include "player_controller.h"
#include "renderer.h"
#include "scene_description.h"
#include "touchscreen_button.h"
#include "touchscreen_controller.h"
#ifdef ANDROID_GAMEPAD
#include "gamepad_controller.h"
#endif // ANDROID_GAMEPAD
#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
#include "gpg_manager.h"
#endif

namespace fpl {
namespace pie_noon {

struct Config;
class CharacterStateMachine;
struct RenderingAssets;

enum PieNoonState {
  kUninitialized = 0,
  kLoadingInitialMaterials,
  kLoading,
  kTutorial,
  kJoining,
  kPlaying,
  kPaused,
  kFinished
};

class PieNoonGame {
 public:
  PieNoonGame();
  ~PieNoonGame();
  bool Initialize(const char* const binary_directory);
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
  PieNoonState UpdatePieNoonState();
  void TransitionToPieNoonState(PieNoonState next_state);
  PieNoonState UpdatePieNoonStateAndTransition();
  void FadeToPieNoonState(PieNoonState next_state, const WorldTime& fade_time,
                        const mathfu::vec4& color, const bool fade_in);
  bool Fading() const { return fade_exit_state_ != kUninitialized; }
  void UploadEvents();
  void UploadAndShowLeaderboards();
  void UpdateGamepadControllers();
  int FindAiPlayer();
  ControllerId AddController(Controller* new_controller);
  Controller * GetController(ControllerId id);
  ControllerId FindNextUniqueControllerId();
  void HandlePlayersJoining(Controller* controller);
  void HandlePlayersJoining();
  PieNoonState HandleMenuButtons();
  //void HandleMenuButton(Controller* controller, TouchscreenButton* button);
  void UpdateControllers(WorldTime delta_time);
  void UpdateTouchButtons(WorldTime delta_time);
  ChannelId PlayStinger();
  ButtonId CurrentlyAnimatingJoinImage(WorldTime time) const;
  const char* TutorialSlideName(int slide_index);
  bool AnyControllerPresses();
  void LoadTutorialSlide(int slide_index);
  void LoadInitialTutorialSlides();
  void RenderInMiddleOfScreen(const mathfu::mat4& ortho_mat, float x_scale,
                              Material* material);

  int ReadPreference(const char *key, int initial_value, int failure_value);
  void WritePreference(const char *key, int value);

  // The overall operating mode of our game. See CalculatePieNoonState for the
  // state machine definition.
  PieNoonState state_;

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
  Shader* shader_grayscale_;

  // Shadow material.
  Material* shadow_mat_;

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
  GuiMenu gui_menu_;

  std::map<int, ControllerId> gamepad_to_controller_map_;

  ButtonId join_id_;
  impel::Impeller1f join_impeller_;

  // Used to render an overlay to fade the screen.
  FullScreenFader full_screen_fader_;
  // State to enter after the fade is complete.
  PieNoonState fade_exit_state_;

  // Channel used to play the ambience sound effect.
  ChannelId ambience_channel_;

  // A stinger will play before transition to the finished state. Don't
  // transition until the stinger is complete.
  ChannelId stinger_channel_;

  // Our current slide of the tutorial. Valid when state_ is kTutorial.
  int tutorial_slide_index_;

# ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  GPGManager gpg_manager;
# endif
};

}  // pie_noon
}  // fpl

#endif  // PIE_NOON_GAME_H

