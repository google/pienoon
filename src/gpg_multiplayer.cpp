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
#include "gpg_multiplayer.h"
#include <algorithm>

namespace fpl {

GPGMultiplayer::GPGMultiplayer()
    : message_mutex_(PTHREAD_MUTEX_INITIALIZER),
      instance_mutex_(PTHREAD_MUTEX_INITIALIZER),
      state_mutex_(PTHREAD_MUTEX_INITIALIZER) {}

bool GPGMultiplayer::Initialize(const std::string& service_id) {
  state_ = kIdle;
  is_hosting_ = false;

  service_id_ = service_id;
  gpg::AndroidPlatformConfiguration platform_configuration;
  platform_configuration.SetActivity((jobject)SDL_AndroidGetActivity());
  if (!platform_configuration.Valid())
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Invalid PlatformConfiguration");

  gpg::NearbyConnections::Builder nearby_builder;
  nearby_connections_ = nearby_builder.SetDefaultOnLog(gpg::LogLevel::VERBOSE)
                            .SetServiceId(service_id_)
                            .Create(platform_configuration);
  discovery_listener_.reset(nullptr);
  message_listener_.reset(nullptr);

  if (nearby_connections_ == nullptr)
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Got null NearbyConnections");

  return (nearby_connections_ != nullptr);
}

void GPGMultiplayer::AddAppIdentifier(const std::string& identifier) {
  gpg::AppIdentifier id;
  id.identifier = identifier;
  app_identifiers_.push_back(id);
}

void GPGMultiplayer::StartAdvertising() { QueueNextState(kAdvertising); }

void GPGMultiplayer::StopAdvertising() {
  pthread_mutex_lock(&instance_mutex_);
  int num_connected_instances = connected_instances_.size();
  pthread_mutex_unlock(&instance_mutex_);
  if (num_connected_instances > 0) {
    QueueNextState(kConnected);
  } else {
    QueueNextState(kIdle);
  }
}

void GPGMultiplayer::StartDiscovery() { QueueNextState(kDiscovering); }

void GPGMultiplayer::StopDiscovery() {
  // check if we are connected
  pthread_mutex_lock(&instance_mutex_);
  int num_connected_instances = connected_instances_.size();
  pthread_mutex_unlock(&instance_mutex_);
  if (num_connected_instances > 0) {
    QueueNextState(kConnected);
  } else {
    QueueNextState(kIdle);
  }
}

void GPGMultiplayer::ResetToIdle() {
  QueueNextState(kIdle);
  DisconnectAll();

  pthread_mutex_lock(&instance_mutex_);
  connected_instances_.clear();
  connected_instances_reverse_.clear();
  instance_names_.clear();
  pending_instances_.clear();
  discovered_instances_.clear();
  pthread_mutex_unlock(&instance_mutex_);

  pthread_mutex_lock(&message_mutex_);
  while (!incoming_messages_.empty()) incoming_messages_.pop();
  pthread_mutex_unlock(&message_mutex_);
}

void GPGMultiplayer::DisconnectInstance(const std::string& instance_id) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Disconnect player (instance_id='%s')",
              instance_id.c_str());
  nearby_connections_->Disconnect(instance_id);

  pthread_mutex_lock(&instance_mutex_);
  auto i = std::find(connected_instances_.begin(), connected_instances_.end(),
                     instance_id);
  if (i != connected_instances_.end()) {
    connected_instances_.erase(i);
    UpdateConnectedInstances();
  }
  if (IsConnected() && connected_instances_.size() == 0) {
    pthread_mutex_unlock(&instance_mutex_);
    QueueNextState(kIdle);
  } else {
    pthread_mutex_unlock(&instance_mutex_);
  }
}

void GPGMultiplayer::DisconnectAll() {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Disconnect all players");
  // In case there are any connection requests outstanding, reject them.
  RejectAllConnectionRequests();

  // Disconnect anyone we are connected to.
  pthread_mutex_lock(&instance_mutex_);
  for (const auto& instance : connected_instances_) {
    nearby_connections_->Disconnect(instance);
  }
  connected_instances_.clear();
  UpdateConnectedInstances();
  pthread_mutex_unlock(&instance_mutex_);

  if (state() == kConnected) {
    QueueNextState(kIdle);
  }
}

