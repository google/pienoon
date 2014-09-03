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
#include "audio_config_generated.h"
#include "config_generated.h"
#include "splat_game.h"
#include "utilities.h"
#include "audio_engine.h"

using mathfu::vec2i;
using mathfu::vec2;
using mathfu::vec3;
using mathfu::vec4;
using mathfu::mat4;

namespace fpl {
namespace splat {

struct CardboardVertex {
  float x, y, z;
  float u, v;
  float normal_x, normal_y, normal_z;
  float tangent_x, tangent_y, tangent_z;
  float handedness;

  void SetPosition(float position_x, float position_y, float position_z) {
    x = position_x;
    y = position_y;
    z = position_z;
  }
};

static const int kQuadNumVertices = 4;
static const int kQuadNumIndices = 6;

static const CardboardVertex kQuadUnpositionedVertices[] = {
    // [x,   y,   z]    [ u,    v]   [normal x, y, z]  [tan x, y, z, handedness]
    {0.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f},
};

static const int kQuadIndices[] = { 0, 1, 2, 2, 1, 3 };

static const Attribute kQuadMeshFormat[] =
    { kPosition3f, kTexCoord2f, kNormal3f, kTangent4f, kEND };

static const char kAssetsDir[] = "assets";
static const char *kBuildPaths[] = {
    "Debug", "Release", "projects\\VisualStudio2010", "build\\Debug\\bin",
    "build\\Release\\bin"};

static const char kConfigFileName[] = "config.bin";

// Return the elapsed milliseconds since the start of the program. This number
// will loop back to 0 after about 49 days; always take the difference to
// properly handle the wrap-around case.
static inline WorldTime CurrentWorldTime() {
  return SDL_GetTicks();
}

SplatGame::SplatGame()
    : state_(kUninitialized),
      state_entry_time_(0),
      matman_(renderer_),
      cardboard_fronts_(RenderableId_Count, nullptr),
      cardboard_backs_(RenderableId_Count, nullptr),
      stick_front_(nullptr),
      stick_back_(nullptr),
      prev_world_time_(0),
      debug_previous_states_(),
      debug_previous_angles_() {
}

SplatGame::~SplatGame() {
  for (int i = 0; i < RenderableId_Count; ++i) {
    delete cardboard_fronts_[i];
    cardboard_fronts_[i] = nullptr;

    delete cardboard_backs_[i];
    cardboard_backs_[i] = nullptr;
  }

  Mix_CloseAudio();

  delete stick_front_;
  stick_front_ = nullptr;

  delete stick_back_;
  stick_back_ = nullptr;
}

bool SplatGame::InitializeConfig() {
  if (!LoadFile(kConfigFileName, &config_source_)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "can't load config.bin\n");
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
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Renderer initialization error: %s\n",
            renderer_.last_error().c_str());
    return false;
  }
  renderer_.color() = mathfu::kOnes4f;
  return true;
}

// Initializes 'vertices' at the specified position, aligned up-and-down.
// 'vertices' must be an array of length kQuadNumVertices.
static void CreateVerticalQuad(float left, float right, float bottom, float top,
                               float depth, CardboardVertex* vertices) {
  MATHFU_STATIC_ASSERT(ARRAYSIZE(kQuadUnpositionedVertices) ==
                       kQuadNumVertices);
  memcpy(vertices, kQuadUnpositionedVertices,
         sizeof(kQuadUnpositionedVertices));
  vertices[0].SetPosition(left, bottom, depth);
  vertices[1].SetPosition(right, bottom, depth);
  vertices[2].SetPosition(left, top, depth);
  vertices[3].SetPosition(right, top, depth);
}

