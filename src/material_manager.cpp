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
#include "material_manager.h"
#include "materials_generated.h"

namespace fpl {

bool MaterialManager::LoadFile(const char *filename, std::string *dest) {
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) return false;
  auto len = static_cast<size_t>(SDL_RWseek(handle, 0, RW_SEEK_END));
  SDL_RWseek(handle, 0, RW_SEEK_SET);
  dest->assign(len + 1, 0);
  size_t rlen = static_cast<size_t>(SDL_RWread(handle, &(*dest)[0], 1, len));
  SDL_RWclose(handle);
  return len == rlen && len > 0;
}

Shader *MaterialManager::LoadShader(const char *basename) {
  std::string vs_file, ps_file;
  std::string filename = std::string(basename) + ".glslv";
  if (LoadFile(filename.c_str(), &vs_file)) {
    filename = std::string(basename) + ".glslf";
    if (LoadFile(filename.c_str(), &ps_file)) {
      return renderer_.CompileAndLinkShader(vs_file.c_str(), ps_file.c_str());
    }
  }
  renderer_.last_error_ = "Couldn\'t load: " + filename;
  return nullptr;
}

GLuint MaterialManager::LoadTGATexture(const char *filename) {
  std::string tga;
  if (LoadFile(filename, &tga)) {
    return renderer_.CreateTextureFromTGAMemory(tga.c_str());
  }
  return 0;
}

Material *MaterialManager::LoadMaterial(const char *filename) {
  std::string flatbuf;
  if (LoadFile(filename, &flatbuf)) {
    auto matdef = matdef::GetMaterial(flatbuf.c_str());
    auto shader = LoadShader(matdef->shader_basename()->c_str());
    // TODO: load textures, and register all in a map.
    auto mat = new Material();
    mat->shader = shader;
    return mat;
  }
  return nullptr;
}



}  // namespace fpl

