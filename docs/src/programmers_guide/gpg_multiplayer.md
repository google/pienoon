GPGMultiplayer {#pie_noon_guide_gpg_multiplayer}
===============

## Introduction

GPGMultiplayer is a wrapper around the [Nearby Connections API][] (part
of [Google Play Game Services][]) which handles callbacks, connection
setup, sending/receiving messages, and disconnections/reconnections. It
is written in C++ and uses the [Google Play Games C++ SDK][].

## Overview

The Nearby Connections API uses a series of callbacks to handle various
events that occur, including device discovery, connection events,
incoming messages, and errors. These callbacks can trigger at any time
on different threads. When you are writing a game, you usually don't
want to have to worry about the nitty-gritty of setting up a connection,
synchronizing data across threads, structuring your callbacks properly,
prompting the user, etc. This library handles all that for you; all you
have to do is tell it what you want to do and call **Update()** every frame.

You can look at the GPGMultiplayer class in `gpg_multiplayer.h`.

## Initializing

Initializing GPGMultiplayer is simple: all you need to do is instantiate
it and call **Initialize()**. When you call Initialize, you pass in a
*service id*, which is how NearbyConnections identifies your specific
game's multiplayer connection and differentiates it from other games.

There are other optional bits of setup you can do before hosting or
joining a game:

* Call **AddAppIdentifier()** to add an application identifier, which
  allows users who don't have your app installed to find it in Google
  Play.
* Use **set_my_instance_name()** to change the name displayed in the
  prompt when you broadcast or connect to a game. By default this will
  be the device name.
* On the host, use **set_max_players_allowed()** to set the maximum
  number of clients that are allowed to connect. The default is
  unlimited.
* Set **set_auto_connect()** to *true* to enable automatically connecting
  without prompting the user. If you are hosting a game, this will
  automatically allow anyone to connect. If you are joining a game, this
  will automatically connect to the first game found.
* Set **set_allow_reconnecting()** to *false* (it defaults to true) to
  disallow anyone who was disconnected from reconnecting (not
  recommended).

After you have initialized GPGMultiplayer, you should ensure that you
call **Update()** once per frame or the library will not operate
correctly.

## Setting up a Connection

There are two sides to every connection: one host, and one or more
clients. The clients all connect to a host and can only send messages to
the host; the host allows all the clients to connect and can send
messages to any and all clients.

### Hosting a Game

To host a game, call **StartAdvertising()**. This will begin advertising
your game over the local Wi-Fi network. Any devices that are performing
*discovery* will see your game and prompt the user to connect. If you have
set auto_connect to true, the host will automatically accept connections;
otherwise the user will be prompted.

You can check how many players are connected at any time by calling
**GetNumConnectedPlayers()**. Once you are satisfied that you have
enough players connected, simply call **StopAdvertising()** to stop
allowing players to connect and start your game.

To abort hosting a game, call **ResetToIdle()** to disconnect all
players and return to an idle state.

### Joining a Game

To search for games to join, call **StartDiscovery()**. The game will
automatically search for hosted games on the local Wi-Fi network, and if
it finds one with the same service ID, it will automatically prompt the
user to connect.

You can check to see if you are connected to a game by calling
**IsConnected()**, which will return true if you successfully connected
to a game.

To abort trying to connect, call **StopDiscovery()** or
**ResetToIdle()** to return to an idle state.

## While Connected

While connected, **IsConnected()** will return true. If it ever returns
false, you have been disconnected and should show the user a message.

At any time you can call **is_hosting()** if you need to know whether
you are the host or a client.

### On the Host

While connected, you can send messages to other players and receive
messages sent by other players.

Clients are identified by their **Instance ID**, which is a string
unique for each player while the app is open. If you are the host, you
can send messages to a specific Instance ID or broadcast them to all
clients.

Clients also have a "Player Number" which corresponds to their connected
player slot (counting up from 0). You can get a player's number from
their instance ID or vice versa using **GetPlayerNumberByInstanceId()**
or **GetInstanceIdByPlayerNumber()**.

If you want to disconnect a specific player, call
**DisconnectInstance()**. You can disconnect all players by calling
**DisconnectAll()** or **ResetToIdle()**.

Remember to call **Update()** every frame!

#### Messages

To send a message to one client, call **SendMessage()**, or to send
message to all clients, call **BroadcastMessage()**. The message itself
is an arbitrary binary blob, but we recommend using [FlatBuffers][] to
structure, encode, and verify your multiplayer messages.

Incoming messages are placed in a queue. To check if you have any
incoming messages available, call **HasMessage()**. If that returns
true, you can call **GetNextMessage()** to get the next message from the
queue (it will be consumed); you may have multiple messages, so you
should keep getting and processing messages until HasMessage returns
false.

When you receive a message, you will also receive the instance ID of the
player who sent it; you can use **GetPlayerNumberByInstanceId()** to find
out their player number.

#### Reconnecting

By default, if a client disconnects, the library will automatically give
it a chance to reconnect and get its old player number back. Your game
may want to know when this happens (so it can, for example, give the
reconnected player a full update of the game state). You can see if any
players have reconnected by calling **HasReconnectedPlayer()**, and if
there are any, call **GetReconnectedPlayer()** to get their player
number.

Similarly to the message queue, this consumes the reconnected player
number, and you may have multiple reconnected players, so keep getting
them until HasReconnectedPlayer returns false.

The list of disconnected clients who are allowed to reconnect is cleared
whenever you call StartAdvertising or StartDiscovery to begin a new
multiplayer session.

If a client has disconnected but you still have other clients connected,
**IsConnected()** will still return true. You can find out if any
clients are connected by calling **GetNumConnectedPlayers()**, or if a
specific player has disconnected by calling
**GetInstanceIdByPlayerNumber()** (a disconnected player will have a
blank instance ID).

### On Clients

While connected, you can send messages to the host and receive messages
from the host.

You can disconnect from the host by using **DisconnectAll()** or
**ResetToIdle()**.

If **IsConnected()** ever returns false, you have been disconnected from
the host.

Remember to call **Update()** every frame!

#### Messages

Unlike the host, clients cannot communicate with arbitrary players, they
can only communicate with the host itself. Call **BroadcastMessage()**
on the client to send a message to the host; see above for details on
the message contents.

Clients use the same **HasMessage()** / **GetNextMessage()** method of
receiving incoming messages, although the instance ID will always be that
of the host.

## Dependencies

GPGMultiplayer uses a few Java methods via JNI to handle prompting the
user for connection setup. If you have your own UI system you can
replace these and remove the Java dependency entirely.

<br>

  [Nearby Connections API]: https://developers.google.com/games/services/android/nearby
  [Google Play Game Services]: http://developer.android.com/google/play-services/games.html
  [Google Play Games C++ SDK]: http://developers.google.com/games/services/downloads/
  [FlatBuffers]: https://google.github.io/flatbuffers/
