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

#include "precompiled.h"

#include "SDL_timer.h"

#include "angle.h"
#include "character_state_machine.h"
// TODO: move to alphabetical order once FlatBuffer include dependency fixed
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "splat_game.h"
#include "utilities.h"

namespace fpl {
namespace splat {

// Don't let our game go faster than 100Hz. The game won't look much smoother
// or play better at faster frame rates. We'll just be hogging the CPU.
static const WorldTime kMinUpdateTime = 10;

// Don't let us jump more than 100ms in one simulation update. If we're
// debugging, or a task switch happens, we could get super-large update times
// that we'd rather just ignore.
static const WorldTime kMaxUpdateTime = 100;


static const float kViewportAngle = 55.0f;
static const float kViewportAspectRatio = 1280.0f / 800.0f;
static const float kViewportNearPlane = 10.0f;
static const float kViewportFarPlane = 50.0f;

static const mat4 kRotate180AroundZ = mat4(-1, 0, 0, 0,
                                            0,-1, 0, 0,
                                            0, 0, 1, 0,
                                            0, 0, 0, 1);

static const char kAssetsDir[] = "assets";
static const char *kBuildPaths[] = {
    "Debug", "Release", "projects\\VisualStudio2010", "build\\Debug\\bin",
    "build\\Release\\bin"};

// Return the elapsed milliseconds since the start of the program. This number
// will loop back to 0 after about 49 days; always take the difference to
// properly handle the wrap-around case.
static inline WorldTime CurrentWorldTime() {
  return SDL_GetTicks();
}

SplatGame::SplatGame()
  : matman_(renderer_)
  , prev_world_time_(0)
  , debug_previous_states_(kPlayerCount, -1)
{}

// Initialize the 'renderer_' member. No other members have been initialized at
// this point.
bool SplatGame::InitializeRenderer() {
  renderer_.Initialize(vec2i(1280, 800), "Splat!");

  const vec3 cameraPosition = vec3(0, 0, -25);

  renderer_.camera.model_view_projection_ =
      mat4::Perspective(kViewportAngle,
                        kViewportAspectRatio,
                        kViewportNearPlane,
                        kViewportFarPlane) *
      kRotate180AroundZ *
      mat4::FromTranslationVector(cameraPosition);

  renderer_.color = vec4(1, 1, 1, 1);

  return true;
}

// Load textures for cardboard into 'materials_'. The 'renderer_' and 'matman_'
// members have been initialized at this point.
bool SplatGame::InitializeMaterials() {
  for (int i = 0; i < RenderableId_Count; ++i) {
    const std::string material_file_name = FileNameFromEnumName(
                                              EnumNameRenderableId(i),
                                              "materials/", ".bin");
    Material* mat = matman_.LoadMaterial(material_file_name.c_str());
    if (!mat) {
      fprintf(stderr, "Error loading material %s: %s\n", EnumNameRenderableId(i),
              renderer_.last_error_.c_str());
      return false;
    }
    materials_.push_back(mat);
  }
  return true;
}

// Calculate a character's target at the start of the game. We want the
// characters to aim at the character directly opposite them.
static splat::CharacterId InitialTargetId(const splat::CharacterId id) {
  return static_cast<splat::CharacterId>(
      (id + splat::kPlayerCount / 2) % splat::kPlayerCount);
}

// The position of a character is at the start of the game.
static mathfu::vec3 InitialPosition(const splat::CharacterId id) {
  static const float kCharacterDistFromCenter = 10.0f;
  const Angle angle_to_position = Angle::FromWithinThreePi(
      static_cast<float>(id) * kTwoPi / splat::kPlayerCount);
  return kCharacterDistFromCenter * angle_to_position.ToXZVector();
}

// Calculate the direction a character is facing at the start of the game.
// We want the characters to face their initial target.
static Angle InitialFaceAngle(const CharacterId id) {
  const mathfu::vec3 characterPosition = InitialPosition(id);
  const mathfu::vec3 targetPosition = InitialPosition(InitialTargetId(id));
  return Angle::FromXZVector(targetPosition - characterPosition);
}

// Create state matchines, characters, controllers, etc. present in
// 'gamestate_'.
bool SplatGame::InitializeGameState() {
  // Load flatbuffer into buffer.
  if (!flatbuffers::LoadFile("character_state_machine_def.bin",
                             true, &state_machine_source_)) {
    fprintf(stderr, "Error loading character state machine.\n");
    return false;
  }

  // Grab the state machine from the buffer.
  auto state_machine_def =
     GetCharacterStateMachineDef(state_machine_source_.c_str());
  if (!CharacterStateMachineDef_Validate(state_machine_def)) {
    fprintf(stderr, "State machine is invalid.\n");
    return false;
  }

  // Create controllers.
  for (CharacterId id = 0; id < kPlayerCount; id++) {
    controllers_.push_back(SdlController(
        &input_, ControlScheme::GetDefaultControlScheme(id)));
  }

  // Create characters.
  for (CharacterId id = 0; id < kPlayerCount; id++) {
    game_state_.characters().push_back(Character(
        id, InitialTargetId(id), kDefaultHealth, InitialFaceAngle(id),
        InitialPosition(id), &controllers_[id], state_machine_def));
  }

  return true;
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool SplatGame::Initialize() {
  printf("Splat initializing...\n");

  if (!ChangeToUpstreamDir(kAssetsDir, kBuildPaths, ARRAYSIZE(kBuildPaths)))
    return false;

  if (!InitializeRenderer())
    return false;

  if (!InitializeMaterials())
    return false;

  if (!InitializeGameState())
    return false;

  printf("Splat initialization complete\n");
  return true;
}

void SplatGame::Render(const RenderScene& scene) {
  // TODO: Implement draw calls here.
  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];
    const Material* mat = materials_[renderable.id()];
    (void)mat;
    (void)renderer_;
    // TODO: Draw carboard with texture from 'mat' at location
    // renderable.matrix_
  }
}

// Debug function to write out state machine transitions.
// TODO: Remove this block and the one in the main loop that prints the
// current state.
void SplatGame::DebugPlayerStates() {
  // Display the state changes, at least until we get real rendering up.
  for (int i = 0; i < kPlayerCount; i++) {
    auto& player = game_state_.characters()[i];
    int id = player.state_machine()->current_state()->id();
    if (debug_previous_states_[i] != id) {
      printf("Player %d - Health %2d, State %s [%d]\n",
              i, player.health(), EnumNameStateId(id), id);
      debug_previous_states_[i] = id;
    }
  }
}

// Debug function providing an example of how to use the renderer.
// TODO: Remove after SplatGame::Render has been implemented.
void SplatGame::DebugRenderExampleTriangle() {
  static Attribute format[] = { kPosition3f, kTexCoord2f, kEND };
  static int indices[] = { 0, 1, 2, 3};
  static float vertices[] = { -1, 0, 0,   0, 0,
                               1, 0, 0,   1, 0,
                              -1, 3, 0,   0, 1,
                               1, 3, 0,   1, 1};

  // Some random "interactivity".
  if (input_.GetButton(SDLK_POINTER1).is_down()) {
    vertices[0] += input_.pointers_[0].mousedelta.x() / 100.0f;
  }

  // Draw example render data.
  Material *idle_character =
          matman_.FindMaterial("materials/character_idle.bin");
  idle_character->Set(renderer_);
  Mesh::RenderArray(GL_TRIANGLE_STRIP, 4, format, sizeof(float) * 5,
                    reinterpret_cast<const char *>(vertices), indices);
}

// Main game loop.
void SplatGame::Run() {
   // Initialize so that we don't sleep the first time through the loop.
  prev_world_time_ = CurrentWorldTime() - kMinUpdateTime;

  while (!input_.exit_requested_ &&
         !input_.GetButton(SDLK_ESCAPE).went_down()) {
    // Milliseconds elapsed since last update. To avoid burning through the CPU,
    // enforce a minimum time between updates. For example, if kMinUpdateTime
    // is 1, we will not exceed 1000Hz update time.
    const WorldTime world_time = CurrentWorldTime();
    const WorldTime delta_time = std::min(world_time - prev_world_time_,
                                          kMaxUpdateTime);
    if (delta_time < kMinUpdateTime) {
      SleepForMilliseconds(kMinUpdateTime - delta_time);
      continue;
    }

    // TODO: Can we move these to 'Render'?
    renderer_.AdvanceFrame(input_.minimized_);
    renderer_.ClearFrameBuffer(vec4(0.0f));

    // Process input device messages since the last game loop.
    // Update render window size.
    input_.AdvanceFrame(&renderer_.window_size_);

    // Update game logic by a variable number of milliseconds.
    game_state_.AdvanceFrame(delta_time);

    // Output debug information.
    DebugPlayerStates();
    DebugRenderExampleTriangle();

    // Populate 'scene' from the game state--all the positions, orientations,
    // and renderable-ids (which specify materials) of the characters and props.
    // Also specify the camera matrix.
    game_state_.PopulateScene(&scene_);

    // Issue draw calls for the 'scene'.
    Render(scene_);

    // Remember the real-world time from this frame.
    prev_world_time_ = world_time;
  }
}

}  // splat
}  // fpl
