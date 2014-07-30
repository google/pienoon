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

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;
  printf("Splat initializing..\n");

  InputSystem input;

  Renderer renderer;
  MaterialManager matman(renderer);

  renderer.Initialize(vec2i(1280, 800), "my amazing game!");

  auto shader = matman.LoadShader("shaders/textured");
  if (!shader) {
    printf("shader load error: %s\n", renderer.last_error_.c_str());
    return 1;
  }

  auto texture_id = matman.LoadTGATexture("textures/mover_s.tga");
  if (!texture_id) {
    printf("can't load texture from tga\n");
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
    printf("can't load character state machines\n");
    //return 1;
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

    shader->Set(renderer);
    glBindTexture(GL_TEXTURE_2D, texture_id);  // FIXME
    Mesh::RenderArray(GL_TRIANGLES, 3, format, sizeof(float) * 5,
                         reinterpret_cast<const char *>(vertices), indices);
  }

  return 0;
}