void GPGMultiplayer::SendConnectionRequest(
    const std::string& host_instance_id) {
  if (message_listener_ == nullptr) {
    message_listener_.reset(new MessageListener(
        [this](const std::string& instance_id,
               std::vector<uint8_t> const& payload, bool is_reliable) {
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Multiplayer OnMessageReceived(%s) callback",
                      instance_id.c_str());
          this->MessageReceivedCallback(instance_id, payload, is_reliable);
        },
        [this](const std::string& instance_id) {
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Multiplayer OnDisconnect(%s) callback",
                      instance_id.c_str());
          this->DisconnectedCallback(instance_id);
        }));
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Sending connection request to %s",
              host_instance_id.c_str());

  // Immediately stop discovery once we start connecting.
  nearby_connections_->SendConnectionRequest(
      my_instance_name_, host_instance_id, std::vector<uint8_t>{},
      [this](int64_t client_id, gpg::ConnectionResponse const& response) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: OnConnectionResponse() callback");
        this->ConnectionResponseCallback(response);
      },
      message_listener_.get());
}

void GPGMultiplayer::AcceptConnectionRequest(
    const std::string& client_instance_id) {
  if (message_listener_ == nullptr) {
    message_listener_.reset(new MessageListener(
        [this](const std::string& instance_id,
               std::vector<uint8_t> const& payload, bool is_reliable) {
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Multiplayer: OnMessageReceived(%s) callback",
                      instance_id.c_str());
          this->MessageReceivedCallback(instance_id, payload, is_reliable);
        },
        [this](const std::string& instance_id) {
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                      "Multiplayer: OnDisconnect(%s) callback",
                      instance_id.c_str());
          this->DisconnectedCallback(instance_id);
        }));
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Accepting connection from %s",
              client_instance_id.c_str());
  nearby_connections_->AcceptConnectionRequest(
      client_instance_id, std::vector<uint8_t>{}, message_listener_.get());

  pthread_mutex_lock(&instance_mutex_);
  connected_instances_.push_back(client_instance_id);
  UpdateConnectedInstances();
  auto i = std::find(pending_instances_.begin(), pending_instances_.end(),
                     client_instance_id);
  if (i != pending_instances_.end()) {
    pending_instances_.erase(i);
  }
  pthread_mutex_unlock(&instance_mutex_);
}

void GPGMultiplayer::RejectConnectionRequest(
    const std::string& client_instance_id) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Rejecting connection from %s",
              client_instance_id.c_str());
  nearby_connections_->RejectConnectionRequest(client_instance_id);

  pthread_mutex_lock(&instance_mutex_);
  auto i = std::find(pending_instances_.begin(), pending_instances_.end(),
                     client_instance_id);
  if (i != pending_instances_.end()) {
    pending_instances_.erase(i);
  }
  pthread_mutex_unlock(&instance_mutex_);
}

void GPGMultiplayer::RejectAllConnectionRequests() {
  pthread_mutex_lock(&instance_mutex_);
  for (const auto& instance_id : pending_instances_) {
    nearby_connections_->RejectConnectionRequest(instance_id);
  }
  pending_instances_.clear();
  pthread_mutex_unlock(&instance_mutex_);
}

