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

#include "splat_game.h"

#include "SDL_mixer.h"

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;

  fpl::splat::SplatGame game;
  if (!game.Initialize())
    return 1;

  Mix_OpenAudio(48000, AUDIO_U8, 2, 2048);
  Mix_Chunk* sample = Mix_LoadWAV("./sounds/whoosh01.wav");
  if (!sample) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load wav file\n");
  }
  Mix_AllocateChannels(1);
  Mix_PlayChannel(0, sample, 0);

  game.Run();

  Mix_CloseAudio();

  return 0;
}
