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
#include "splat_common_generated.h"
#include "config_generated.h"
#include "splat_game.h"
#include "splat_rendering_assets_generated.h"
#include "utilities.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace splat {

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
  const Config& config = GetConfig();

  perspective_matrix_ = mat4::Perspective(
      config.viewport_angle(), config.viewport_aspect_ratio(),
      config.viewport_near_plane(), config.viewport_far_plane(), -1.0f);

  auto window_size = config.window_size();
  assert(window_size);
  if (!renderer_.Initialize(LoadVec2i(window_size),
                            config.window_title()->c_str())) {
    fprintf(stderr, "Renderer initialization error: %s\n",
            renderer_.last_error().c_str());
    return false;
  }
  renderer_.color() = vec4(1, 1, 1, 1);
  return true;
}

static Mesh* CreateCardboardMesh(const char* material_file_name,
    MaterialManager* matman, float z_offset) {
  static const unsigned int kCardboardTextureIndex = 0;
  static const float kPixelToWorld = 0.008f;
  static const int kNumVerticesInQuad = 4;
  static const int kNumFloatsPerVertex = 12;
  static Attribute kMeshFormat[] =
      { kPosition3f, kTexCoord2f, kNormal3f, kTangent4f, kEND };
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
   // [x,           y,        z]   [ u,    v]  [normal x, y, z]   [tan x, y, z, handedness]
   -right,        0.0f, z_offset,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,
    right,        0.0f, z_offset,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,
   -right, geo_size[1], z_offset,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f,
    right, geo_size[1], z_offset,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f};
  MATHFU_STATIC_ASSERT(ARRAYSIZE(vertices) ==
                       kNumVerticesInQuad * kNumFloatsPerVertex);

  // Create the mesh and add in the material.
  Mesh* mesh = new Mesh(vertices, kNumVerticesInQuad,
                        sizeof(float) * kNumFloatsPerVertex,
                        kMeshFormat);
  mesh->AddIndices(kMeshIndices, ARRAYSIZE(kMeshIndices), mat);
  return mesh;
}

static bool CheckLengthOfArray(const char* file_name, const char* array_name,
                               int length, int correct_length) {
  if (length == correct_length)
    return true;

  fprintf(stderr, "%s's %s has %d entries, needs %d.\n",
          file_name, array_name, length, correct_length);
  return false;
}

// Load textures for cardboard into 'materials_'. The 'renderer_' and 'matman_'
// members have been initialized at this point.
bool SplatGame::InitializeRenderingAssets() {
  const Config& config = GetConfig();

  // Load the splat asset file.
  static const char* kRenderingAssetsFileName = "splat_rendering_assets.bin";
  if (!flatbuffers::LoadFile(kRenderingAssetsFileName, true,
                             &rendering_assets_source_)) {
    fprintf(stderr, "Can't load %s.\n", kRenderingAssetsFileName);
    return false;
  }

  // Check data validity.
  const RenderingAssets& assets = GetRenderingAssets();
  if (!CheckLengthOfArray(kRenderingAssetsFileName, "cardboard_fronts",
                          assets.cardboard_fronts()->Length(),
                          RenderableId_Count) ||
      !CheckLengthOfArray(kRenderingAssetsFileName, "cardboard_backs",
                          assets.cardboard_backs()->Length(),
                          RenderableId_Count) ||
      !CheckLengthOfArray(kRenderingAssetsFileName, "shadows",
                          assets.shadows()->Length(),
                          RenderableId_Count))
    return false;

  // Create cardboard meshes. First fronts, then backs.
  auto meshes = &cardboard_fronts_;
  auto materials = assets.cardboard_fronts();
  auto z_offset = config.cardboard_front_z_offset();
  for (int j = 0; j < 2; ++j) {

    // Create a mesh for each renderable (front and back).
    for (int i = 0; i < RenderableId_Count; ++i) {
      (*meshes)[i] = CreateCardboardMesh(materials->Get(i)->c_str(),
                                      &matman_, z_offset);
    }

    // Then load cardboard backs.
    meshes = &cardboard_backs_;
    materials = assets.cardboard_backs();
    z_offset = config.cardboard_back_z_offset();
  }

  // We default to the invalid texture, so it has to exist.
  if (!cardboard_fronts_[RenderableId_Invalid]) {
    fprintf(stderr, "Can't load backup texture.\n");
    return false;
  }

  return true;
}