// Call me once a frame!
void GPGMultiplayer::Update() {
  pthread_mutex_lock(&state_mutex_);  // unlocked in two places below
  if (!next_states_.empty()) {
    // Transition at most one state per frame.
    MultiplayerState next_state = next_states_.front();
    next_states_.pop();
    pthread_mutex_unlock(&state_mutex_);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Multiplayer: Existing state %d to enter state %d", state(),
                next_state);
    TransitionState(state(), next_state);
  } else {
    pthread_mutex_unlock(&state_mutex_);
  }

  // Now update based on what state we are in.
  switch (state()) {
    case kDiscovering: {
      pthread_mutex_lock(&instance_mutex_);
      bool has_discovered_instance = !discovered_instances_.empty();
      pthread_mutex_unlock(&instance_mutex_);

      if (has_discovered_instance) {
        QueueNextState(kDiscoveringPromptedUser);
      }
      break;
    }
    case kDiscoveringPromptedUser: {
      DialogResponse response = GetConnectionDialogResponse();
      if (response == kDialogNo) {
        // we decided not to connect to the first discovered instance.
        pthread_mutex_lock(&instance_mutex_);
        discovered_instances_.pop_front();
        pthread_mutex_unlock(&instance_mutex_);

        QueueNextState(kDiscovering);
      } else if (response == kDialogYes) {
        // We decided to try to connect to the first discovered instance.
        pthread_mutex_lock(&instance_mutex_);
        auto instance = discovered_instances_.front();
        discovered_instances_.pop_front();
        pthread_mutex_unlock(&instance_mutex_);

        SendConnectionRequest(instance);
        QueueNextState(kDiscoveringWaitingForHost);
      }
      // Otherwise we haven't gotten a response yet.
    }
    case kAdvertising: {
      pthread_mutex_lock(&instance_mutex_);
      bool has_pending_instance = !pending_instances_.empty();
      pthread_mutex_unlock(&instance_mutex_);

      if (has_pending_instance) {
        if (max_connected_players_allowed() >= 0 &&
            GetNumConnectedPlayers() >= max_connected_players_allowed()) {
          // Already have a full game, auto-reject any additional players.
          RejectConnectionRequest(pending_instances_.front());
        } else {
          QueueNextState(kAdvertisingPromptedUser);
        }
      }
      break;
    }
    case kAdvertisingPromptedUser: {
      // check if we allowed connection
      int response = GetConnectionDialogResponse();
      if (response == 0) {
        // Reject removes the instance_id from pending
        pthread_mutex_lock(&instance_mutex_);
        auto instance = pending_instances_.front();
        pthread_mutex_unlock(&instance_mutex_);

        RejectConnectionRequest(instance);
        QueueNextState(kAdvertising);
      } else if (response == 1) {
        // Accept removes the instance_id from pending and adds it to connected
        pthread_mutex_lock(&instance_mutex_);
        auto instance = pending_instances_.front();
        pthread_mutex_unlock(&instance_mutex_);

        AcceptConnectionRequest(instance);
        QueueNextState(kAdvertising);
      }
      // Otherwise we haven't gotten a response yet.
      break;
    }
    default:
      // no default behavior
      break;
  }
}

