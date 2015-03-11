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

// gpg_multiplayer.h
//
// Multiplayer library using the Nearby Connections API in the Google Play
// Games SDK.
//
// This library wraps the new NearbyConnections library so that game code
// doesn't have to worry about:
//  - callbacks
//  - synchronization/thread safety
//  - prompting the user to connect
//  - error handling (not 100% yet)
//
// It provides an interface where the game code can just call Update() each
// frame, and then retreive incoming messages from a queue whenever is
// convenient.
//
// To start, call Initialize() and pass in a unique service ID for your game.
// After this point you should start calling Update() each frame. You can also
// call set_my_instance_name() to set a human-readable name for your instance
// (maybe your Play Games full name, or your device's name).
//
// When you want to host a game, call StartAdvertising(). This library handles
// prompting the player when someone connects to accept/reject them.  When you
// have enough connected players simply call StopAdvertising() and you are all
// connected up and ready to go.
//
// When you want to join a game, call StartDiscovery(). This library handles
// prompting the player when they find a host to connect to. Once you have
// connected to a host and they have accepted you, you will be fully connected.
//
// To send a message to a specific user (as the host), call SendMessage(). To
// send a message to all other users (as either host or client), call
// BroadcastMessage. Only the host can see all the players.
//
// To receive, call HasMessage() to check if there are any messages available,
// then GetNextMessage() to get the next incoming message from the queue.

#ifndef GPG_MULTIPLAYER_H
#define GPG_MULTIPLAYER_H

#include <list>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace fpl {

class GPGMultiplayer {
 public:
  GPGMultiplayer();

  enum MultiplayerState {
    // Starting state, you aren't connected, broadcasting, or scanning.
    kIdle = 0,
    // The host is advertising its connection.
    kAdvertising = 1,
    // The host is advertising its connection and has prompted the user.
    kAdvertisingPromptedUser = 2,
    // The client is scanning for hosts.
    kDiscovering = 3,
    // The client is asking the user whether to connect to a specific host.
    kDiscoveringPromptedUser = 4,
    // The user chose to connect, we are waiting for the host to accept us.
    kDiscoveringWaitingForHost = 5,
    // We are fully connected to the other side.
    kConnected = 6,
    // A connection error occurred.
    kError = 7
  };

  // The user's response to a connection dialog.
  enum DialogResponse {
    // The user responded "No" to the prompt.
    kDialogNo,
    // The user responded "Yes" to the prompt.
    kDialogYes,
    // The user has not yet responded to the prompt, we are still waiting.
    kDialogWaiting,
  };

  // Initialize the connection manager, set up callbacks, etc.
  // Call this before doing anything else but after initializing
  // GameServices. service_id should be unique for your game.
  bool Initialize(const std::string& service_id);

  // Add an app identifier that is used for linking to your device's app store,
  // if a user scanning for games doesn't have this one installed.
  void AddAppIdentifier(const std::string& identifier);

  // Update, call this once per frame if you can.
  void Update();

  // Broadcast that you are hosting a game. To change the name from the default,
  // call set_my_instance_name() before calling this.
  void StartAdvertising();
  // Stop broadcasting your game; if you have connected instances, the state
  // will become kConnected, otherwise you will go back to kIdle.
  void StopAdvertising();

  // Look for games to join as a client. To change your name from the
  // default, call set_my_instance_name() before calling this.
  void StartDiscovery();
  // Stop looking for games to join, sets the state back to kIdle.
  void StopDiscovery();

  // Disconnect a specific other player (host / client).
  void DisconnectInstance(const std::string& instance_id);
  // Disconnect all other players (host / client).
  void DisconnectAll();

  // Stop whatever you are doing, disconnect all players, and reset back to
  // idle.
  void ResetToIdle();

  // Set the name that will be shown to clients performing discovery,
  // or shown to hosts when you send a connection request.
  void set_my_instance_name(const std::string& my_instance_name) {
    my_instance_name_ = my_instance_name;
  }

  MultiplayerState state() const { return state_; }

