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
#include "material.h"
#include "renderer.h"

namespace fpl {

void Texture::Load() {
  data_ = renderer_.LoadAndUnpackTexture(filename_.c_str(), &size_,
                                         &has_alpha_);
  if (!data_) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "texture load: %s: %s",
                filename_.c_str(), renderer_.last_error().c_str());
  }
}

void Texture::Finalize() {
  if (data_) {
    id_ = renderer_.CreateTexture(data_, size_, has_alpha_, desired_);
    free(data_);
    data_ = nullptr;
  }
}

void Material::Set(Renderer &renderer) {
  renderer.SetBlendMode(blend_mode_);
  for (size_t i = 0; i < textures_.size(); i++) {
    GL_CALL(glActiveTexture(GL_TEXTURE0 + i));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textures_[i]->id()));
  }
}

}  // namespace fpl
