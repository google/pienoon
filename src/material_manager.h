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
#include "common.h"
#include "async_loader.h"

namespace fpl {

class MaterialManager {
 public:
  MaterialManager(Renderer &renderer) : renderer_(renderer) {}

  // Returns a previously loaded shader object, or nullptr.
  Shader *FindShader(const char *basename);
  // Loads a shader if it hasn't been loaded already, by appending .glslv
  // and .glslf to the basename, compiling and linking them.
  // If this returns nullptr, the error can be found in Renderer::last_error().
  Shader *LoadShader(const char *basename);

  // Returns a previously created texture, or nullptr.
  Texture *FindTexture(const char *filename);
  // Queue's a texture for loading if it hasn't been loaded already.
  // Currently only supports TGA/WebP format files.
  // Returned texture isn't usable until TryFinalize() succeeds and the id
  // is non-zero.
  Texture *LoadTexture(const char *filename,
                       TextureFormat format = kFormatAuto);
  // LoadTextures doesn't actually load anything, this will start the async
  // loading of all files, and decompression.
  void StartLoadingTextures();
  // Call this repeatedly until it returns true, which signals all textures
  // will have loaded, and turned into OpenGL textures.
  // Textures with a 0 id will have failed to load.
  bool TryFinalize();

  // Returns a previously loaded material, or nullptr.
  Material *FindMaterial(const char *filename);
  // Loads a material, which is a compiled FlatBuffer file with
  // root Material. This loads all resources contained there-in.
  // If this returns nullptr, the error can be found in Renderer::last_error().
  Material *LoadMaterial(const char *filename);

  // Deletes all OpenGL textures contained in this material, and removes the
  // textures and the material from material manager. Any subsequent requests
  // for these textures through Load*() will cause them to be loaded anew.
  void UnloadMaterial(const char *filename);

  // Handy accessors, so you don't have to pass the renderer around too.
  Renderer &renderer() { return renderer_; }
  const Renderer &renderer() const { return renderer_; }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialManager);

  Renderer &renderer_;
  std::map<std::string, Shader *> shader_map_;
  std::map<std::string, Texture *> texture_map_;
  std::map<std::string, Material *> material_map_;
  AsyncLoader loader_;
};

}  // namespace fpl

#endif  // MATERIAL_MANAGER_H