void GPGMultiplayer::TransitionState(MultiplayerState old_state,
                                     MultiplayerState new_state) {
  if (old_state == new_state) {
    return;
  }
  // First, exit the old state.
  switch (old_state) {
    case kDiscovering:
    case kDiscoveringPromptedUser:
    case kDiscoveringWaitingForHost: {
      // Make sure we are totally leaving the "discovering" world.
      if (new_state != kDiscoveringPromptedUser &&
          new_state != kDiscoveringWaitingForHost &&
          new_state != kDiscovering) {
        nearby_connections_->StopDiscovery(service_id_);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: Stopped discovery.");
      }
      break;
    }
    case kAdvertising:
    case kAdvertisingPromptedUser: {
      // Make sure we are totally leaving the "advertising" world.
      if (new_state != kAdvertising && new_state != kAdvertisingPromptedUser) {
        nearby_connections_->StopAdvertising();
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: Stopped advertising");
      }
      break;
    }
    default:
      // no default behavior
      break;
  }

  // Then, set the state.
  state_ = new_state;

  // Then, activate the new state.
  switch (new_state) {
    case kIdle: {
      is_hosting_ = false;
      break;
    }
    case kAdvertising: {
      is_hosting_ = true;

      if (old_state != kAdvertisingPromptedUser) {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: Calling StartAdvertising()");
        nearby_connections_->StartAdvertising(
            my_instance_name_, app_identifiers_, gpg::Duration::zero(),
            [this](int64_t client_id,
                   gpg::StartAdvertisingResult const& result) {
              SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                          "Multiplayer: StartAdvertising() callback");
              this->StartAdvertisingCallback(result);
            },
            [this](int64_t client_id,
                   gpg::ConnectionRequest const& connection_request) {
              this->ConnectionRequestCallback(connection_request);
            });
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: StartAdvertising() returned");
      }
      break;
    }
    case kDiscovering: {
      is_hosting_ = false;
      if (old_state != kDiscoveringWaitingForHost &&
          old_state != kDiscoveringPromptedUser) {
        if (discovery_listener_ == nullptr) {
          discovery_listener_.reset(new DiscoveryListener(
              [this](gpg::EndpointDetails const& endpoint_details) {
                this->DiscoveryEndpointFoundCallback(endpoint_details);
              },
              [this](const std::string& instance_id) {
                this->DiscoveryEndpointLostCallback(instance_id);
              }));
        }
        nearby_connections_->StartDiscovery(service_id_, gpg::Duration::zero(),
                                            discovery_listener_.get());
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "Multiplayer: Started discovery");
      }
      break;
    }
    case kAdvertisingPromptedUser: {
      pthread_mutex_lock(&instance_mutex_);
      std::string instance_id = pending_instances_.front();
      std::string instance_name = instance_names_[instance_id];
      std::string message =
          std::string("Accept connection from \"") + instance_name + "\"?";
      pthread_mutex_unlock(&instance_mutex_);

      if (!DisplayConnectionDialog("Connection Request", message.c_str(), "Yes",
                                   "No")) {
        // Failed to display dialog, go back to previous state.
        QueueNextState(old_state);
      }
      break;
    }
    case kDiscoveringPromptedUser: {
      pthread_mutex_lock(&instance_mutex_);
      std::string instance_id = discovered_instances_.front();
      std::string instance_name = instance_names_[instance_id];
      std::string message =
          std::string("Connect to \"") + instance_name + "\"?";
      pthread_mutex_unlock(&instance_mutex_);

      if (!DisplayConnectionDialog("Host Found", message.c_str(), "Yes",
                                   "No")) {
        // Failed to display dialog, go back to previous state.
        QueueNextState(old_state);
      }
      break;
    }
    case kConnected: {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "Multiplayer: connection activated");
      break;
    }
    default:
      // no default behavior
      break;
  }
}

void GPGMultiplayer::QueueNextState(MultiplayerState next_state) {
  pthread_mutex_lock(&state_mutex_);
  next_states_.push(next_state);
  pthread_mutex_unlock(&state_mutex_);
}

bool GPGMultiplayer::SendMessage(const std::string& instance_id,
                                 const std::vector<uint8_t>& payload,
                                 bool reliable) {
  if (GetPlayerNumberByInstanceId(instance_id) == -1) {
    // Ensure we are actually connected to the specified instance.
    return false;
  } else {
  }

  if (reliable) {
    nearby_connections_->SendReliableMessage(instance_id, payload);
  } else {
    nearby_connections_->SendUnreliableMessage(instance_id, payload);
  }
  return true;
}

void GPGMultiplayer::BroadcastMessage(const std::vector<uint8_t>& payload,
                                      bool reliable) {
  pthread_mutex_lock(&instance_mutex_);
  std::vector<std::string> all_instances{connected_instances_.begin(),
                                         connected_instances_.end()};
  pthread_mutex_unlock(&instance_mutex_);
  if (reliable) {
    nearby_connections_->SendReliableMessage(all_instances, payload);
  } else {
    nearby_connections_->SendUnreliableMessage(all_instances, payload);
  }
}

bool GPGMultiplayer::HasMessage() {
  pthread_mutex_lock(&message_mutex_);
  bool empty = incoming_messages_.empty();
  pthread_mutex_unlock(&message_mutex_);
  return !empty;
}

std::pair<std::string, std::vector<uint8_t>> GPGMultiplayer::GetNextMessage() {
  if (HasMessage()) {
    pthread_mutex_lock(&message_mutex_);
    auto message = incoming_messages_.front();
    incoming_messages_.pop();
    pthread_mutex_unlock(&message_mutex_);
    return message;
  } else {
    std::pair<std::string, std::vector<uint8_t>> blank{"", {}};
    return blank;
  }
}