// Creates a mesh of a single quad (two triangles) vertically upright.
// The quad's has x and y size determined by the size of the texture.
// The quad is offset in (x,y,z) space by the 'offset' variable.
// Returns a mesh with the quad and texture, or nullptr if anything went wrong.
Mesh* SplatGame::CreateVerticalQuadMesh(
    const flatbuffers::String* material_name, const vec3& offset) {
  const Config& config = GetConfig();

  // Don't try to load obviously invalid materials. Suppresses error logs from
  // the material manager.
  if (material_name == nullptr || material_name->c_str()[0] == '\0')
    return nullptr;

  // Load the material from file, and check validity.
  Material* material = matman_.LoadMaterial(material_name->c_str());
  bool material_valid = material != nullptr && material->textures().size() > 0;
  if (!material_valid)
    return nullptr;

  // Create vertex geometry in proportion to the texture size.
  // This is nice for the artist since everything is at the scale of the
  // original artwork.
  const Texture* front_texture = material->textures()[0];
  const vec2 im(front_texture->size);
  const vec2 geo_size = im * vec2(config.pixel_to_world_scale());
  const float half_width = geo_size[0] * 0.5f;

  // Initialize a vertex array in the requested position.
  CardboardVertex vertices[kQuadNumVertices];
  CreateVerticalQuad(offset[0] - half_width, offset[0] + half_width,
                     offset[1], offset[1] + geo_size[1],
                     offset[2], vertices);

  // Create mesh and add in quad indices.
  Mesh* mesh = new Mesh(vertices, kQuadNumVertices, sizeof(CardboardVertex),
                        kQuadMeshFormat);
  mesh->AddIndices(kQuadIndices, kQuadNumIndices, material);
  return mesh;
}

// Load textures for cardboard into 'materials_'. The 'renderer_' and 'matman_'
// members have been initialized at this point.
bool SplatGame::InitializeRenderingAssets() {
  const Config& config = GetConfig();

  // Check data validity.
  if (config.renderables()->Length() != RenderableId_Count) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "%s's 'renderables' array has %d entries, needs %d.\n",
                 kConfigFileName, config.renderables()->Length(),
                 RenderableId_Count);
    return false;
  }

  // Create a mesh for the front and back of each cardboard cutout.
  const vec3 front_z_offset(0.0f, 0.0f, config.cardboard_front_z_offset());
  const vec3 back_z_offset(0.0f, 0.0f, config.cardboard_back_z_offset());
  for (int id = 0; id < RenderableId_Count; ++id) {
    auto renderable = config.renderables()->Get(id);
    const vec3 offset = renderable->offset() == nullptr ? mathfu::kZeros3f :
                        LoadVec3(renderable->offset());
    const vec3 front_offset = offset + front_z_offset;
    const vec3 back_offset = offset + back_z_offset;

    cardboard_fronts_[id] = CreateVerticalQuadMesh(
        renderable->cardboard_front(), front_offset);

    cardboard_backs_[id] = CreateVerticalQuadMesh(
        renderable->cardboard_back(), back_offset);
  }

  // We default to the invalid texture, so it has to exist.
  if (!cardboard_fronts_[RenderableId_Invalid]) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't load backup texture.\n");
    return false;
  }

  // Create stick front and back meshes.
  const vec3 stick_front_offset(0.0f, config.stick_y_offset(),
                                config.stick_front_z_offset());
  const vec3 stick_back_offset(0.0f, config.stick_y_offset(),
                               config.stick_back_z_offset());
  stick_front_ = CreateVerticalQuadMesh(config.stick_front(),
                                        stick_front_offset);
  stick_back_ = CreateVerticalQuadMesh(config.stick_back(), stick_back_offset);

  return true;
}

// Create state matchines, characters, controllers, etc. present in
// 'gamestate_'.
bool SplatGame::InitializeGameState() {
  const Config& config = GetConfig();

  game_state_.set_config(&config);

  // Load flatbuffer into buffer.
  if (!LoadFile("character_state_machine_def.bin", &state_machine_source_)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Error loading character state machine.\n");
    return false;
  }

  // Grab the state machine from the buffer.
  auto state_machine_def = GetStateMachine();
  if (!CharacterStateMachineDef_Validate(state_machine_def)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "State machine is invalid.\n");
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

  debug_previous_states_.resize(config.character_count(), -1);
  debug_previous_angles_.resize(config.character_count(), Angle(0.0f));

  return true;
}

