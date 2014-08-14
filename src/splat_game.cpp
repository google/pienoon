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
#include "config_generated.h"
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


static const float kViewportAngle = kHalfPi * 0.5f;
static const float kViewportAspectRatio = 1280.0f / 800.0f;
static const float kViewportNearPlane = 1.0f;
static const float kViewportFarPlane = 100.0f;
static const mat4 kViewportPerspective(
                      mat4::Perspective(kViewportAngle,
                                        kViewportAspectRatio,
                                        kViewportNearPlane,
                                        kViewportFarPlane, -1.0f));

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
    : matman_(renderer_),
      cardboard_fronts_(RenderableId_Count, NULL),
      cardboard_backs_(RenderableId_Count, NULL),
      prev_world_time_(0),
      debug_previous_states_(),
      debug_previous_angles_() {
}

SplatGame::~SplatGame() {
  for (int i = 0; i < RenderableId_Count; ++i) {
    delete cardboard_fronts_[i];
    cardboard_fronts_[i] = NULL;

    delete cardboard_backs_[i];
    cardboard_backs_[i] = NULL;
  }
}

bool SplatGame::InitializeConfig() {
  if (!flatbuffers::LoadFile("config.bin", true, &config_source_)) {
    fprintf(stderr, "can't load config.bin\n");
    return false;
  }
  return true;
}

// Initialize the 'renderer_' member. No other members have been initialized at
// this point.
bool SplatGame::InitializeRenderer() {
  const Config* config = GetConfig();
  if (!renderer_.Initialize(vec2i(config->window_size()->x(),
                                  config->window_size()->y()),
                            config->window_title()->c_str())) {
    fprintf(stderr, "Renderer initialization error: %s\n",
            renderer_.last_error_.c_str());
    return false;
  }
  renderer_.color = vec4(1, 1, 1, 1);
  return true;
}

static Mesh* CreateCardboardMesh(const char* material_file_name,
    MaterialManager* matman, float z_offset) {
  static const unsigned int kCardboardTextureIndex = 0;
  static const float kPixelToWorld = 0.008f;
  static const int kNumVerticesInQuad = 4;
  static const int kNumFloatsPerVertex = 5;
  static Attribute kMeshFormat[] = { kPosition3f, kTexCoord2f, kEND };
  static int kMeshIndices[] = { 0, 1, 2, 2, 1, 3 };

  // Load the material and check its validity.
  Material* mat = matman->LoadMaterial(material_file_name);
  if (mat == NULL || mat->textures().size() <= kCardboardTextureIndex)
    return NULL;

  // Create vertex geometry in proportion to the texture size.
  const Texture* cardboard_texture = mat->textures()[kCardboardTextureIndex];
  const vec2 im(cardboard_texture->size);
  const vec2 geo_size = im * vec2(kPixelToWorld);
  const float right = geo_size[0] * 0.5f;
  const float vertices[] = {
      // [x,            y,        z]    [   u,    v]
      -right,        0.0f, z_offset,     0.0f, 0.0f,
       right,        0.0f, z_offset,     1.0f, 0.0f,
      -right, geo_size[1], z_offset,     0.0f, 1.0f,
       right, geo_size[1], z_offset,     1.0f, 1.0f };
  STATIC_ASSERT(ARRAYSIZE(vertices) ==
                kNumVerticesInQuad * kNumFloatsPerVertex);

  // Create the mesh and add in the material.
  Mesh* mesh = new Mesh(vertices, kNumVerticesInQuad,
                        sizeof(float) * kNumFloatsPerVertex,
                        kMeshFormat);
  mesh->AddIndices(kMeshIndices, ARRAYSIZE(kMeshIndices), mat);
  return mesh;
}

