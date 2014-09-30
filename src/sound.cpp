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

const ChannelId kInvalidChannel = -1;
const int kPlayMusicError = -1;
const int kLoopForever = -1;
const int kPlayOnce = 0;

static void LogAudioLoadingError(const char* filename) {
  SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't load %s\n", filename);
}

Sound::~Sound() {
  if (data_) {
    Mix_FreeChunk(data_);
  }
}

bool Sound::LoadFile(const char* filename) {
  data_ = Mix_LoadWAV(filename);
  if (!data_) {
    LogAudioLoadingError(filename);
    return false;
  }
  return true;
}

bool Sound::Play(ChannelId channel_id, bool loop) {
  int loops = loop ? kLoopForever : kPlayOnce;
  if (Mix_PlayChannel(channel_id, data_, loops) == kInvalidChannel) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play sound: %s\n", Mix_GetError());
    return false;
  }
  return true;
}

Music::~Music() {
  if (data_) {
    Mix_FreeMusic(data_);
  }
}

bool Music::LoadFile(const char* filename) {
  data_ = Mix_LoadMUS(filename);
  if (!data_) {
    LogAudioLoadingError(filename);
    return false;
  }
  return true;
}

bool Music::Play(ChannelId channel_id, bool loop) {
  (void)channel_id;  // SDL_mixer does not currently support
                     // more than one channel of streaming audio.
  int loops = loop ? kLoopForever : kPlayOnce;
  if (Mix_PlayMusic(data_, loops) == kPlayMusicError) {
    return false;
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play music: %s\n", Mix_GetError());
  }
  return true;
}

}  // namespace fpl

