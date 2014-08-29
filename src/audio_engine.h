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

#ifndef SPLAT_AUDIO_ENGINE_H_
#define SPLAT_AUDIO_ENGINE_H_

#include "sound.h"
#include "splat_common_generated.h"

namespace fpl {

struct AudioConfig;

class AudioEngine {
 public:
  ~AudioEngine();

  bool Initialize(const AudioConfig* config);

  // Play a sound associated with the given sound_id.
  void PlaySound(int sound_id);

  // TODO: Update audio volume per channel each frame. b/17316699
  // void AdvanceFrame();

 private:
  // Hold the audio bus list.
  std::string buses_source_;

  // Hold the sounds.
  std::vector<Sound> sounds_;
};

}  // namespace fpl

#endif  // SPLAT_AUDIO_ENGINE_H_
