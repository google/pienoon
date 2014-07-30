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

struct Material {
  Shader *shader;
  std::vector<GLuint> textures;
};

class MaterialManager {
 public:
  MaterialManager(Renderer &renderer) : renderer_(renderer) {}

  Shader *LoadShader(const char *basename);
  GLuint LoadTGATexture(const char *filename);

  Material *LoadMaterial(const char *filename);

  static bool LoadFile(const char *filename, std::string *dest);

 private:
  Renderer &renderer_;
};

}  // namespace fpl

#endif // MATERIAL_MANAGER_H
