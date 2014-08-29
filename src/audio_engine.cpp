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

#include "audio_config_generated.h"
#include "audio_engine.h"
#include "sound.h"
#include "sound_assets_generated.h"
#include "splat_common_generated.h"
#include "utilities.h"

namespace fpl {

AudioEngine::~AudioEngine() {
  for (unsigned int i = 0; i < sounds_.size(); ++i) {
    sounds_[i].Unload();
  }
  Mix_CloseAudio();
}

bool AudioEngine::Initialize(const AudioConfig* config) {
  // Initialize audio engine.
  if (Mix_OpenAudio(config->output_frequency(),
                    AUDIO_S16LSB,
                    config->output_channels(),
                    config->output_buffer_size()) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't open audio stream\n");
    return false;
  }

  // Number of sound that can be played simutaniously.
  Mix_AllocateChannels(config->mixer_channels());

  // Load the audio buses.
  if (!LoadFile("buses.bin", &buses_source_)) {
    return false;
  }

  // Load the list of assets.
  std::string sound_assets_source;
  if (!LoadFile("sound_assets.bin", &sound_assets_source)) {
    return false;
  }

  // Ensure that there is a 1:1 mapping of sound ids to sound assets.
  const SoundAssets* sound_assets = GetSoundAssets(sound_assets_source.c_str());
  if (sound_assets->sounds()->Length() != splat::SoundId_Count) {
    SDL_LogError(
        SDL_LOG_CATEGORY_ERROR,
        "Incorrect number of sound assets loaded. Expected %d, got %d\n",
        splat::SoundId_Count, sound_assets->sounds()->Length());
    return false;
  }

  // Create a Sound for each sound def
  unsigned int sound_count = sound_assets->sounds()->Length();
  sounds_.resize(sound_count);
  for (unsigned int i = 0; i < sound_count; ++i) {
    if (!sounds_[i].LoadSound(sound_assets->sounds()->Get(i)->c_str())) {
      return false;
    }
  }

  return true;
}

void AudioEngine::PlaySound(int sound_id) {
  const int kPlayChannelError = -1;
  Sound& sound = sounds_[sound_id];
  if (Mix_PlayChannel(-1, sound.SelectChunk(), 0) == kPlayChannelError) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play audio sample: %s\n", Mix_GetError());
  }
}

}  // namespace fpl