// Initialize each member in turn. This is logically just one function, since
// the order of initialization cannot be changed. However, it's nice for
// debugging and readability to have each section lexographically separate.
bool SplatGame::Initialize() {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Splat initializing...\n");

  if (!ChangeToUpstreamDir(kAssetsDir, kBuildPaths, ARRAYSIZE(kBuildPaths)))
    return false;

  if (!InitializeConfig())
    return false;

  if (!InitializeRenderer())
    return false;

  if (!InitializeRenderingAssets())
    return false;

  // Some people are having trouble loading the audio engine, and it's not
  // strictly necessary for gameplay, so don't die if the audio engine fails to
  // initialize.
  audio_engine_.Initialize(GetConfig().audio());

  if (!InitializeGameState())
    return false;

# ifdef PLATFORM_MOBILE
  if (!gpg_manager.Initialize())
    return false;
# endif

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Splat initialization complete\n");
  return true;
}

// Returns the mesh for renderable_id, if we have one, or the pajama mesh
// (a mesh with a texture that's obviously wrong), if we don't.
Mesh* SplatGame::GetCardboardFront(int renderable_id) {
  const bool is_valid_id = 0 <= renderable_id &&
                           renderable_id < RenderableId_Count &&
                           cardboard_fronts_[renderable_id] != nullptr;
  return is_valid_id ? cardboard_fronts_[renderable_id]
                     : cardboard_fronts_[RenderableId_Invalid];
}

void SplatGame::RenderCardboard(const SceneDescription& scene,
                                const mat4& camera_transform) {
  const Config& config = GetConfig();

  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];
    const int id = renderable.id();

    // Set up vertex transformation into projection space.
    const mat4 mvp = camera_transform * renderable.world_matrix();
    renderer_.model_view_projection() = mvp;

    // Set the camera and light positions in object space.
    const mat4 world_matrix_inverse = renderable.world_matrix().Inverse();
    renderer_.camera_pos() = world_matrix_inverse *
                             game_state_.camera_position();

    // TODO: check amount of lights.
    renderer_.light_pos() = world_matrix_inverse * scene.lights()[0];

    // Note: Draw order is back-to-front, so draw the cardboard back, then
    // popsicle stick, then cardboard front--in that order.
    //
    // If we have a back, draw the back too, slightly offset.
    // The back is the *inside* of the cardboard, representing corrugation.
    if (cardboard_backs_[id]) {
      cardboard_backs_[id]->Render(renderer_);
    }

    // Draw the popsicle stick that props up the cardboard.
    if (config.renderables()->Get(id)->stick() && stick_front_ != nullptr &&
        stick_back_ != nullptr) {
      stick_front_->Render(renderer_);
      stick_back_->Render(renderer_);
    }

    // Draw the front of the cardboard.
    Mesh* front = GetCardboardFront(id);
    front->Render(renderer_);
  }
}

void SplatGame::Render(const SceneDescription& scene) {
  const Config& config = GetConfig();
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
    if (config.renderables()->Get(id)->shadow()) {
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
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                   "character %d - Health %2d, State %s [%d]\n",
              i, character.health(), EnumNameStateId(id), id);
      debug_previous_states_[i] = id;
    }

    // Report face angle changes.
    if (debug_previous_angles_[i] != character.face_angle()) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                   "character %d - face error %.0f(%.0f) - target %d\n",
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
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                 "Pie from [%i]->[%i] w/ %i dmg at pos[%.2f, %.2f, %.2f]\n",
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

// Debug function to move the camera if the mouse button is down.
void SplatGame::MoveCamera() {
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

    if (config.print_camera_orientation()) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                   "camera position (%.5ff, %.5ff, %.5ff)\n",
                   new_position[0], new_position[1], new_position[2]);
    }
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

    if (config.print_camera_orientation()) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                   "camera target (%.5ff, %.5ff, %.5ff)\n",
                   new_target[0], new_target[1], new_target[2]);
    }
  }
}

