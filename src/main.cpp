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
#include "renderer.h"
#include "material_manager.h"
#include "input.h"
#include "renderer.h"

using namespace fpl;

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

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;
  printf("Splat initializing..\n");
  if (!ChangeToAssetsDir()) return 1;

  InputSystem input;

  Renderer renderer;
  MaterialManager matman(renderer);

  renderer.Initialize(vec2i(1280, 800), "my amazing game!");

  auto mat = matman.LoadMaterial("materials/example.bin");
  if (!mat) {
    fprintf(stderr, "load error: %s\n", renderer.last_error_.c_str());
    return 1;
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
  if (!MaterialManager::LoadFile("character_state_machine_def.bin",
                                 &state_machine_source)) {
    fprintf(stderr, "can't load character state machines\n");
    return 1;
  }

  //auto state_machine_def =
  //    splat::GetCharacterStateMachineDef(state_machine_source.c_str());
  //CharacterStateMachineDef_Validate(state_machine_def);

  while (!input.exit_requested_ &&
         !input.GetButton(SDLK_ESCAPE).went_down()) {
    renderer.AdvanceFrame(input.minimized_);
    renderer.ClearFrameBuffer(vec4(0.0f));
    input.AdvanceFrame(&renderer.window_size_);

    // Some random "interactivity"
    if (input.GetButton(SDLK_POINTER1).is_down()) {
      vertices[0] += input.pointers_[0].mousedelta.x() / 100.0f;
    }

    mat->Set(renderer);
    Mesh::RenderArray(GL_TRIANGLES, 3, format, sizeof(float) * 5,
                         reinterpret_cast<const char *>(vertices), indices);
  }

  return 0;
}

