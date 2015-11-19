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
#include "gpg_manager.h"

//#define NO_GPG

namespace fpl {

GPGManager::GPGManager()
    : state_(kStart), do_ui_login_(false), delayed_login_(false) {}

pthread_mutex_t GPGManager::events_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GPGManager::achievements_mutex_ = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t GPGManager::players_mutex_ = PTHREAD_MUTEX_INITIALIZER;

bool GPGManager::Initialize(bool ui_login) {
  state_ = kStart;
  do_ui_login_ = ui_login;
  event_data_initialized_ = false;
  achievement_data_initialized_ = false;
  player_data_.reset(nullptr);
#ifdef NO_GPG
  return true;
#endif

  /*
  // This code is here because we may be able to do this part of the
  // initialization here in the future, rather than relying on JNI_OnLoad below.
  auto env = reinterpret_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  JavaVM *vm = nullptr;
  auto ret = env->GetJavaVM(&vm);
  assert(ret >= 0);
  gpg::AndroidInitialization::JNI_OnLoad(vm);
  */
  gpg::AndroidPlatformConfiguration platform_configuration;
  platform_configuration.SetActivity((jobject)SDL_AndroidGetActivity());

  // Creates a games_services object that has lambda callbacks.
  game_services_ =
      gpg::GameServices::Builder()
          .SetDefaultOnLog(gpg::LogLevel::VERBOSE)
          .SetOnAuthActionStarted([this](gpg::AuthOperation op) {
            state_ =
                state_ == kAuthUILaunched ? kAuthUIStarted : kAutoAuthStarted;
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "GPG: Sign in started! (%d)", state_);
          })
          .SetOnAuthActionFinished([this](gpg::AuthOperation op,
                                          gpg::AuthStatus status) {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                        "GPG: Sign in finished with a result of %d (%d)",
                        status, state_);
            if (op == gpg::AuthOperation::SIGN_IN) {
              state_ =
                  status == gpg::AuthStatus::VALID
                      ? kAuthed
                      : ((state_ == kAuthUIStarted || state_ == kAuthUILaunched)
                             ? kAuthUIFailed
                             : kAutoAuthFailed);
              if (state_ == kAuthed) {
                // If we just logged in, go fetch our data!
                FetchPlayer();
                FetchEvents();
                FetchAchievements();
              }
            } else if (op == gpg::AuthOperation::SIGN_OUT) {
              state_ = kStart;
              SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                          "GPG: SIGN OUT finished with a result of %d", status);
            } else {
              SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                          "GPG: unknown auth op %d", op);
            }
          })
          .Create(platform_configuration);

  if (!game_services_) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "GPG: failed to create GameServices!");
    return false;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: created GameServices");
  return true;
}

// Called every frame from the game, to see if there's anything to be done
// with the async progress from gpg
void GPGManager::Update() {
#ifdef NO_GPG
  return;
#endif
  assert(game_services_);
  switch (state_) {
    case kStart:
    case kAutoAuthStarted:
      // Nothing to do, waiting.
      break;
    case kAutoAuthFailed:
    case kManualSignBackIn:
      // Need to explicitly ask for user  login.
      if (do_ui_login_) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: StartAuthorizationUI");
        game_services_->StartAuthorizationUI();
        state_ = kAuthUILaunched;
        do_ui_login_ = false;
      } else {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "GPG: skipping StartAuthorizationUI");
        state_ = kAuthUIFailed;
      }
      break;
    case kAuthUILaunched:
    case kAuthUIStarted:
      // Nothing to do, waiting.
      break;
    case kAuthUIFailed:
      // Both auto and UI based auth failed, I guess at this point we give up.
      if (delayed_login_) {
        // Unless the user expressed desire to try log in again while waiting
        // for this state.
        delayed_login_ = false;
        state_ = kManualSignBackIn;
        do_ui_login_ = true;
      }
      break;
    case kAuthed:
      // We're good. TODO: Now start actually using gpg functionality...
      break;
  }
}

bool GPGManager::LoggedIn() {
#ifdef NO_GPG
  return false;
#endif
  assert(game_services_);
  if (state_ < kAuthed) {
    SDL_LogDebug(SDL_LOG_CATEGORY_ERROR,
                 "GPG: player not logged in, can\'t interact with gpg!");
    return false;
  }
  return true;
}

void GPGManager::ToggleSignIn() {
#ifdef NO_GPG
  return;
#endif
  delayed_login_ = false;
  if (state_ == kAuthed) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: Attempting to log out...");
    game_services_->SignOut();
  } else if (state_ == kStart || state_ == kAuthUIFailed) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: Attempting to log in...");
    state_ = kManualSignBackIn;
    do_ui_login_ = true;
  } else {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "GPG: Ignoring log in/out in state %d", state_);
    delayed_login_ = true;
  }
}

void GPGManager::IncrementEvent(const char *event_id, uint64_t score) {
#ifdef NO_GPG
  return;
#endif
  if (!LoggedIn()) return;
  game_services_->Events().Increment(event_id, score);
}