SplatState SplatGame::CalculateSplatState() const {
  const Config& config = GetConfig();

  switch (state_) {
    case kPlaying:
      // When we're down to one or zero active characters, the game's over.
      if (game_state_.NumActiveCharacters() <= 1)
        return kFinished;
      break;

    case kFinished: {
      // Reset after a certain amount of time has passed and someone presses
      // the throw key.
      const WorldTime min_finished_time =
          state_entry_time_ + config.play_finished_timeout();
      if (prev_world_time_ >= min_finished_time &&
          (game_state_.AllLogicalInputs() & LogicalInputs_ThrowPie) != 0)
        return kPlaying;
      break;
    }

    default:
      assert(false);
  }
  return state_;
}

void SplatGame::TransitionToSplatState(SplatState next_state) {
  assert(state_ != next_state); // Must actually transition.

  switch (next_state) {
    case kPlaying:
      game_state_.Reset();
      break;

    case kFinished:
      for (auto it = game_state_.characters().begin();
               it != game_state_.characters().end(); ++it) {
        if (it->health() > 0) {
          it->IncrementStat(kWins);
        } else {
          // TODO: this does not account for draws.
          it->IncrementStat(kLosses);
        }
      }
      UploadStats();
      break;

    default:
      assert(false);
  }

  state_ = next_state;
  state_entry_time_ = prev_world_time_;
}

void SplatGame::UploadStats() {
#   ifdef PLATFORM_MOBILE
    static const char *leaderboard_ids[] = {
      "CgkI97yope0IEAIQAw",  // kWins
      "CgkI97yope0IEAIQBA",  // kLosses
      "CgkI97yope0IEAIQBQ",  // kDraws
      "CgkI97yope0IEAIQAg",  // kAttacks
      "CgkI97yope0IEAIQBg",  // kHits
      "CgkI97yope0IEAIQBw",  // kBlocks
      "CgkI97yope0IEAIQCA",  // kMisses
    };
    static_assert(sizeof(leaderboard_ids) / sizeof(const char *) ==
                  kMaxStats, "update leaderboard_ids");
    // Now upload all stats:
    // TODO: this assumes player 0 == the logged in player.
    for (int ps = kWins; ps < kMaxStats; ps++) {
      gpg_manager.SaveStat(leaderboard_ids[ps],
        game_state_.characters()[0].GetStat(static_cast<PlayerStats>(ps)));
    }
#   endif
}

void SplatGame::Run() {
  // Initialize so that we don't sleep the first time through the loop.
  const Config& config = GetConfig();
  const WorldTime min_update_time = config.min_update_time();
  const WorldTime max_update_time = config.min_update_time();
  prev_world_time_ = CurrentWorldTime() - min_update_time;
  TransitionToSplatState(kPlaying);

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
    renderer_.ClearFrameBuffer(mathfu::kZeros4f);

    // Process input device messages since the last game loop.
    // Update render window size.
    input_.AdvanceFrame(&renderer_.window_size());

    // Update game logic by a variable number of milliseconds.
    game_state_.AdvanceFrame(delta_time, &audio_engine_);

    // Populate 'scene' from the game state--all the positions, orientations,
    // and renderable-ids (which specify materials) of the characters and props.
    // Also specify the camera matrix.
    game_state_.PopulateScene(&scene_);

    // Issue draw calls for the 'scene'.
    Render(scene_);

    // Output debug information.
    if (config.print_character_states()) {
      DebugPrintCharacterStates();
    }
    if (config.print_pie_states()) {
      DebugPrintPieStates();
    }
    if (config.allow_camera_movement()) {
      MoveCamera();
    }

    // Remember the real-world time from this frame.
    prev_world_time_ = world_time;

    // Advance to the next play state, if required.
    const SplatState next_state = CalculateSplatState();
    if (next_state != state_) {
      TransitionToSplatState(next_state);
    }

#   ifdef PLATFORM_MOBILE
    // Since we don't have a UI nor any gameplay on Android, for testing,
    // we'll check if a third finger went down on the touch screen,
    // if so we update the leaderboards and show the UI:
    if (input_.GetButton(SDLK_POINTER3).went_down()) {
      // For testing, increase stat:
      game_state_.characters()[0].IncrementStat(kAttacks);
      UploadStats();
      // For testing, show UI:
      gpg_manager.ShowLeaderboards();
    }
    gpg_manager.Update();
#   endif
  }
}

}  // splat
}  // fpl