// Callbacks are below.

// Callback on the host when it starts advertising.
void GPGMultiplayer::StartAdvertisingCallback(
    gpg::StartAdvertisingResult const& result) {
  // We've started hosting
  if (result.status == gpg::StartAdvertisingResult::StatusCode::SUCCESS) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Multiplayer: Started advertising (name='%s')",
                result.local_endpoint_name.c_str());
  } else {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "Multiplayer: FAILED to start advertising");
    QueueNextState(kError);
  }
}

// Callback on the host when a client tries to connect.
void GPGMultiplayer::ConnectionRequestCallback(
    gpg::ConnectionRequest const& connection_request) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
              "Multiplayer: Incoming connection (instance_id=%s,name=%s)",
              connection_request.remote_endpoint_id.c_str(),
              connection_request.remote_endpoint_name.c_str());
  // process the incoming connection
  pthread_mutex_lock(&instance_mutex_);
  pending_instances_.push_back(connection_request.remote_endpoint_id);
  instance_names_[connection_request.remote_endpoint_id] =
      connection_request.remote_endpoint_name;
  pthread_mutex_unlock(&instance_mutex_);
}

// Callback on the client when it discovers a host.
void GPGMultiplayer::DiscoveryEndpointFoundCallback(
    gpg::EndpointDetails const& endpoint_details) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Multiplayer: Found endpoint");
  pthread_mutex_lock(&instance_mutex_);
  instance_names_[endpoint_details.endpoint_id] = endpoint_details.name;
  discovered_instances_.push_back(endpoint_details.endpoint_id);
  pthread_mutex_unlock(&instance_mutex_);
}

// Callback on the client when a host it previous discovered disappears.
void GPGMultiplayer::DiscoveryEndpointLostCallback(
    const std::string& instance_id) {
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Multiplayer: Lost endpoint");
  pthread_mutex_lock(&instance_mutex_);
  auto i = std::find(discovered_instances_.begin(), discovered_instances_.end(),
                     instance_id);
  if (i != discovered_instances_.end()) {
    discovered_instances_.erase(i);
  }
  pthread_mutex_unlock(&instance_mutex_);
}

// Callback on the client when it is either accepted or rejected by the host.
void GPGMultiplayer::ConnectionResponseCallback(
    gpg::ConnectionResponse const& response) {
  if (response.status == gpg::ConnectionResponse::StatusCode::ACCEPTED) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Multiplayer: Connected!");

    pthread_mutex_lock(&instance_mutex_);
    connected_instances_.push_back(response.remote_endpoint_id);
    UpdateConnectedInstances();
    pthread_mutex_unlock(&instance_mutex_);

    StopDiscovery();
  } else {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Multiplayer: Not connected...");
    state_ = kDiscovering;
  }
}

// Callback on host or client when an incoming message is received.
void GPGMultiplayer::MessageReceivedCallback(
    const std::string& instance_id, std::vector<uint8_t> const& payload,
    bool is_reliable) {
  pthread_mutex_lock(&message_mutex_);
  incoming_messages_.push({instance_id, payload});
  pthread_mutex_unlock(&message_mutex_);
}

// Callback on host or client when a connected instance disconnects.
void GPGMultiplayer::DisconnectedCallback(const std::string& instance_id) {
  pthread_mutex_lock(&instance_mutex_);
  auto i = std::find(connected_instances_.begin(), connected_instances_.end(),
                     instance_id);
  if (i != connected_instances_.end()) {
    connected_instances_.erase(i);
    UpdateConnectedInstances();
  }
  if (connected_instances_.size() == 0) {
    state_ = kIdle;
  }
  pthread_mutex_unlock(&instance_mutex_);
}

// Important: make sure you lock instance_mutex_ before calling this.
void GPGMultiplayer::UpdateConnectedInstances() {
  connected_instances_reverse_.clear();
  for (unsigned int i = 0; i < connected_instances_.size(); i++) {
    connected_instances_reverse_[connected_instances_[i]] = i;
  }
}
int GPGMultiplayer::GetNumConnectedPlayers() {
  pthread_mutex_lock(&instance_mutex_);
  int num_players = connected_instances_.size();
  pthread_mutex_unlock(&instance_mutex_);
  return num_players;
}

