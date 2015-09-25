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
#include "mesh_generated.h"
#include "utilities.h"

namespace fpl {

static_assert(kBlendModeOff == static_cast<BlendMode>(matdef::BlendMode_OFF) &&
                  kBlendModeTest ==
                      static_cast<BlendMode>(matdef::BlendMode_TEST) &&
                  kBlendModeAlpha ==
                      static_cast<BlendMode>(matdef::BlendMode_ALPHA),
              "BlendMode enums in renderer.h and material.fbs must match.");
static_assert(kBlendModeCount == kBlendModeAlpha + 1,
              "Please update static_assert above with new enum values.");

template <typename T>
T FindInMap(const std::map<std::string, T> &map, const char *name) {
  auto it = map.find(name);
  return it != map.end() ? it->second : 0;
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
      shader = renderer_.CompileAndLinkShader(vs_file.c_str(), ps_file.c_str());
      if (shader) {
        shader_map_[basename] = shader;
      } else {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Shader Error:\n%s\n",
                     renderer_.last_error().c_str());
      }
      return shader;
    }
  }
  SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can\'t load shader: %s",
               filename.c_str());
  renderer_.last_error() = "Couldn\'t load: " + filename;
  return nullptr;
}

Texture *MaterialManager::FindTexture(const char *filename) {
  return FindInMap(texture_map_, filename);
}

Texture *MaterialManager::LoadTexture(const char *filename,
                                      TextureFormat format) {
  auto tex = FindTexture(filename);
  if (tex) return tex;
  tex = new Texture(renderer_, filename);
  tex->set_desired_format(format);
  loader_.QueueJob(tex);
  texture_map_[filename] = tex;
  return tex;
}

void MaterialManager::StartLoadingTextures() { loader_.StartLoading(); }

bool MaterialManager::TryFinalize() { return loader_.TryFinalize(); }

Material *MaterialManager::FindMaterial(const char *filename) {
  return FindInMap(material_map_, filename);
}

Material *MaterialManager::LoadMaterial(const char *filename) {
  auto mat = FindMaterial(filename);
  if (mat) return mat;
  std::string flatbuf;
  if (LoadFile(filename, &flatbuf)) {
    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t *>(flatbuf.c_str()), flatbuf.length());
    assert(matdef::VerifyMaterialBuffer(verifier));
    auto matdef = matdef::GetMaterial(flatbuf.c_str());
    mat = new Material();
    mat->set_blend_mode(static_cast<BlendMode>(matdef->blendmode()));
    for (flatbuffers::uoffset_t i = 0; i < matdef->texture_filenames()->size();
         i++) {
      auto format =
          matdef->desired_format() && i < matdef->desired_format()->size()
              ? static_cast<TextureFormat>(matdef->desired_format()->Get(i))
              : kFormatAuto;
      auto tex =
          LoadTexture(matdef->texture_filenames()->Get(i)->c_str(), format);
      mat->textures().push_back(tex);
    }
    material_map_[filename] = mat;
    return mat;
  }
  renderer_.last_error() = std::string("Couldn\'t load: ") + filename;
  return nullptr;
}

void MaterialManager::UnloadMaterial(const char *filename) {
  auto mat = FindMaterial(filename);
  if (!mat) return;
  mat->DeleteTextures();
  material_map_.erase(filename);
  for (auto it = mat->textures().begin(); it != mat->textures().end(); ++it) {
    texture_map_.erase((*it)->filename());
  }
  delete mat;
}

Mesh *MaterialManager::FindMesh(const char *filename) {
  return FindInMap(mesh_map_, filename);
}

template <typename T>
void CopyAttribute(const T *attr, uint8_t *&buf) {
  auto dest = (T *)buf;
  *dest = *attr;
  buf += sizeof(T);
}

Mesh *MaterialManager::LoadMesh(const char *filename) {
  auto mesh = FindMesh(filename);
  if (mesh) return mesh;
  std::string flatbuf;
  if (LoadFile(filename, &flatbuf)) {
    flatbuffers::Verifier verifier(
        reinterpret_cast<const uint8_t *>(flatbuf.c_str()), flatbuf.length());
    assert(matdef::VerifyMaterialBuffer(verifier));
    auto meshdef = meshdef::GetMesh(flatbuf.c_str());
    // Collect what attributes are available.
    std::vector<Attribute> attrs;
    attrs.push_back(kPosition3f);
    if (meshdef->normals()) attrs.push_back(kNormal3f);
    if (meshdef->tangents()) attrs.push_back(kTangent4f);
    if (meshdef->colors()) attrs.push_back(kColor4ub);
    if (meshdef->texcoords()) attrs.push_back(kTexCoord2f);
    attrs.push_back(kEND);
    auto vert_size = Mesh::VertexSize(attrs.data());
    // Create an interleaved buffer. Would be cool to do this without
    // the additional copy, but that's not easy in OpenGL.
    // Could use multiple buffers instead, but likely less efficient.
    auto buf = new uint8_t[vert_size * meshdef->positions()->Length()];
    auto p = buf;
    for (flatbuffers::uoffset_t i = 0; i < meshdef->positions()->Length();
         i++) {
      if (meshdef->positions()) CopyAttribute(meshdef->positions()->Get(i), p);
      if (meshdef->normals()) CopyAttribute(meshdef->normals()->Get(i), p);
      if (meshdef->tangents()) CopyAttribute(meshdef->tangents()->Get(i), p);
      if (meshdef->colors()) CopyAttribute(meshdef->colors()->Get(i), p);
      if (meshdef->texcoords()) CopyAttribute(meshdef->texcoords()->Get(i), p);
    }
    mesh = new Mesh(buf, meshdef->positions()->Length(), (int)vert_size,
                    attrs.data());
    delete[] buf;
    // Load indices an materials.
    for (flatbuffers::uoffset_t i = 0; i < meshdef->surfaces()->size(); i++) {
      auto surface = meshdef->surfaces()->Get(i);
      auto mat = LoadMaterial(surface->material()->c_str());
      if (!mat) {
        delete mesh;
        return nullptr;
      }  // Error msg already set.
      mesh->AddIndices(
          reinterpret_cast<const uint16_t *>(surface->indices()->Data()),
          surface->indices()->Length(), mat);
    }
    mesh_map_[filename] = mesh;
    return mesh;
  }
  renderer_.last_error() = std::string("Couldn\'t load: ") + filename;
  return nullptr;
}

void MaterialManager::UnloadMesh(const char *filename) {
  auto mesh = FindMesh(filename);
  if (!mesh) return;
  mesh_map_.erase(filename);
  delete mesh;
}

}  // namespace fpl
