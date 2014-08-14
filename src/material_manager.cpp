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

static_assert(kBlendModeOff == static_cast<BlendMode>(matdef::BlendMode_OFF) &&
              kBlendModeTest == static_cast<BlendMode>(matdef::BlendMode_TEST) &&
              kBlendModeAlpha == static_cast<BlendMode>(matdef::BlendMode_ALPHA),
              "BlendMode enums in renderer.h and material.fbs must match.");
static_assert(kBlendModeCount == kBlendModeAlpha + 1,
              "Please update static_assert above with new enum values.");

template<typename T> T FindInMap(const std::map<std::string, T> &map,
                                  const char *name) {
  auto it = map.find(name);
  return it != map.end() ? it->second : 0;
}

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

Shader *MaterialManager::FindShader(const char *basename) {
  return FindInMap(shader_map_, basename);
}

Shader *MaterialManager::LoadShader(const char *basename) {
  auto shader = FindShader(basename);
  if (shader) return shader;
  std::string vs_file, ps_file;
  std::string filename = std::string(basename) + ".glslv";
  if (LoadFile(filename.c_str(), &vs_file)) {
    filename = std::string(basename) + ".glslf";
    if (LoadFile(filename.c_str(), &ps_file)) {
      shader = renderer_.CompileAndLinkShader(vs_file.c_str(),
                                              ps_file.c_str());
      if (shader)
        shader_map_[basename] = shader;
      else
        fprintf(stderr, "Shader Error:\n%s\n", renderer_.last_error().c_str());
      return shader;
    }
  }
  renderer_.last_error() = "Couldn\'t load: " + filename;
  return nullptr;
}

Texture *MaterialManager::FindTexture(const char *filename) {
  return FindInMap(texture_map_, filename);
}

Texture *MaterialManager::LoadTexture(const char *filename) {
  auto tex = FindTexture(filename);
  if (tex) return tex;
  std::string tga;
  if (LoadFile(filename, &tga)) {
    tex = renderer_.CreateTextureFromTGAMemory(tga.c_str());
    if (tex) texture_map_[filename] = tex;
    return tex;
  }
  renderer_.last_error() = std::string("Couldn\'t load: ") + filename;
  return 0;
}

Material *MaterialManager::FindMaterial(const char *filename) {
  return FindInMap(material_map_, filename);
}

Material *MaterialManager::LoadMaterial(const char *filename) {
  auto mat = FindMaterial(filename);
  if (mat) return mat;
  std::string flatbuf;
  if (LoadFile(filename, &flatbuf)) {
    assert(matdef::VerifyMaterialBuffer(flatbuffers::Verifier(
      reinterpret_cast<const uint8_t *>(flatbuf.c_str()), flatbuf.length())));
    auto matdef = matdef::GetMaterial(flatbuf.c_str());
    auto shadername = matdef->shader_basename();
    auto shader = LoadShader(shadername->c_str());
    if (!shader) return nullptr;
    mat = new Material();
    mat->set_shader(shader);
    mat->set_blend_mode(static_cast<BlendMode>(matdef->blendmode()));
    for (auto it = matdef->texture_filenames()->begin();
             it != matdef->texture_filenames()->end(); ++it) {
      auto tex = LoadTexture(it->c_str());
      if (!tex) {
        delete mat;
        return nullptr;
      }
      mat->textures().push_back(tex);
    }
    material_map_[filename] = mat;
    return mat;
  }
  renderer_.last_error() = std::string("Couldn\'t load: ") + filename;
  return nullptr;
}

}  // namespace fpl

