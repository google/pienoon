Pie Noon Input Controllers {#pie_noon_guide_controllers}
===================

#  Introduction {#input_controllers_introduction}

This document outlines the controller components that are used to abstract input
in Pie Noon.

# Overview {#input_controllers_overview}

Pie Noon uses the concept of controllers  (controller.h) to abstract input from
multiple controller types into one consistent interface.  Each controller serves
as an adapter between a specific type of input (keyboard, touchscreen, gamepad,
etc) and the game.

This way, the game can ignore the details of how the input is being created, and
simply operate on a consistent set of input fields.

# Logical Inputs {#input_controllers_logical_inputs}

Input is broken down into a number of "Logical Inputs", enumerated in
`character_state_machine.fbs`.  These correspond to basic actions that a player
can trigger.  Things like "navigate left", "Throw a pie", "select a menu
option" are all logical inputs.  The controller classes then serve primarily to
serve as a translation layer, and map things like "Keyevent_Gamepad_Left" onto
their corresponding logical input.  (LogicalInputs_Left in this case.)

Note that there are also several logical inputs that are not triggered by the
player's input, but rather by the game state.  (`NoHealth`, `AnimationEnd`,
`JoinedGame`, etc.)

# Controller Handling {#input_controller_handling}

There are several different types of controllers, each responsible for handling
a different sort of input.  The main game (`pie_noon_game.h/.cpp`) maintains
maintains a list of all controllers it is listening to, as well as a separate
list of all controllers that are currently in the game.  Whenever a player
indicates that they want to join the game, the corresponding controller for that
player is moved into the list of controllers in the game.  When a game starts,
the remaining slots are populated by ai controllers.

Each controller is responsible for implementing an `AdvanceFrame()` method,
which the game guarantees it will call once per update.  It typically uses this
function to query whatever input it is listening for, and translate it into
logical input bits.

## Controller {#input_handling_controller}

The base class for all controllers.  In addition to defining a virtual method
for `AdvanceFrame()`, it also contains some common code for bookkeeping and
accessors for the logical input bits.

## PlayerController {#input_handling_playercontroller}

Represents a controller that listens to keyboard input.  It contains several
different keyboard configurations, and each instance can be set to listen to a
different one.

## GamepadController {#input_handling_controller}

A controller that listens to gamepad input.  Note that because these use the
android event system to track gamepad inputs, they are ommitted from non-android
builds.

## AI controllers {#input_handling_aicontroller}

These controllers are somewhat unique - they don't translate inputs from a
player into game inputs - instead they represent computer-controlled players,
and as such take the gamestate itself as input, and choose moves based on what
would make sense for them to do.

There are several constants in config.json that control the behavior of the
computer-controlled players.