// Create state matchines, characters, controllers, etc. present in
// 'gamestate_'.
bool SplatGame::InitializeGameState() {
  const Config& config = GetConfig();

  game_state_.set_config(&config);

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
  controllers_.resize(config.character_count());
  for (unsigned int i = 0; i < config.character_count(); ++i) {
    controllers_[i].Initialize(
        &input_, ControlScheme::GetDefaultControlScheme(i));
  }

  // Create characters.
  for (unsigned int i = 0; i < config.character_count(); ++i) {
    game_state_.characters().push_back(Character(
        i, &controllers_[i], state_machine_def));
  }

  game_state_.Reset();

  debug_previous_states_.resize(config.character_count(), -1);
  debug_previous_angles_.resize(config.character_count(), Angle(0.0f));

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

  if (!InitializeRenderingAssets())
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

void SplatGame::RenderCardboard(const SceneDescription& scene,
                                const mat4& camera_transform) {
  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];
    const int id = renderable.id();

    // Set up vertex transformation into projection space.
    const mat4 mvp = camera_transform * renderable.world_matrix();
    renderer_.model_view_projection() = mvp;

    // Set the camera and light positions in object space.
    const mat4 world_matrix_inverse = renderable.world_matrix().Inverse();
    renderer_.camera_pos() =
        world_matrix_inverse * game_state_.camera_position();

    // TODO: check amount of lights.
    renderer_.light_pos() = world_matrix_inverse * scene.lights()[0];

    // Draw the front of the character, if we have it.
    // If we don't have it, draw the pajama material for "Invalid".
    Mesh* front = GetCardboardFront(id);
    front->Render(renderer_);

    // If we have a back, draw the back too, slightly offset.
    if (cardboard_backs_[id]) {
      cardboard_backs_[id]->Render(renderer_);
    }
  }
}

void SplatGame::Render(const SceneDescription& scene) {
  const RenderingAssets& assets = GetRenderingAssets();
  const mat4 camera_transform = perspective_matrix_ * scene.camera();

  // Render a ground plane.
  // TODO: Replace with a regular environment prop. Calculate scale_bias from
  // environment prop size.
  renderer_.model_view_projection() = camera_transform;
  auto ground_mat = matman_.LoadMaterial("materials/floor.bin");
  assert(ground_mat);
  ground_mat->Set(renderer_);
  const float gs = 16.4;
  const float txs = 1;
  Mesh::RenderAAQuadAlongX(vec3(-gs, 0, 0), vec3(gs, 0, 8),
                           vec2(0, 0), vec2(txs, txs));
  vec2 scale_bias(txs / gs, -0.5f);

  // Render shadows for all Renderables first, with depth testing off so
  // they blend properly.
  renderer_.DepthTest(false);
  auto shadow_mat = matman_.LoadMaterial("materials/floor_shadows.bin");
  assert(shadow_mat);
  renderer_.model_view_projection() = camera_transform;
  renderer_.light_pos() = scene.lights()[0];  // TODO: check amount of lights.
  shadow_mat->get_shader()->SetUniform("scale_bias", scale_bias);
  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];
    const int id = renderable.id();
    Mesh* front = GetCardboardFront(id);
    if (assets.shadows()->Get(id)) {
      renderer_.model() = renderable.world_matrix();
      // The first texture of the shadow shader has to be that of the billboard.
      shadow_mat->textures()[0] = front->GetMaterial(0)->textures()[0];
      shadow_mat->Set(renderer_);
      front->Render(renderer_, true);
    }
  }
  renderer_.DepthTest(true);

  // Now render the Renderables normally, on top of the shadows.
  RenderCardboard(scene, camera_transform);
}

// Debug function to print out state machine transitions.
void SplatGame::DebugPrintCharacterStates() {
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
}

