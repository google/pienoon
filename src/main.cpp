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
#include "character_state_machine.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "game_state.h"
#include "input.h"
#include "material_manager.h"
#include "render_scene.h"
#include "renderer.h"
#include "sdl_controller.h"
#include "timeline_generated.h"
#include "angle.h"

namespace fpl
{

static const float kViewportAngle = 55.0;
static const float kViewportAspectRatio = 1280.0/800.0;
static const float kViewportNearPlane =10;
static const float kViewportFarPlane = 50;

static const mat4 kRotate180AroundZ =  mat4(-1, 0, 0, 0,
                                             0,-1, 0, 0,
                                             0, 0, 1, 0,
                                             0, 0, 0, 1);

void RenderSceneFromDescription(Renderer& renderer,
                                const std::vector<Material*> materials,
                                const RenderScene& scene)
{
  for (size_t i = 0; i < scene.renderables().size(); ++i) {
    const Renderable& renderable = scene.renderables()[i];
    const Material* mat = materials[renderable.id()];
    (void)mat;
    (void)renderer;
    // TODO: Draw carboard with texture from 'mat' at location
    // renderable.matrix_
  }
}

// Try to change into the assets directory when running the executable from
// the build path.
#if defined(_WIN32)
inline char* getcwd(char *buffer, int maxlen) {
  return _getcwd(buffer, maxlen);
}

inline int chdir(const char *dirname) {
  return _chdir(dirname);
}
#endif  // defined(_WIN32)

static bool ChangeToAssetsDir() {
#if !defined(__ANDROID__)
  {
    static const char kAssetsDir[] = "assets";
    static const char *kBuildPaths[] = {
        "Debug", "Release", "projects\\VisualStudio2010", "build\\Debug\\bin",
        "build\\Release\\bin"};
    char path[256];
    char *dir = getcwd(path, sizeof(path));
    assert(dir);

    // Return if we're already in the assets directory.
    const char* last_slash = strrchr(path, flatbuffers::kPathSeparator);
    const char* last_dir = last_slash ? last_slash + 1 : path;
    if (strcmp(last_dir, kAssetsDir) == 0)
      return true;

    // Change to the directory one above the assets directory.
    const size_t len_dir = strlen(dir);
    int success;
    for (size_t i = 0; i < ARRAYSIZE(kBuildPaths); ++i) {
      const size_t len_build_path = strlen(kBuildPaths[i]);
      char* beyond_base = &dir[len_dir - len_build_path];
      if (strcmp(beyond_base, kBuildPaths[i]) != 0)
        continue;

      *beyond_base = '\0';
      success = chdir(dir);
      assert(success == 0);
      break;
    }

    // Change into assets directory.
    success = chdir(kAssetsDir);
    if (success != 0) {
      fprintf(stderr, "Unable to change into %s dir\n", kAssetsDir);
      return false;
    }
  }
#endif // !defined(__ANDROID__)
  return true;
}

static inline bool IsUpperCase(const char c) {
  return c == toupper(c);
}

static std::string CamelCaseToSnakeCase(const char* const camel) {
  // Replace capitals with underbar + lowercase.
  std::string snake;
  for (const char* c = camel; *c != '\0'; ++c) {
    if (IsUpperCase(*c)) {
      const bool is_start_or_end = c == camel || *(c + 1) == '\0';
      if (!is_start_or_end) {
        snake += '_';
      }
      snake += static_cast<char>(tolower(*c));
    } else {
      snake += *c;
    }
  }
  return snake;
}

static std::string FileNameFromEnumName(const char* const enum_name,
                                        const char* const prefix,
                                        const char* const suffix) {
  // Skip over the initial 'k', if it exists.
  const bool starts_with_k = enum_name[0] == 'k' && IsUpperCase(enum_name[1]);
  const char* const camel_case_name = starts_with_k ? enum_name + 1 : enum_name;

  // Assemble file name.
  return std::string(prefix)
       + CamelCaseToSnakeCase(camel_case_name)
       + std::string(suffix);
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
static Angle InitialFaceAngle(const splat::CharacterId id) {
  const mathfu::vec3 characterPosition = InitialPosition(id);
  const mathfu::vec3 targetPosition = InitialPosition(InitialTargetId(id));
  return Angle::FromXZVector(targetPosition - characterPosition);
}

int MainLoop() {
  // Time consumed when GameState::AdvanceFrame is called.
  // At some point this might be variable.
  static const WorldTime kDeltaTime = 1;

  printf("Splat initializing..\n");
  if (!ChangeToAssetsDir()) return 1;

  InputSystem input;

  Renderer renderer;
  MaterialManager matman(renderer);

  renderer.Initialize(vec2i(1280, 800), "my amazing game!");

  std::vector<Material*> materials;
  for (int i = 0; i < RenderableId_Count; ++i) {
    const std::string material_file_name = FileNameFromEnumName(
                                              EnumNameRenderableId(i),
                                              "materials/", ".bin");
    Material* mat = matman.LoadMaterial(material_file_name.c_str());
    if (!mat) {
      fprintf(stderr, "load error: %s\n", renderer.last_error_.c_str());
      return 1;
    }
    materials.push_back(mat);
  }

  vec3 cameraPosition = vec3(0, 0, -25);

  renderer.camera.model_view_projection_ =
      mat4::Perspective(kViewportAngle,
                        kViewportAspectRatio,
                        kViewportNearPlane,
                        kViewportFarPlane) *
      kRotate180AroundZ *
      mat4::FromTranslationVector(cameraPosition);

  renderer.color = vec4(1, 1, 1, 1);

  Attribute format[] = { kPosition3f, kTexCoord2f, kEND };
  int indices[] = { 0, 1, 2, 3};
  float vertices[] = { -1, 0, 0,   0, 0,
                        1, 0, 0,   1, 0,
                       -1, 3, 0,   0, 1,
                        1, 3, 0,   1, 1};

  std::string state_machine_source;
  if (!flatbuffers::LoadFile("character_state_machine_def.bin",
                             true,
                             &state_machine_source)) {
    printf("can't load character state machines\n");
    return 1;
  }

  auto state_machine_def =
     splat::GetCharacterStateMachineDef(state_machine_source.c_str());
  CharacterStateMachineDef_Validate(state_machine_def);

  splat::GameState game_state;

  splat::SdlController* controllers[splat::kPlayerCount];
  for (int i = 0; i < splat::kPlayerCount; i++) {
    controllers[i] = new splat::SdlController(
        &input, splat::ControlScheme::GetDefaultControlScheme(i));
  }
  for (splat::CharacterId id = 0; id < splat::kPlayerCount; id++) {
    game_state.characters().push_back(splat::Character(
        id, InitialTargetId(id), splat::kDefaultHealth, InitialFaceAngle(id),
        InitialPosition(id), controllers[id], state_machine_def));
  }

  // TODO: Remove this block and the one in the main loop that prints the
  // current state.
  // This is just for development. It keeps track of when a state machine
  // transitions so that we can print the change. Printing every frame would be
  // spammy.
  std::vector<int> previous_states(splat::kPlayerCount, -1);

  RenderScene scene;
  while (!input.exit_requested_ &&
         !input.GetButton(SDLK_ESCAPE).went_down()) {
    renderer.AdvanceFrame(input.minimized_);
    renderer.ClearFrameBuffer(vec4(0.0f));
    input.AdvanceFrame(&renderer.window_size_);
    game_state.AdvanceFrame(kDeltaTime);

    // Display the state changes, at least until we get real rendering up.
    for (int i = 0; i < splat::kPlayerCount; i++) {
      auto& player = game_state.characters()[i];
      int id = player.state_machine()->current_state()->id();
      if (previous_states[i] != id) {
        printf("Player %d - Health %2d, State %s [%d]\n",
               i, player.health(), splat::EnumNameStateId(id), id);
        previous_states[i] = id;
      }
    }

    // Some random "interactivity"
    if (input.GetButton(SDLK_POINTER1).is_down()) {
      vertices[0] += input.pointers_[0].mousedelta.x() / 100.0f;
    }

    Material *idle_character =
            matman.FindMaterial("materials/character_idle.bin");
    idle_character->Set(renderer);
    Mesh::RenderArray(GL_TRIANGLE_STRIP, 4, format, sizeof(float) * 5,
                      reinterpret_cast<const char *>(vertices), indices);

    // Populate 'scene' from the game state--all the positions, orientations,
    // and renderable-ids (which specify materials) of the characters and props.
    // Also specify the camera matrix.
    game_state.PopulateScene(&scene);

    // Issue draw calls for the 'scene'.
    RenderSceneFromDescription(renderer, materials, scene);
  }
  return 0;
}

} // namespace fpl

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;
  return fpl::MainLoop();
}