  // Are you fully connected and no longer advertising or discovering?
  bool IsConnected() const { return (state() == kConnected); }

  // Returns whether you are advertising.
  bool IsAdvertising() const {
    return (state() == kAdvertising || state() == kAdvertisingPromptedUser);
  }
  // Returns whether you are discovering.
  bool IsDiscovering() const {
    return (state() == kDiscovering || state() == kDiscoveringPromptedUser ||
            state() == kDiscoveringWaitingForHost);
  }
  // Returns whether you have had a connection error.
  bool HasError() const { return (state() == kError); }

  // Get the number of players we have connected. If you are a client, this will
  // be at most 1, since you are only connected to the host.
  int GetNumConnectedPlayers();

  // Return true if this user is the host, false if you are a client.
  bool is_hosting() const { return is_hosting_; }

  // Get the instance ID of a connected instance by player number, or empty if
  // you pass in an invalid player number. This locks a mutex so you're best off
  // caching this value for performance.
  std::string GetInstanceIdByPlayerNumber(unsigned int player);

  // Get the player number of a connected instance by instance ID (the reverse
  // of GetInstanceIdByPlayerNumber), or -1 if there is no such connected
  // instance. This locks a mutex so you're best off caching this value for
  // performance.
  int GetPlayerNumberByInstanceId(const std::string& instance_id);

  // Send a message to a specific instance. Returns false if you are not
  // connected to that instance (in which case nothing is sent).
  bool SendMessage(const std::string& instance_id,
                   const std::vector<uint8_t>& payload, bool reliable);

  // For the host: broadcast to all clients. For the client, sends just to host.
  void BroadcastMessage(const std::vector<uint8_t>& payload, bool reliable);

  // Returns true if there are one or more messages available in the queue.
  // You would then call GetNextMessage() to retrieve the next message.
  bool HasMessage();

  // Get the latest incoming message, or a blank vector if there is none.
  // In the pair, first = the sender's instance_id, second = the message.
  std::pair<std::string, std::vector<uint8_t>> GetNextMessage();

  // On the host, set the maximum number of players allowed to connect (not
  // counting the host itself). Set this to a negative number to allow an
  // unlimited number of players.
  void set_max_connected_players_allowed(int players) {
    max_connected_players_allowed_ = players;
  }
  int max_connected_players_allowed() const {
    return max_connected_players_allowed_;
  }

  // On the host, set this to true to automatically allow users to connect.
  // On the client, set this to true to automatically connect to the first host.
  void set_auto_connect(bool b) { auto_connect_ = b; }
  bool auto_connect() { return auto_connect_; }

 private:
  // Enter a new state, exiting the previous one first.
  void TransitionState(MultiplayerState old_state, MultiplayerState new_state);

  // Queue up the next state to go into at the next Update.
  void QueueNextState(MultiplayerState next_state);

  // On the client, request a connection from a host you have discovered.
  void SendConnectionRequest(const std::string& host_instance_id);
  // On the host, accept a client's connection request.
  void AcceptConnectionRequest(const std::string& client_instance_id);
  // On the host, reject a client's connection request, disconnecting them.
  void RejectConnectionRequest(const std::string& client_instance_id);
  // On the host, reject all pending connection requests.
  void RejectAllConnectionRequests();

  // Listens for hosts that are advertising.
  class DiscoveryListener : public gpg::IEndpointDiscoveryListener {
   public:
    explicit DiscoveryListener(
        std::function<void(gpg::EndpointDetails const&)> endpointfound_callback,
        std::function<void(const std::string&)> endpointremoved_callback)
        : endpointfound_callback_(endpointfound_callback),
          endpointremoved_callback_(endpointremoved_callback) {}
    void OnEndpointFound(int64_t client_id,
                         gpg::EndpointDetails const& endpoint_details) {
      endpointfound_callback_(endpoint_details);
    }
    void OnEndpointLost(int64_t client_id, const std::string& instance_id) {
      endpointremoved_callback_(instance_id);
    }

   private:
    std::function<void(gpg::EndpointDetails const&)> endpointfound_callback_;
    std::function<void(const std::string&)> endpointremoved_callback_;
  };

