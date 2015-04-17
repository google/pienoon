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
#include "cardboard_controller.h"
#include "full_screen_fader.h"
#include "game_state.h"
#include "gui_menu.h"
#include "input.h"
#include "material_manager.h"
#include "multiplayer_controller.h"
#include "multiplayer_director.h"
#include "pindrop/pindrop.h"
#include "player_controller.h"
#include "renderer.h"
#include "scene_description.h"
#include "touchscreen_button.h"
#include "touchscreen_controller.h"

#ifdef ANDROID_GAMEPAD
#include "gamepad_controller.h"
#endif  // ANDROID_GAMEPAD
#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
#include "gpg_manager.h"
#include "gpg_multiplayer.h"
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
  kFinished,
  kMultiplayerWaiting,
  kMultiscreenClient,
};

class PieNoonGame {
 public:
  PieNoonGame();
  ~PieNoonGame();
  bool Initialize(const char* const binary_directory);
  void Run();

 private:
  bool InitializeConfig();
#ifdef ANDROID_CARDBOARD
  bool InitializeCardboardConfig();
#endif
  bool InitializeRenderer();
  Mesh* CreateVerticalQuadMesh(const flatbuffers::String* material_name,
                               const vec3& offset, const vec2& pixel_bounds,
                               float pixel_to_world_scale);
  bool InitializeRenderingAssets();
  bool InitializeGameState();
  void RenderCardboard(const SceneDescription& scene,
                       const mat4& camera_transform);
  void Render(const SceneDescription& scene);
  void RenderForDefault(const SceneDescription& scene);
  void RenderForCardboard(const SceneDescription& scene);
  void RenderScene(const SceneDescription& scene,
                   const mat4& additional_camera_changes,
                   const vec2i& resolution);
  void Render2DElements();
  void GetCardboardTransforms(mat4& left_eye_transform,
                              mat4& right_eye_transform);
  void CorrectCardboardCamera(mat4& cardboard_camera);
  void CardboardUndistortFramebuffer();
  void RenderCardboardCenteringBar();
  void DebugPrintCharacterStates();
  void DebugPrintPieStates();
  void DebugCamera();
  const Config& GetConfig() const;
  const Config& GetCardboardConfig() const;
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
  Controller* GetController(ControllerId id);
  ControllerId FindNextUniqueControllerId();
  void HandlePlayersJoining(Controller* controller);
  void HandlePlayersJoining();
  void AttachMultiplayerControllers();
  PieNoonState HandleMenuButtons(WorldTime time);
  // void HandleMenuButton(Controller* controller, TouchscreenButton* button);
  void UpdateControllers(WorldTime delta_time);
  void UpdateTouchButtons(WorldTime delta_time);

  pindrop::Channel PlayStinger();
  void InitCountdownImage(int seconds);
  void UpdateCountdownImage(WorldTime time);

  ButtonId CurrentlyAnimatingJoinImage(WorldTime time) const;
  const char* TutorialSlideName(int slide_index);
  bool AnyControllerPresses();
  void LoadTutorialSlide(int slide_index);
  void LoadInitialTutorialSlides();
  void RenderInMiddleOfScreen(const mathfu::mat4& ortho_mat, float x_scale,
                              Material* material);

  void ProcessMultiplayerMessages();
  void ProcessPlayerStatusMessage(const multiplayer::PlayerStatus&);

  // returns true if a new splat was displayed
  bool ShowMultiscreenSplat(int splat_num);

  static int ReadPreference(const char* key, int initial_value,
                            int failure_value);
  static void WritePreference(const char* key, int value);

  void CheckForNewAchievements();

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  void StartMultiscreenGameAsHost();
  void StartMultiscreenGameAsClient(CharacterId id);
  void SendMultiscreenPlayerCommand();
#endif
  void ReloadMultiscreenMenu();
  void UpdateMultiscreenMenuIcons();
  void SetupWaitingForPlayersMenu();

  // The overall operating mode of our game. See CalculatePieNoonState for the
  // state machine definition.
  PieNoonState state_;

  // The elapsed time when we entered state_. In the same time system as
  // prev_world_time_.
  WorldTime state_entry_time_;

  // Hold configuration binary data.
  std::string config_source_;
#ifdef ANDROID_CARDBOARD
  std::string cardboard_config_source_;
#endif

  // Report touches, button presses, keyboard presses.
  InputSystem input_;

  // Hold rendering context.
  Renderer renderer_;

  // Load and own rendering resources.
  MaterialManager matman_;

  // Manage ownership and playing of audio assets.
  pindrop::AudioEngine audio_engine_;

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

  // On the host, directs the fake controllers in the multiscreen gameplay.
  std::unique_ptr<MultiplayerDirector> multiplayer_director_;
  // On the client, which player are we? 0-3
  CharacterId multiscreen_my_player_id_;
  // On the client, Which action we are performing: Attack, Defend, or Cancel
  // (for waiting).
  ButtonId multiscreen_action_to_perform_;
  // On the client, which other player we are aimed at.  In multiscreen, each
  // player starts aimed at the next player (or p3 is aimed back at p0).
  CharacterId multiscreen_action_aim_at_;
  int multiscreen_turn_number_;
  // Animation for the multiscreen splats that appear.
  float multiscreen_splat_param;
  float multiscreen_splat_param_speed;

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

  CardboardController* cardboard_controller_;

  ButtonId join_id_;
  motive::Motivator1f join_motivator_;

  // Used to render an overlay to fade the screen.
  FullScreenFader full_screen_fader_;
  // State to enter after the fade is complete.
  PieNoonState fade_exit_state_;

  // Channel used to play the ambience sound effect.
  pindrop::Channel ambience_channel_;

  // A stinger will play before transition to the finished state. Don't
  // transition until the stinger is complete.
  pindrop::Channel stinger_channel_;

  // Channel for the menu music or in-game music.
  pindrop::Channel music_channel_;

  // Tutorial slides we are in the midst of displaying.
  std::vector<std::string> tutorial_slides_;

  // Tutorial aspect ratio
  float tutorial_aspect_ratio_;

  // Our current slide of the tutorial. Valid when state_ is kTutorial.
  int tutorial_slide_index_;

  // The Worldtime when the curent tutorial slide was displayed.
  WorldTime tutorial_slide_time_;

  // The time we started animating the "join" countdown image
  WorldTime join_animation_start_time_;

  ButtonId countdown_start_button_;

  // The time at which the current turn is over.
  WorldTime multiscreen_turn_end_time_;

  int next_achievement_index_;

  // String version number of the game.
  const char* version_;

  // The Worldtime when the game was paused, used just for analytics.
  WorldTime pause_time_;

#ifdef PIE_NOON_USES_GOOGLE_PLAY_GAMES
  GPGManager gpg_manager;

  // Network multiplayer library for multi-screen version
  GPGMultiplayer gpg_multiplayer_;
#endif
};

}  // pie_noon
}  // fpl

#endif  // PIE_NOON_GAME_H
