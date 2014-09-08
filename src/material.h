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

#ifndef FPL_MATERIAL_H
#define FPL_MATERIAL_H

#include "shader.h"

namespace fpl {

class Renderer;

enum BlendMode {
  kBlendModeOff,
  kBlendModeTest,
  kBlendModeAlpha,

  kBlendModeCount // Must be at end.
};

struct Texture {
  Texture(GLuint _id, const vec2i &_size) : id(_id), size(_size) {}

  GLuint id;
  vec2i size;
};

class Material {
 public:
  Material() : blend_mode_(kBlendModeOff) {}

  void Set(Renderer &renderer);

  std::vector<Texture *> &textures() { return textures_; }
  const std::vector<Texture *> &textures() const { return textures_; }
  int blend_mode() const { return blend_mode_; }
  void set_blend_mode(BlendMode blend_mode) {
    assert(0 <= blend_mode && blend_mode < kBlendModeCount);
    blend_mode_ = blend_mode;
  }

 private:
  std::vector<Texture *> textures_;
  BlendMode blend_mode_;
};

}  // namespace fpl

#endif  // FPL_MATERIAL_H