  // Listens for messages or disconnects from connected instances.
  class MessageListener : public gpg::IMessageListener {
   public:
    explicit MessageListener(
        std::function<void(const std::string&, std::vector<uint8_t>, bool)>
            message_received_callback,
        std::function<void(const std::string&)> disconnected_callback)
        : message_received_callback_(message_received_callback),
          disconnected_callback_(disconnected_callback) {}
    void OnMessageReceived(int64_t /* client_id */,
                           const std::string& instance_id,
                           std::vector<uint8_t> const& payload,
                           bool is_reliable) {
      message_received_callback_(instance_id, payload, is_reliable);
    }
    void OnDisconnected(int64_t /* client_id */,
                        const std::string& instance_id) {
      disconnected_callback_(instance_id);
    }

   private:
    std::function<void(const std::string&, std::vector<uint8_t>, bool)>
        message_received_callback_;
    std::function<void(const std::string&)> disconnected_callback_;
  };

  void StartAdvertisingCallback(gpg::StartAdvertisingResult const& info);
  void ConnectionRequestCallback(
      gpg::ConnectionRequest const& connection_request);
  void DiscoveryEndpointFoundCallback(
      gpg::EndpointDetails const& endpoint_details);
  void DiscoveryEndpointLostCallback(const std::string& instance_id);
  void ConnectionResponseCallback(gpg::ConnectionResponse const& response);
  void MessageReceivedCallback(const std::string& instance_id,
                               std::vector<uint8_t> const& payload,
                               bool is_reliable);
  void DisconnectedCallback(const std::string& instance_id);

  DialogResponse GetConnectionDialogResponse();
  bool DisplayConnectionDialog(const char* title, const char* question_text,
                               const char* yes_text, const char* no_text);

  // Update connected_instances_reverse_ to match to connected_instances_.
  void UpdateConnectedInstances();

  // The NearbyConnections library.
  std::unique_ptr<gpg::NearbyConnections> nearby_connections_;
  // The DiscoveryListener we've instantiated.
  std::unique_ptr<DiscoveryListener> discovery_listener_;
  // The MessageListener we've instantiated.
  std::unique_ptr<MessageListener> message_listener_;

  std::string service_id_;
  std::vector<gpg::AppIdentifier> app_identifiers_;

  // Keep track of fully-connected instances here. Lock instance_mutex_ before
  // using.
  std::vector<std::string> connected_instances_;
  // Keep a reverse map of instance IDs to vector indices. Lock instance_mutex_
  // before using.
  std::map<std::string, int> connected_instances_reverse_;
  // The host keeps track of instances that are trying to connect. Lock
  // instance_mutex_ before using.
  std::list<std::string> pending_instances_;
  // The client keeps track of the instances it has discovered. Lock
  // instance_mutex_ before using.
  std::list<std::string> discovered_instances_;
  // Keep a mapping of instance IDs to full names. Lock instance_mutex_ before
  // using.
  std::map<std::string, std::string> instance_names_;

  typedef std::queue<std::pair<std::string, std::vector<uint8_t>>> MessageQueue;

  // List of incoming messages. Lock message_mutex_ before using.
  MessageQueue incoming_messages_;

  // Our current state.
  MultiplayerState state_;
  // Our next state(s). Will enter the next one during the next Update().
  std::queue<MultiplayerState> next_states_;

  std::string my_instance_name_;
  int max_connected_players_allowed_;  // 0 to allow any number

  // Mutex for the incoming_messages_ queue.
  pthread_mutex_t message_mutex_;

  // Mutex for instance management: connected_instances_, pending_instances_,
  // discovered_instances, and instance_names_.
  pthread_mutex_t instance_mutex_;

  // Mutex for the next_states_ queue. Needed only in rare occasions like a
  // callback setting an error condition.
  pthread_mutex_t state_mutex_;

  bool is_hosting_;  // only true if we are the host.
  bool auto_connect_;
};

}  // namespace fpl

#endif  // GPG_MULTIPLAYER_H
