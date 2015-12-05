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
#include <algorithm>
#include "fplbase/utilities.h"
#include "gpg_multiplayer.h"

namespace fpl {

GPGMultiplayer::GPGMultiplayer()
    : message_mutex_(PTHREAD_MUTEX_INITIALIZER),
      instance_mutex_(PTHREAD_MUTEX_INITIALIZER),
      state_mutex_(PTHREAD_MUTEX_INITIALIZER) {}

bool GPGMultiplayer::Initialize(const std::string& service_id) {
  state_ = kIdle;
  is_hosting_ = false;
  allow_reconnecting_ = true;

  service_id_ = service_id;
  gpg::AndroidPlatformConfiguration platform_configuration;
  platform_configuration.SetActivity((jobject)fplbase::AndroidGetActivity());

  gpg::NearbyConnections::Builder nearby_builder;
  nearby_connections_ = nearby_builder.SetDefaultOnLog(gpg::LogLevel::VERBOSE)
                            .SetServiceId(service_id_)
                            .Create(platform_configuration);
  discovery_listener_.reset(nullptr);
  message_listener_.reset(nullptr);

  if (nearby_connections_ == nullptr) {
    fplbase::LogError(fplbase::kApplication,
             "GPGMultiplayer: Unable to build a NearbyConnections instance.");
    return false;
  }

  return true;
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
  fplbase::LogInfo(fplbase::kApplication,
          "GPGMultiplayer: Disconnect player (instance_id='%s')",
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
  fplbase::LogInfo(fplbase::kApplication,
                   "GPGMultiplayer: Disconnect all players");
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

  if (state() == kConnected || state() == kConnectedWithDisconnections) {
    QueueNextState(kIdle);
  }
}

void GPGMultiplayer::SendConnectionRequest(
    const std::string& host_instance_id) {
  if (message_listener_ == nullptr) {
    message_listener_.reset(new MessageListener(
        [this](const std::string& instance_id,
               std::vector<uint8_t> const& payload, bool is_reliable) {
          fplbase::LogInfo(fplbase::kApplication,
                  "GPGMultiplayer: OnMessageReceived(%s) callback",
                  instance_id.c_str());
          this->MessageReceivedCallback(instance_id, payload, is_reliable);
        },
        [this](const std::string& instance_id) {
          fplbase::LogInfo(fplbase::kApplication,
                  "GPGMultiplayer: OnDisconnect(%s) callback",
                  instance_id.c_str());
          this->DisconnectedCallback(instance_id);
        }));
  }
  LogInfo(fplbase::kApplication,
          "GPGMultiplayer: Sending connection request to %s",
          host_instance_id.c_str());

  // Immediately stop discovery once we start connecting.
  nearby_connections_->SendConnectionRequest(
      my_instance_name_, host_instance_id, std::vector<uint8_t>{},
      [this](int64_t client_id, gpg::ConnectionResponse const& response) {
        LogInfo(fplbase::kApplication,
                "GPGMultiplayer: OnConnectionResponse() callback");
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
          fplbase::LogInfo(fplbase::kApplication,
                  "GPGMultiplayer: OnMessageReceived(%s) callback",
                  instance_id.c_str());
          this->MessageReceivedCallback(instance_id, payload, is_reliable);
        },
        [this](const std::string& instance_id) {
          fplbase::LogInfo(fplbase::kApplication,
                  "GPGMultiplayer: OnDisconnect(%s) callback",
                  instance_id.c_str());
          this->DisconnectedCallback(instance_id);
        }));
  }
  fplbase::LogInfo(fplbase::kApplication,
                   "GPGMultiplayer: Accepting connection from %s",
          client_instance_id.c_str());
  nearby_connections_->AcceptConnectionRequest(
      client_instance_id, std::vector<uint8_t>{}, message_listener_.get());