// This is still somewhat game-specific.  (Because it assumes that your
// leaderboards are tied to events.)  TODO: clean this up further later.
void GPGManager::ShowLeaderboards(const GPGIds *ids, size_t id_len) {
#ifdef NO_GPG
  return;
#endif
  if (!LoggedIn()) return;
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: launching leaderboard UI");
  // First, get all current event counts from GPG in one callback,
  // which allows us to conveniently update and show the leaderboards without
  // having to deal with multiple callbacks.
  game_services_->Events().FetchAll([id_len, ids, this](
      const gpg::EventManager::FetchAllResponse &far) {
    for (auto it = far.data.begin(); it != far.data.end(); ++it) {
      // Look up leaderboard id from corresponding event id.
      const char *leaderboard_id = nullptr;
      for (size_t i = 0; i < id_len; i++) {
        if (ids[i].event == it->first) {
          leaderboard_id = ids[i].leaderboard.c_str();
        }
      }
      assert(leaderboard_id);
      if (leaderboard_id) {
        game_services_->Leaderboards().SubmitScore(leaderboard_id,
                                                   it->second.Count());
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "GPG: submitted score %llu for id %s", it->second.Count(),
                    leaderboard_id);
      }
    }
    game_services_->Leaderboards().ShowAllUI([](const gpg::UIStatus &status) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "GPG: Leaderboards UI FAILED, UIStatus is: %d", status);
    });
  });
}

// Unlocks a given achievement.
void GPGManager::UnlockAchievement(std::string achievement_id) {
  if (LoggedIn()) {
    game_services_->Achievements().Unlock(achievement_id);
  }
}

// Increments an incremental achievement.
void GPGManager::IncrementAchievement(std::string achievement_id) {
  if (LoggedIn()) {
    game_services_->Achievements().Increment(achievement_id);
  }
}

// Increments an incremental achievement by an amount.
void GPGManager::IncrementAchievement(std::string achievement_id,
                                      uint32_t steps) {
  if (LoggedIn()) {
    game_services_->Achievements().Increment(achievement_id, steps);
  }
}

// Reveals a given achievement.
void GPGManager::RevealAchievement(std::string achievement_id) {
  if (LoggedIn()) {
    game_services_->Achievements().Reveal(achievement_id);
  }
}

// Updates local player stats with values from the server:
void GPGManager::FetchEvents() {
  if (!LoggedIn() || event_data_state_ == kPending) return;
  event_data_state_ = kPending;

  game_services_->Events().FetchAll(
      [this](const gpg::EventManager::FetchAllResponse &far) mutable {

        pthread_mutex_lock(&events_mutex_);

        if (IsSuccess(far.status)) {
          event_data_state_ = kComplete;
          event_data_initialized_ = true;
        } else {
          event_data_state_ = kFailed;
        }

        event_data_ = far.data;
        pthread_mutex_unlock(&events_mutex_);
      });
}

bool GPGManager::IsAchievementUnlocked(std::string achievement_id) {
  if (!achievement_data_initialized_) {
    return false;
  }
  bool ret = false;
  pthread_mutex_lock(&achievements_mutex_);
  for (unsigned int i = 0; i < achievement_data_.size(); i++) {
    if (achievement_data_[i].Id() == achievement_id) {
      ret = achievement_data_[i].State() == gpg::AchievementState::UNLOCKED;
      break;
    }
  }
  pthread_mutex_unlock(&achievements_mutex_);
  return ret;
}

uint64_t GPGManager::GetEventValue(std::string event_id) {
  if (!event_data_initialized_) {
    return 0;
  }
  pthread_mutex_lock(&achievements_mutex_);
  uint64_t result = event_data_[event_id].Count();
  pthread_mutex_unlock(&achievements_mutex_);
  return result;
}

// Updates local player achievements with values from the server:
void GPGManager::FetchAchievements() {
  if (!LoggedIn() || achievement_data_state_ == kPending) return;
  achievement_data_state_ = kPending;

  game_services_->Achievements().FetchAll(
      [this](const gpg::AchievementManager::FetchAllResponse &far) mutable {

        pthread_mutex_lock(&achievements_mutex_);

        if (IsSuccess(far.status)) {
          achievement_data_state_ = kComplete;
          achievement_data_initialized_ = true;
        } else {
          achievement_data_state_ = kFailed;
        }

        achievement_data_ = far.data;
        pthread_mutex_unlock(&achievements_mutex_);
      });
}

void GPGManager::ShowAchievements() {
#ifdef NO_GPG
  return;
#endif
  if (!LoggedIn()) return;
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: launching achievement UI");
  game_services_->Achievements().ShowAllUI([](const gpg::UIStatus &status) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "GPG: Achievement UI FAILED, UIStatus is: %d", status);
  });
}

void GPGManager::FetchPlayer() {
  game_services_->Players().FetchSelf([this](
      const gpg::PlayerManager::FetchSelfResponse &fsr) mutable {

    pthread_mutex_lock(&players_mutex_);
    if (IsSuccess(fsr.status)) {
      gpg::Player *player_data = new gpg::Player(fsr.data);
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "GPG: got player info. ID = %s, name = %s, avatar=%s",
                  player_data->Id().c_str(), player_data->Name().c_str(),
                  player_data->AvatarUrl(gpg::ImageResolution::HI_RES).c_str());
      player_data_.reset(player_data);
    } else {
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                   "GPG: failed to get player info");
      player_data_.reset(nullptr);
    }
    pthread_mutex_unlock(&players_mutex_);
  });
}

}  // fpl

#ifdef __ANDROID__
extern "C" JNIEXPORT
jint GPG_JNI_OnLoad(JavaVM *vm, void *reserved) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG_JNI_OnLoad called");

  gpg::AndroidInitialization::JNI_OnLoad(vm);

  return JNI_VERSION_1_4;
}

extern "C" JNIEXPORT void JNICALL
Java_com_google_fpl_pie_1noon_FPLActivity_nativeOnActivityResult(
    JNIEnv *env, jobject thiz, jobject activity, jint request_code,
    jint result_code, jobject data) {
  gpg::AndroidSupport::OnActivityResult(env, activity, request_code,
                                        result_code, data);
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GPG: nativeOnActivityResult");
}
#endif
