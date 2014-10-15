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
#include "sound.h"
#include "SDL_mixer.h"
#include "audio_engine.h"

namespace fpl {

const int kPlayStreamError = -1;
const int kLoopForever = -1;
const int kPlayOnce = 0;

static void LogAudioLoadingError(const char* filename) {
  SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't load %s\n", filename);
}

SoundBuffer::~SoundBuffer() {
  if (data_) {
    Mix_FreeChunk(data_);
  }
}

bool SoundBuffer::LoadFile(const char* filename) {
  data_ = Mix_LoadWAV(filename);
  if (!data_) {
    LogAudioLoadingError(filename);
    return false;
  }
  return true;
}

bool SoundBuffer::Play(ChannelId channel_id, bool loop) {
  int loops = loop ? kLoopForever : kPlayOnce;
  if (Mix_PlayChannel(channel_id, data_, loops) ==
      AudioEngine::kInvalidChannel) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play sound: %s\n", Mix_GetError());
    return false;
  }
  return true;
}


void SoundBuffer::SetGain(ChannelId channel_id, float gain) {
  Mix_Volume(channel_id,
             static_cast<int>(gain * static_cast<float>(MIX_MAX_VOLUME)));
}

SoundStream::~SoundStream() {
  if (data_) {
    Mix_FreeMusic(data_);
  }
}

bool SoundStream::LoadFile(const char* filename) {
  data_ = Mix_LoadMUS(filename);
  if (!data_) {
    LogAudioLoadingError(filename);
    return false;
  }
  return true;
}

bool SoundStream::Play(ChannelId channel_id, bool loop) {
  (void)channel_id;  // SDL_mixer does not currently support
                     // more than one channel of streaming audio.
  int loops = loop ? kLoopForever : kPlayOnce;
  if (Mix_PlayMusic(data_, loops) == kPlayStreamError) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play music: %s\n", Mix_GetError());
    return false;
  }
  return true;
}

void SoundStream::SetGain(ChannelId channel_id, float gain) {
  (void)channel_id;
  Mix_VolumeMusic(static_cast<int>(gain * static_cast<float>(MIX_MAX_VOLUME)));
}

}  // namespace fpl