// Debug function to print out the state of each AirbornePie.
void SplatGame::DebugPrintPieStates() {
  for (unsigned int i = 0; i < game_state_.pies().size(); ++i) {
    AirbornePie& pie = game_state_.pies()[i];
    printf("Pie from [%i]->[%i] w/ %i dmg at pos[%.2f, %.2f, %.2f]\n",
           pie.source(), pie.target(), pie.damage(),
           pie.position().x(), pie.position().y(), pie.position().z());
  }
}

const Config& SplatGame::GetConfig() const {
  return *fpl::splat::GetConfig(config_source_.c_str());
}

const CharacterStateMachineDef* SplatGame::GetStateMachine() const {
  return fpl::splat::GetCharacterStateMachineDef(state_machine_source_.c_str());
}

const RenderingAssets& SplatGame::GetRenderingAssets() const {
  return *fpl::splat::GetRenderingAssets(rendering_assets_source_.c_str());
}


// Debug function to move the camera if the mouse button is down.
void SplatGame::DebugCamera() {
  const Config& config = GetConfig();
  const vec2 mouse_delta = vec2(input_.pointers_[0].mousedelta);

  // Translate the camera in world x, y, z coordinates.
  const bool translate_xz = input_.GetButton(SDLK_POINTER1).is_down();
  const bool translate_y = input_.GetButton(SDLK_POINTER2).is_down();
  if (translate_xz || translate_y) {
    const vec3 camera_delta = vec3(
        translate_xz ? mouse_delta.x() * config.mouse_to_ground_scale() : 0.0f,
        translate_y  ? mouse_delta.x() * config.mouse_to_height_scale() : 0.0f,
        translate_xz ? mouse_delta.y() * config.mouse_to_ground_scale() : 0.0f);
    const vec3 new_position = game_state_.camera_position() + camera_delta;
    game_state_.set_camera_position(new_position);

#   ifdef SPLAT_DEBUG_CAMERA
      printf("camera position (%.5ff, %.5ff, %.5ff)\n",
             new_position[0], new_position[1], new_position[2]);
#   endif
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
    const float scale = dist * config.mouse_to_camera_rotation_scale();
    const vec3 unscaled_delta = mouse_delta.x() * side + mouse_delta.y() * up;
    const vec3 target_delta = scale * unscaled_delta;
    const vec3 new_target = game_state_.camera_target() + target_delta;
    game_state_.set_camera_target(new_target);

#   ifdef SPLAT_DEBUG_CAMERA
      printf("camera target (%.5ff, %.5ff, %.5ff)\n",
             new_target[0], new_target[1], new_target[2]);
#   endif
  }
}

void SplatGame::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  const Config& config = GetConfig();
  const WorldTime min_update_time = config.min_update_time();
  const WorldTime max_update_time = config.min_update_time();
  prev_world_time_ = CurrentWorldTime() - min_update_time;

  while (!input_.exit_requested_ &&
         !input_.GetButton(SDLK_ESCAPE).went_down()) {
    // Milliseconds elapsed since last update. To avoid burning through the CPU,
    // enforce a minimum time between updates. For example, if min_update_time
    // is 1, we will not exceed 1000Hz update time.
    const WorldTime world_time = CurrentWorldTime();
    const WorldTime delta_time = std::min(world_time - prev_world_time_,
                                          max_update_time);
    if (delta_time < min_update_time) {
      SleepForMilliseconds(min_update_time - delta_time);
      continue;
    }

    // TODO: Can we move these to 'Render'?
    renderer_.AdvanceFrame(input_.minimized_);
    renderer_.ClearFrameBuffer(vec4(0.0f));

    // Process input device messages since the last game loop.
    // Update render window size.
    input_.AdvanceFrame(&renderer_.window_size());

    // Update game logic by a variable number of milliseconds.
    game_state_.AdvanceFrame(delta_time);

    // Output debug information.
    if (config.print_character_states()) {
      DebugPrintCharacterStates();
    }
    if (config.print_pie_states()) {
      DebugPrintPieStates();
    }
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
