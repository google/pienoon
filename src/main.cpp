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

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;

  fpl::splat::SplatGame game;
  if (!game.Initialize())
    return 1;

# ifdef __ANDROID__
  // Temp.. probably move this somewhere else once working
  gpg::AndroidPlatformConfiguration platform_configuration;
  platform_configuration.SetActivity((jobject)SDL_AndroidGetActivity());

  // Creates a games_services object that has lambda callbacks.
  bool is_auth_in_progress = false;
  auto game_services =
    gpg::GameServices::Builder()
      .SetDefaultOnLog(gpg::LogLevel::VERBOSE)
      .SetOnAuthActionStarted([&is_auth_in_progress](gpg::AuthOperation op) {
        is_auth_in_progress = true;
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "GPG: Sign in started!");
      })
      .SetOnAuthActionFinished([&is_auth_in_progress](gpg::AuthOperation op,
                                                      gpg::AuthStatus status) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "GPG: Sign in finished with a result of %d", status);
        is_auth_in_progress = false;
      })
      .Create(platform_configuration);

  if (!game_services) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "GPG: failed to create GameServices!");
  } else {
    // Submit a high score
    game_services->Leaderboards().SubmitScore("myid", 0);

    // Show the default Achievements UI
    game_services->Achievements().ShowAllUI();
  }
# endif

  Mix_OpenAudio(48000, AUDIO_U8, 2, 2048);
  Mix_Chunk* sample = Mix_LoadWAV("sounds/whoosh01.wav");
  if (!sample) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to load wav file\n");
  }
  Mix_AllocateChannels(1);
  Mix_PlayChannel(0, sample, 0);

  game.Run();

  Mix_CloseAudio();

  return 0;
}
