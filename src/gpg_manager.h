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

#ifndef GPG_MANAGER_H
#define GPG_MANAGER_H

#include "common.h"
#include "pthread.h"
#include "gpg/achievement_manager.h"
#include "gpg/player_manager.h"
#include "gpg/types.h"

namespace fpl {

enum RequestState { kPending, kComplete, kFailed };

struct GPGKeyValuePair {
  std::string id;
  uint64_t value;
};

class GPGManager {
 public:
  GPGManager();

  // Start of initial initialization and auth.
  bool Initialize(bool ui_login);

  // Call once a frame to allow us to track our async work.
  void Update();

  // To be called from UI to sign out (if we were signed in) or sign back in
  // (if we were signed out).
  void ToggleSignIn();

  // Logged in status, can be shown in UI.
  bool LoggedIn();

  struct GPGIds {
    std::string leaderboard;
    std::string event;
  };

  // Request this stat to be saved for the logged in
  // player. Does nothing if not logged in.
  void IncrementEvent(const char *event_id, uint64_t score);

  void ShowLeaderboards(const GPGIds *ids, size_t id_len);
  void ShowAchievements();

  // Asynchronously fetches the stats associated with the current player
  // from the server.  (Does nothing if not logged in.)
  // The status of the data can be checked via event_data_state.
  // const char* fields[]
  void FetchEvents();
  void FetchAchievements();

  // Asynchronously fetches the current player's info from the server.
  // (Does nothing if not logged in.)
  void FetchPlayer();

  RequestState event_data_state() const { return event_data_state_; }

  std::map<std::string, gpg::Event> &event_data() { return event_data_; }

  gpg::Player *player_data() const { return player_data_.get(); }

  uint64_t GetEventValue(std::string event_id);
  bool IsAchievementUnlocked(std::string achievement_id);
  void UnlockAchievement(std::string achievement_id);
  void IncrementAchievement(std::string achievement_id);
  void IncrementAchievement(std::string achievement_id, uint32_t steps);
  void RevealAchievement(std::string achievement_id);

 private:
  // These are the states the manager can be in, in sequential order they
  // are expected to happen.
  enum AsyncState {
    kStart,
    kAutoAuthStarted,
    kAutoAuthFailed,
    kManualSignBackIn,
    kAuthUILaunched,
    kAuthUIStarted,
    kAuthUIFailed,
    kAuthed,
  };

  AsyncState state_;
  bool do_ui_login_;
  bool delayed_login_;
  std::unique_ptr<gpg::GameServices> game_services_;

  void UpdatePlayerStats();

  // The stats the stats currently stored on the server.
  // Retrieved after authentication.
  bool event_data_initialized_;
  bool achievement_data_initialized_;
  RequestState event_data_state_;
  RequestState achievement_data_state_;
  static pthread_mutex_t events_mutex_;
  static pthread_mutex_t achievements_mutex_;
  static pthread_mutex_t players_mutex_;
  std::map<std::string, gpg::Event> event_data_;
  std::unique_ptr<gpg::Player> player_data_;
  std::vector<gpg::Achievement> achievement_data_;
};

}  // fpl

#endif  // GPG_MANAGER_H
