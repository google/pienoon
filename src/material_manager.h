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

#ifndef MATERIAL_MANAGER_H
#define MATERIAL_MANAGER_H

#include "renderer.h"

namespace fpl {

class Material {
 public:
  Material() : shader_(nullptr) {}

  void Set(const Renderer &renderer);

  Shader *shader_;
  std::vector<GLuint> textures_;
};

class MaterialManager {
 public:
  MaterialManager(Renderer &renderer) : renderer_(renderer) {}

  // Returns a previously loaded shader object, or NULL.
  Shader *FindShader(const char *basename);
  // Loads a shader if it hasn't been loaded already, by appending .glslv
  // and .glslf to the basename, compiling and linking them.
  // If this returns NULL, the error can be found in Renderer::last_error_.
  Shader *LoadShader(const char *basename);

  // Returns a previously loaded texture id, or 0.
  GLuint FindTexture(const char *filename);
  // Loads a texture if it hasn't been loaded already. Currently only supports
  // TGA format files. 0 if the file couldn't be read.
  GLuint LoadTexture(const char *filename);

  // Returns a previously loaded material, or NULL.
  Material *FindMaterial(const char *filename);
  // Loads a material, which is a compiled FlatBuffer file with
  // root Material. This loads all resources contained there-in.
  // If this returns NULL, the error can be found in Renderer::last_error_.
  Material *LoadMaterial(const char *filename);

  // Utility function for any file loading.
  static bool LoadFile(const char *filename, std::string *dest);

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialManager);

  Renderer &renderer_;
  std::map<std::string, Shader *> shader_map_;
  std::map<std::string, GLuint> texture_map_;
  std::map<std::string, Material *> material_map_;
};

}  // namespace fpl

#endif // MATERIAL_MANAGER_H
