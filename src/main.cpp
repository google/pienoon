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
#include "character_state_machine_def_generated.h"
#include "game_state.h"
#include "input.h"
#include "material_manager.h"
#include "render_scene.h"
#include "renderer.h"
#include "sdl_controller.h"
#include "timeline_generated.h"

namespace fpl
{

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void PopulateSceneFromGameState(RenderScene* scene) {
  const Quat identityQuaternion(1.0f, 0.0f, 0.0f, 0.0f);
  scene->set_camera(mathfu::mat4::FromTranslationVector(
                      mathfu::vec3(0.0f, 5.0f, -10.0f)));
  scene->renderables().push_back(Renderable(RenderableId_CharacterIdle,
                                            mathfu::mat4::Identity()));
  scene->lights().push_back(mathfu::vec3(10.0f, 10.0f, 10.0f));
}

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
static bool ChangeToAssetsDir() {
#if !defined(__ANDROID__)
  {
    static const char kAssetsDir[] = "assets";
    static const char *kBuildPaths[] = {"Debug", "Release"};
    char path[256];
    char *dir = getcwd(path, sizeof(path));
    assert(dir);
    dir = strrchr(path, flatbuffers::kPathSeparator);
    dir = dir ? dir + 1 : path;
    if (strcmp(dir, kAssetsDir) != 0) {
      int success;
      for (size_t i = 0; i < sizeof(kBuildPaths) / sizeof(kBuildPaths[0]);
           ++i) {
        if (strcmp(dir, kBuildPaths[i]) == 0) {
          success = chdir("..");
          assert(success == 0);
          break;
        }
      }
      success = chdir(kAssetsDir);
      if (success != 0) {
        fprintf(stderr, "Unable to change into %s dir\n", kAssetsDir);
        return false;
      }
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

int MainLoop() {
  printf("Splat initializing..\n");
  if (!ChangeToAssetsDir()) return 1;

  InputSystem input;

  Renderer renderer;
  MaterialManager matman(renderer);

  renderer.Initialize(vec2i(1280, 800), "my amazing game!");

  std::vector<Material*> materials;
  for (int i = 0; i < RenderableId_Num; ++i) {
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

  renderer.camera.model_view_projection_ =
      mat4::Ortho(-2.0f, 2.0f, -2.0f, 2.0f, -1.0f, 10.0f);

  renderer.color = vec4(1, 1, 1, 1);

  Attribute format[] = { kPosition3f, kTexCoord2f, kEND };
  int indices[] = { 0, 1, 2 };
  float vertices[] = { 0, 0, 0,   0, 0,
                       1, 1, 0,   1, 1,
                       1, -1, 0,  1, 0 };

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

  // TODO: Move this to a data file at some point, or make it configurable
  // in-game
  const int DEFAULT_HEALTH = 10;
  const int PLAYER_COUNT = 4;

  splat::SdlController* controllers[PLAYER_COUNT];
  for (int i = 0; i < PLAYER_COUNT; i++) {
    controllers[i] = new splat::SdlController(
        &input, splat::ControlScheme::GetDefaultControlScheme(i));
  }
  for (int i = 0; i < PLAYER_COUNT; i++) {
    game_state.AddCharacter(DEFAULT_HEALTH, controllers[i], state_machine_def);
  }
  // TODO: Remove this block and the one in the main loop that prints the
  // current state.
  // This is just for development. It keeps track of when a state machine
  // transitions so that we can print the change. Printing every frame would be
  // spammy.
  std::vector<int> previous_states(PLAYER_COUNT, -1);

  RenderScene scene;
  while (!input.exit_requested_ &&
         !input.GetButton(SDLK_ESCAPE).went_down()) {
    renderer.AdvanceFrame(input.minimized_);
    renderer.ClearFrameBuffer(vec4(0.0f));
    input.AdvanceFrame(&renderer.window_size_);
    game_state.AdvanceFrame();

    // Display the state changes, at least until we get real rendering up.
    for (int i = 0; i < PLAYER_COUNT; i++) {
      auto& player = game_state.characters()->at(i);
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

    materials[0]->Set(renderer);
    Mesh::RenderArray(GL_TRIANGLES, 3, format, sizeof(float) * 5,
                      reinterpret_cast<const char *>(vertices), indices);

    // Populate 'scene' from the game state--all the positions, orientations,
    // and renderable-ids (which specify materials) of the characters and props.
    // Also specify the camera matrix.
    PopulateSceneFromGameState(&scene);

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