// Load textures for cardboard into 'materials_'. The 'renderer_' and 'matman_'
// members have been initialized at this point.
bool SplatGame::InitializeMaterials() {
  static const float kCardboardFrontZOffset = 0.0f;
  static const float kCardboardBackZOffset = -0.15f;

  // Load cardboard fronts.
  for (int i = 0; i < RenderableId_Count; ++i) {
    const std::string material_file_name = FileNameFromEnumName(
        EnumNameRenderableId(i), "materials/", ".bin");
    cardboard_fronts_[i] = CreateCardboardMesh(
        material_file_name.c_str(), &matman_, kCardboardFrontZOffset);
  }

  // Load cardboard backs.
  for (int i = 0; i < RenderableId_Count; ++i) {
    const std::string material_file_name = FileNameFromEnumName(
        EnumNameRenderableId(i), "materials/", "_back.bin");
    cardboard_backs_[i] = CreateCardboardMesh(
        material_file_name.c_str(), &matman_, kCardboardBackZOffset);
  }

  return true;
}

// Calculate a character's target at the start of the game. We want the
// characters to aim at the character directly opposite them.
static CharacterId InitialTargetId(const CharacterId id,
                                   const int character_count) {
  return static_cast<CharacterId>(
      (id + character_count / 2) % character_count);
}

// The position of a character is at the start of the game.
static mathfu::vec3 InitialPosition(const CharacterId id,
                                    const int character_count) {
  static const float kCharacterDistFromCenter = 7.0f;
  const Angle angle_to_position = Angle::FromWithinThreePi(
      static_cast<float>(id) * kTwoPi / character_count);
  return kCharacterDistFromCenter * angle_to_position.ToXZVector();
}

// Calculate the direction a character is facing at the start of the game.
// We want the characters to face their initial target.
static Angle InitialFaceAngle(const CharacterId id, const int character_count) {
  return Angle::FromWithinThreePi(kTwoPi * id / character_count);
  const mathfu::vec3 characterPosition = InitialPosition(id, character_count);
  const mathfu::vec3 targetPosition =
      InitialPosition(InitialTargetId(id, character_count), character_count);
  return Angle::FromXZVector(targetPosition - characterPosition);
}

// Create state matchines, characters, controllers, etc. present in
// 'gamestate_'.
bool SplatGame::InitializeGameState() {
  const Config* config = GetConfig();

  game_state_.set_config(config);

  // Load flatbuffer into buffer.
  if (!flatbuffers::LoadFile("character_state_machine_def.bin",
                             true, &state_machine_source_)) {
    fprintf(stderr, "Error loading character state machine.\n");
    return false;
  }

  // Grab the state machine from the buffer.
  auto state_machine_def = GetStateMachine();
  if (!CharacterStateMachineDef_Validate(state_machine_def)) {
    fprintf(stderr, "State machine is invalid.\n");
    return false;
  }

  // Create controllers.
  controllers_.resize(config->character_count());
  for (CharacterId id = 0; id < config->character_count(); ++id) {
    controllers_[id].Initialize(
        &input_, ControlScheme::GetDefaultControlScheme(id));
  }

  // Create characters.
  for (CharacterId id = 0; id < config->character_count(); ++id) {
    game_state_.characters().push_back(Character(
        id, InitialTargetId(id, config->character_count()),
        config->character_health(),
        InitialFaceAngle(id, config->character_count()),
        InitialPosition(id, config->character_count()),
        &controllers_[id], state_machine_def));
  }

  debug_previous_states_.resize(config->character_count(), -1);
  debug_previous_angles_.resize(config->character_count(), Angle(0.0f));

  return true;
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool SplatGame::Initialize() {
  printf("Splat initializing...\n");

  if (!ChangeToUpstreamDir(kAssetsDir, kBuildPaths, ARRAYSIZE(kBuildPaths)))
    return false;

  if (!InitializeConfig())
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

Mesh* SplatGame::GetCardboardFront(int renderable_id) {
  const bool is_valid_id = 0 <= renderable_id &&
                           renderable_id < RenderableId_Count &&
                           cardboard_fronts_[renderable_id] != NULL;
  return is_valid_id ? cardboard_fronts_[renderable_id]
                     : cardboard_fronts_[RenderableId_Invalid];
}

void SplatGame::Render(const SceneDescription& scene) {
  const mat4 camera_transform = kViewportPerspective * scene.camera();

  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];

    const mat4 mvp = camera_transform * renderable.world_matrix();
    renderer_.camera.model_view_projection_ = mvp;

    // Draw the front of the character, if we have it.
    // If we don't have it, draw the pajama material for "Invalid".
    const int id = renderable.id();
    Mesh* front = GetCardboardFront(id);
    front->Render(renderer_);

    // If we have a back, draw the back too, slightly offset.
    if (cardboard_backs_[id]) {
      cardboard_backs_[id]->Render(renderer_);
    }
  }
}

