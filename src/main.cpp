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

#ifdef __ANDROID__
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "main: JNI_OnLoad called");

  gpg::AndroidInitialization::JNI_OnLoad(vm);

  return JNI_VERSION_1_4;
}
#endif

int main(int argc, char *argv[]) {
  (void) argc; (void) argv;

  fpl::splat::SplatGame game;
  if (!game.Initialize()) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Splat: init failed, exiting!");
    return 1;
  }

# ifdef __ANDROID__
  // Temp.. probably move this somewhere else once working

  /*
  auto env = reinterpret_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  JavaVM *vm = nullptr;
  auto ret = env->GetJavaVM(&vm);
  assert(ret >= 0);
  gpg::AndroidInitialization::JNI_OnLoad(vm);
  */
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
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                "GPG: failed to create GameServices!");
  } else {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: created GameServices");

    // Submit a high score
    game_services->Leaderboards().SubmitScore("myid", 0);

    // Show the default Achievements UI
    game_services->Achievements().ShowAllUI();
  }
# endif

  game.Run();

  return 0;
}