int GPGMultiplayer::GetPlayerNumberByInstanceId(
    const std::string& instance_id) {
  pthread_mutex_lock(&instance_mutex_);
  auto i = connected_instances_reverse_.find(instance_id);
  int player_num = ((i != connected_instances_reverse_.end()) ? i->second : -1);
  pthread_mutex_unlock(&instance_mutex_);
  return player_num;
}

std::string GPGMultiplayer::GetInstanceIdByPlayerNumber(unsigned int player) {
  pthread_mutex_lock(&instance_mutex_);
  // Copy out of thread-unsafe storage.
  std::string instance_id = (player < connected_instances_.size())
                                ? connected_instances_[player]
                                : "";
  pthread_mutex_unlock(&instance_mutex_);
  return instance_id;
}

// JNI calls for displaying the connection prompt and getting the results.

// Show a dialog box, allowing the user to reply to a Yes or No question.
bool GPGMultiplayer::DisplayConnectionDialog(const char* title,
                                             const char* question_text,
                                             const char* yes_text,
                                             const char* no_text) {
#ifdef __ANDROID__
  if (auto_connect_) {
    return true;
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "DisplayQuestionBox() start");
  bool question_shown = false;

  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID is_text_dialog_open =
      env->GetMethodID(fpl_class, "isTextDialogOpen", "()Z");
  jboolean open = env->CallBooleanMethod(activity, is_text_dialog_open);
  jmethodID get_query_dialog_response =
      env->GetMethodID(fpl_class, "getQueryDialogResponse", "()I");
  int response = env->CallIntMethod(activity, get_query_dialog_response);
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "DisplayQuestionBox() resp = %d",
              response);

  if (!open && response == -1) {
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "DisplayQuestionBox() show");

    jmethodID show_query_dialog =
        env->GetMethodID(fpl_class, "showQueryDialog",
                         "(Ljava/lang/String;Ljava/lang/String;"
                         "Ljava/lang/String;Ljava/lang/String;)V");
    jstring titlej = env->NewStringUTF(title);
    jstring questionj = env->NewStringUTF(question_text);
    jstring yesj = env->NewStringUTF(yes_text);
    jstring noj = env->NewStringUTF(no_text);
    env->CallVoidMethod(activity, show_query_dialog, titlej, questionj, yesj,
                        noj);
    env->DeleteLocalRef(titlej);
    env->DeleteLocalRef(questionj);
    env->DeleteLocalRef(yesj);
    env->DeleteLocalRef(noj);
    question_shown = true;
  }
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
  return question_shown;
#else
  (void)title;
  (void)question_text;
  (void)yes_text;
  (void)no_text;
  return false;
#endif
}

// Get the user's reply to the connection dialog (kDialogNo for No, kDialogYes
// for Yes), or kDialogWaiting if there is no result yet. Calling this consumes
// the result.
GPGMultiplayer::DialogResponse GPGMultiplayer::GetConnectionDialogResponse() {
#ifdef __ANDROID__
  // If we are set to automatically connect, pretend this is true.
  if (auto_connect_) {
    return kDialogYes;
  }
  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID get_query_dialog_response =
      env->GetMethodID(fpl_class, "getQueryDialogResponse", "()I");
  int result = env->CallIntMethod(activity, get_query_dialog_response);
  SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "GetQuestionResponse() = %d",
              result);
  if (result >= 0) {
    jmethodID reset_query_dialog_response =
        env->GetMethodID(fpl_class, "resetQueryDialogResponse", "()V");
    env->CallVoidMethod(activity, reset_query_dialog_response);
  }
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);

  switch (result) {
    case 0:
      return kDialogNo;
    case 1:
      return kDialogYes;
    default:
      return kDialogWaiting;
  }
#else
  return kDialogWaiting;
#endif
}

}  // namespace fpl
