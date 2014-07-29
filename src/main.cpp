/*
* Copyright 2014 Google Inc. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "precompiled.h"

#include "character_state_machine.h"
#include "character_state_machine_def_generated.h"
#include "input.h"
#include "renderer.h"

using namespace fpl;

// TODO: move elsewhere
char *LoadFile(const char *filename, size_t *length_ptr = nullptr)
{
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) return nullptr;
  auto len = static_cast<size_t>(SDL_RWseek(handle, 0, RW_SEEK_END));
  SDL_RWseek(handle, 0, RW_SEEK_SET);
  auto buf = reinterpret_cast<char *>(malloc(len + 1));
  if (!buf) {
    SDL_RWclose(handle);
    return nullptr;
  }
  buf[len] = 0;
  size_t rlen = static_cast<size_t>(SDL_RWread(handle, buf, 1, len));
  SDL_RWclose(handle);
  if (len != rlen || len <= 0) { free(buf); return NULL; }
  if (length_ptr) *length_ptr = len;
  return buf;
}

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;
  printf("Splat initializing..\n");

  InputSystem input;

  Renderer renderer;
  renderer.Initialize(vec2i(1280, 800), "my amazing game!");

  // This code will be inside a future material manager instead.
  auto vs_source = LoadFile("shaders/textured.glslv");
  auto ps_source = LoadFile("shaders/textured.glslf");
  auto texture_tga = LoadFile("textures/mover_s.tga");
  if (!vs_source || !ps_source || !texture_tga) {
    printf("can't load assets\n");
    return 1;
  }

  auto shader = renderer.CompileAndLinkShader(vs_source, ps_source);
  if (!shader) {
    printf("shader error: %s\n", renderer.glsl_error_.c_str());
    return 1;
  }

  auto texture_id = renderer.CreateTextureFromTGAMemory(texture_tga);
  if (!texture_id) {
    printf("can't create texture from tga\n");
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

  auto state_machine_source = LoadFile("character_state_machine_def.bin");
  if (!state_machine_source) {
    printf("can't load character state machines\n");
    return 1;
  }

  auto state_machine_def =
      splat::GetCharacterStateMachineDef(state_machine_source);
  CharacterStateMachineDef_Validate(state_machine_def);

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

  // Temp resource cleanup:
  free(state_machine_source);
  free(vs_source);
  free(ps_source);
  free(texture_tga);

  return 0;
}

