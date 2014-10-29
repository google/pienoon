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

#ifndef PLAYER_CONTROLLER_H_
#define PLAYER_CONTROLLER_H_

#include "precompiled.h"

#include "SDL_keycode.h"
#include "controller.h"
#include "input.h"

namespace fpl {
namespace pie_noon {

// A keybind represents a mapping between a physical key and a game specific
// input.
struct Keybind {
  // The physical button that must pressed.
  SDL_Keycode physical_input;

  // The game specific input the button represents
  uint32_t logical_input;
};

// A control scheme consists of a mapping between physical input buttons and
// keys and the logical game actions.
class ControlScheme {
 public:
  static const int kDefinedControlSchemeCount;
  static const ControlScheme kDefaultSchemes[];

  std::vector<Keybind> keybinds;

  // Returns one of four default control schemes.
  // Eventually we probably want these to be data driven, but this will work in
  // the short term.
  static const ControlScheme* GetDefaultControlScheme(int i);
};

// A PlayerController tracks the current state of a human player's logical
// inputs. It is responsible for polling the InputSystem for the current state
// of the physical inputs that map to logical actions.
class PlayerController : public Controller {
 public:
  PlayerController();

  // Set up a controller using the given input system and control scheme.
  // The input_system and scheme pointers are unowned and must outlive this
  // object.
  void Initialize(InputSystem* input_system, const ControlScheme* scheme);

  // Map the input from the physical inputs to logical game inputs.
  virtual void AdvanceFrame(WorldTime delta_time);

 private:
  // A pointer to the object to query for the current input state.
  InputSystem* input_system_;

  // The control scheme for this controller.
  const ControlScheme* scheme_;
};

}  // pie_noon
}  // fpl

#endif  // PLAYER_CONTROLLER_H_