// Debug function to write out state machine transitions.
// TODO: Remove this block and the one in the main loop that prints the
// current state.
void SplatGame::DebugCharacterStates() {
  // Display the state changes, at least until we get real rendering up.
  for (unsigned int i = 0; i < game_state_.characters().size(); ++i) {
    auto& character = game_state_.characters()[i];
    int id = character.state_machine()->current_state()->id();
    if (debug_previous_states_[i] != id) {
      printf("character %d - Health %2d, State %s [%d]\n",
              i, character.health(), EnumNameStateId(id), id);
      debug_previous_states_[i] = id;
    }

    // Report face angle changes.
    if (debug_previous_angles_[i] != character.face_angle()) {
      printf("character %d - face error %.0f(%.0f) - target %d\n",
          i, game_state_.FaceAngleError(i).ToDegrees(),
          game_state_.TargetFaceAngle(i).ToDegrees(),
          character.target());
      debug_previous_angles_[i] = character.face_angle();
    }
  }

  for (unsigned int i = 0; i < game_state_.pies().size(); ++i) {
    AirbornePie& pie = game_state_.pies()[i];
    printf("Pie from [%i]->[%i] w/ %i dmg at pos[%.2f, %.2f, %.2f]\n",
           pie.source(), pie.target(), pie.damage(),
           pie.position().x(), pie.position().y(), pie.position().z());
  }
}

const Config* SplatGame::GetConfig() const {
  return fpl::splat::GetConfig(config_source_.c_str());
}

const CharacterStateMachineDef* SplatGame::GetStateMachine() const {
  return fpl::splat::GetCharacterStateMachineDef(state_machine_source_.c_str());
}

// Debug function to move the camera if the mouse button is down.
void SplatGame::DebugCamera() {
  static const float kMouseToXZTranslationScale = 0.005f;
  static const float kMouseToYTranslationScale = 0.0025f;
  static const float kMouseToCameraRotationScale = 0.001f;

  const vec2 mouse_delta = vec2(input_.pointers_[0].mousedelta);

  // Translate the camera in world x, y, z coordinates.
  const bool translate_xz = input_.GetButton(SDLK_POINTER1).is_down();
  const bool translate_y = input_.GetButton(SDLK_POINTER2).is_down();
  if (translate_xz || translate_y) {
    const vec3 camera_delta = vec3(
          translate_xz ? mouse_delta.x() * kMouseToXZTranslationScale : 0.0f,
          translate_y  ? mouse_delta.x() * kMouseToYTranslationScale  : 0.0f,
          translate_xz ? mouse_delta.y() * kMouseToXZTranslationScale : 0.0f);
    const vec3 new_position = game_state_.camera_position() + camera_delta;
    game_state_.set_camera_position(new_position);

    printf("camera position (%.5ff, %.5ff, %.5ff)\n",
           new_position[0], new_position[1], new_position[2]);
  }

  // Move the camera target in the camera plane.
  const bool rotate = input_.GetButton(SDLK_POINTER3).is_down();
  if (rotate) {
    // Get axes of camera space.
    const vec3 up(0.0f, 1.0f, 0.0f);
    vec3 forward = game_state_.camera_target() - game_state_.camera_position();
    const float dist = forward.Normalize();
    const vec3 side = vec3::CrossProduct(up, forward);

    // Apply mouse movement along up and side axes. Scale so that no matter
    // distance, the same angle is applied.
    const float scale = dist * kMouseToCameraRotationScale;
    const vec3 unscaled_delta = mouse_delta.x() * side + mouse_delta.y() * up;
    const vec3 target_delta = scale * unscaled_delta;
    const vec3 new_target = game_state_.camera_target() + target_delta;
    game_state_.set_camera_target(new_target);

    printf("camera target (%.5ff, %.5ff, %.5ff)\n",
           new_target[0], new_target[1], new_target[2]);
  }
}

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
    DebugCharacterStates();
    DebugCamera();

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