  pthread_mutex_lock(&instance_mutex_);
  AddNewConnectedInstance(client_instance_id);
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
  LogInfo(fplbase::kApplication, "GPGMultiplayer: Rejecting connection from %s",
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
    LogInfo(fplbase::kApplication,
            "GPGMultiplayer: Exiting state %d to enter state %d", state(),
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
      break;
    }
    case kConnectedWithDisconnections: {
      pthread_mutex_lock(&instance_mutex_);
      bool has_disconnected_instance = !disconnected_instances_.empty();
      bool has_pending_instance = !pending_instances_.empty();
      pthread_mutex_unlock(&instance_mutex_);
      if (!has_disconnected_instance) {
        fplbase::LogInfo(fplbase::kApplication,
                "GPGMultiplayer: No disconnected instances.");
        QueueNextState(kConnected);
      } else if (has_pending_instance) {
        // Check if the pending instance is one of the disconnected ones.
        // If so, and we still have room, connect him.
        // Otherwise, reject.

        pthread_mutex_lock(&instance_mutex_);
        auto i = disconnected_instances_.find(pending_instances_.front());
        bool is_reconnection = (i != disconnected_instances_.end());
        pthread_mutex_unlock(&instance_mutex_);

        if (!is_reconnection) {
          // Not a disconnected instance. Reject.
          RejectConnectionRequest(pending_instances_.front());
        } else if (max_connected_players_allowed() >= 0 &&
                   GetNumConnectedPlayers() >=
                       max_connected_players_allowed()) {
          // Too many players to allow us back. Reject. But we might be
          // allowed back again in the future.
          RejectConnectionRequest(pending_instances_.front());
        } else {
          // A valid reconnecting instance. Allow.
          AcceptConnectionRequest(pending_instances_.front());
        }
      }
      break;
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
          // Prompt the user to allow the connection.
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
    default: {
      // no default behavior
      break;
    }
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
        fplbase::LogInfo(fplbase::kApplication,
                         "GPGMultiplayer: Stopped discovery.");
      }
      break;
    }
    case kConnectedWithDisconnections:
    case kAdvertising:
    case kAdvertisingPromptedUser: {
      // Make sure we are totally leaving the "advertising" world.
      if (new_state != kAdvertising && new_state != kAdvertisingPromptedUser &&
          new_state != kConnectedWithDisconnections) {
        nearby_connections_->StopAdvertising();
        fplbase::LogInfo(fplbase::kApplication,
                         "GPGMultiplayer: Stopped advertising");
      }
      break;
    }
    default: {
      // no default behavior
      break;
    }
  }

  // Then, set the state.
  state_ = new_state;

  // Then, activate the new state.
  switch (new_state) {
    case kIdle: {
      is_hosting_ = false;
      ClearDisconnectedInstances();
      break;
    }
    case kConnectedWithDisconnections:  // advertises also
    case kAdvertising: {
      is_hosting_ = true;
      if (new_state != kConnectedWithDisconnections) {
        ClearDisconnectedInstances();
      }

      if (old_state != kAdvertising && old_state != kAdvertisingPromptedUser &&
          old_state != kConnectedWithDisconnections) {
        nearby_connections_->StartAdvertising(
            my_instance_name_, app_identifiers_, gpg::Duration::zero(),
            [this](int64_t client_id,
                   gpg::StartAdvertisingResult const& result) {
              fplbase::LogInfo(fplbase::kApplication,
                      "GPGMultiplayer: StartAdvertising callback");
              this->StartAdvertisingCallback(result);
            },
            [this](int64_t client_id,
                   gpg::ConnectionRequest const& connection_request) {
              this->ConnectionRequestCallback(connection_request);
            });
        fplbase::LogInfo(fplbase::kApplication,
                         "GPGMultiplayer: Starting advertising");
      }
      break;
    }
    case kDiscovering: {
      is_hosting_ = false;
      ClearDisconnectedInstances();

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
        fplbase::LogInfo(fplbase::kApplication,
                         "GPGMultiplayer: Starting discovery");
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
      fplbase::LogInfo(fplbase::kApplication,
                       "GPGMultiplayer: Connection activated.");
      break;
    }
    default: {
      // no default behavior
      break;
    }
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

GPGMultiplayer::SenderAndMessage GPGMultiplayer::GetNextMessage() {
  if (HasMessage()) {
    pthread_mutex_lock(&message_mutex_);
    auto message = incoming_messages_.front();
    incoming_messages_.pop();
    pthread_mutex_unlock(&message_mutex_);
    return message;
  } else {
    SenderAndMessage blank{"", {}};
    return blank;
  }
}

bool GPGMultiplayer::HasReconnectedPlayer() {
  pthread_mutex_lock(&instance_mutex_);
  bool has_reconnected_player = !reconnected_players_.empty();
  pthread_mutex_unlock(&instance_mutex_);
  return has_reconnected_player;
}

int GPGMultiplayer::GetReconnectedPlayer() {
  pthread_mutex_lock(&instance_mutex_);
  if (reconnected_players_.empty()) {
    pthread_mutex_unlock(&instance_mutex_);
    return -1;
  } else {
    int player = reconnected_players_.front();
    reconnected_players_.pop();
    pthread_mutex_unlock(&instance_mutex_);
    return player;
  }
}

// Callbacks are below.

// Callback on the host when it starts advertising.
void GPGMultiplayer::StartAdvertisingCallback(
    gpg::StartAdvertisingResult const& result) {
  // We've started hosting
  if (result.status == gpg::StartAdvertisingResult::StatusCode::SUCCESS) {
    fplbase::LogInfo(fplbase::kApplication,
            "GPGMultiplayer: Started advertising (name='%s')",
            result.local_endpoint_name.c_str());
  } else {
    fplbase::LogError(fplbase::kApplication,
             "GPGMultiplayer: FAILED to start advertising, error code %d",
             result.status);
    if (state() == kConnectedWithDisconnections) {
      // We couldn't allow reconnections, sorry!
      ClearDisconnectedInstances();
      QueueNextState(kConnected);
    } else {
      QueueNextState(kError);
    }
  }
}

// Callback on the host when a client tries to connect.
void GPGMultiplayer::ConnectionRequestCallback(
    gpg::ConnectionRequest const& connection_request) {
  fplbase::LogInfo(fplbase::kApplication,
          "GPGMultiplayer: Incoming connection (instance_id=%s,name=%s)",
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
  fplbase::LogInfo(fplbase::kApplication, "GPGMultiplayer: Found endpoint");
  pthread_mutex_lock(&instance_mutex_);
  instance_names_[endpoint_details.endpoint_id] = endpoint_details.name;
  discovered_instances_.push_back(endpoint_details.endpoint_id);
  pthread_mutex_unlock(&instance_mutex_);
}

// Callback on the client when a host it previous discovered disappears.
void GPGMultiplayer::DiscoveryEndpointLostCallback(
    const std::string& instance_id) {
  fplbase::LogInfo(fplbase::kApplication, "GPGMultiplayer: Lost endpoint");
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
    fplbase::LogInfo(fplbase::kApplication, "GPGMultiplayer: Connected!");

    pthread_mutex_lock(&instance_mutex_);
    connected_instances_.push_back(response.remote_endpoint_id);
    UpdateConnectedInstances();
    pthread_mutex_unlock(&instance_mutex_);

    QueueNextState(kConnected);
  } else {
    fplbase::LogInfo(fplbase::kApplication,
            "GPGMultiplayer: Didn't connect, response status = %d",
            response.status);
    QueueNextState(kDiscovering);
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
  if (allow_reconnecting() && is_hosting() && IsConnected() &&
      GetNumConnectedPlayers() > 1) {
    // We are connected, and we have other instances connected besides this one.
    // Rather than simply disconnecting this instance, let's remember it so we
    // can give it back its connection slot if it tries to reconnect.
    fplbase::LogInfo(fplbase::kApplication,
            "GPGMultiplayer: Allowing reconnection by instance %s",
            instance_id.c_str());
    pthread_mutex_lock(&instance_mutex_);
    auto i = connected_instances_reverse_.find(instance_id);
    if (i != connected_instances_reverse_.end()) {
      int idx = i->second;
      disconnected_instances_[instance_id] = idx;
      // Put an empty instance ID as a placeholder for a disconnected instance.
      connected_instances_[idx] = "";
      UpdateConnectedInstances();
    }
    pthread_mutex_unlock(&instance_mutex_);
    // When the state is kConnectedWithDisconnections, we start advertising
    // again and allow only the disconnected instances to reconnect.
    QueueNextState(kConnectedWithDisconnections);
  } else {
    // Simply remove the connected index.
    pthread_mutex_lock(&instance_mutex_);
    auto i = std::find(connected_instances_.begin(), connected_instances_.end(),
                       instance_id);
    if (i != connected_instances_.end()) {
      connected_instances_.erase(i);
      UpdateConnectedInstances();
    }
    pthread_mutex_unlock(&instance_mutex_);
    if (IsConnected() && GetNumConnectedPlayers() == 0) {
      QueueNextState(kIdle);
    }
  }
}

// Important: make sure you lock instance_mutex_ before calling this.
void GPGMultiplayer::UpdateConnectedInstances() {
  connected_instances_reverse_.clear();
  for (unsigned int i = 0; i < connected_instances_.size(); i++) {
    connected_instances_reverse_[connected_instances_[i]] = i;
  }
}

// Important: make sure you lock instance_mutex_ before calling this.
int GPGMultiplayer::AddNewConnectedInstance(const std::string& instance_id) {
  int new_index = -1;
  // First, check if we are a reconnection.
  if (state() == kConnectedWithDisconnections) {
    auto i = disconnected_instances_.find(instance_id);
    if (i != disconnected_instances_.end()) {
      // We've got a disconnected instance, let's find a slot.
      if (connected_instances_[i->second] == "") {
        new_index = i->second;
        connected_instances_[new_index] = instance_id;
        disconnected_instances_.erase(i);
      }
      // If the slot was already taken, fall through to default behavior below.
    }
  }
  if (new_index == -1) {
    if (max_connected_players_allowed() < 0 ||
        (int)connected_instances_.size() < max_connected_players_allowed()) {
      // There's an empty player slot at the end, just connect there.
      new_index = connected_instances_.size();
      connected_instances_.push_back(instance_id);
    } else {
      // We're full, but there might be a reserved spot for a disconnected
      // player. We'll just use that. Sorry, prevoius player!
      for (unsigned int i = 0; i < connected_instances_.size(); i++) {
        if (connected_instances_[i] == "") {
          new_index = i;
          connected_instances_[new_index] = instance_id;
          break;
        }
      }
    }
  }
  if (state() == kConnectedWithDisconnections && new_index >= 0) {
    fplbase::LogInfo(fplbase::kApplication,
            "GPGMultiplayer: Connected a reconnected player");
    reconnected_players_.push(new_index);
  }
  fplbase::LogInfo(fplbase::kApplication,
                   "GPGMultiplayer: Instance %s goes in slot %d",
          instance_id.c_str(), new_index);
  return new_index;
}

void GPGMultiplayer::ClearDisconnectedInstances() {
  disconnected_instances_.clear();
  while (!reconnected_players_.empty()) reconnected_players_.pop();
}

int GPGMultiplayer::GetNumConnectedPlayers() {
  int num_players = 0;
  pthread_mutex_lock(&instance_mutex_);
  for (auto instance_id : connected_instances_) {
    if (instance_id != "") {
      num_players++;
    }
  }
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
  bool question_shown = false;

  JNIEnv* env = fplbase::AndroidGetJNIEnv();
  jobject activity = fplbase::AndroidGetActivity();
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID is_text_dialog_open =
      env->GetMethodID(fpl_class, "isTextDialogOpen", "()Z");
  jboolean open = env->CallBooleanMethod(activity, is_text_dialog_open);
  jmethodID get_query_dialog_response =
      env->GetMethodID(fpl_class, "getQueryDialogResponse", "()I");
  int response = env->CallIntMethod(activity, get_query_dialog_response);

  if (!open && response == -1) {
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
  JNIEnv* env = fplbase::AndroidGetJNIEnv();
  jobject activity = fplbase::AndroidGetActivity();
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID get_query_dialog_response =
      env->GetMethodID(fpl_class, "getQueryDialogResponse", "()I");
  int result = env->CallIntMethod(activity, get_query_dialog_response);
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
