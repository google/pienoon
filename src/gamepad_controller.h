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

#ifndef GAMEPAD_CONTROLLER_H_
#define GAMEPAD_CONTROLLER_H_

#include <vector>

#include "precompiled.h"

#include "SDL_keycode.h"
#include "controller.h"
#include "player_controller.h"
#include "input.h"

namespace fpl {
namespace pie_noon {

static const SDL_JoystickID kInvalidControllerId = -1;

// A GamepadController tracks the current state of a human player's logical
// inputs. It is responsible for polling the gamepad for the current state
// of the physical inputs that map to logical actions.
class GamepadController : public Controller {
 public:
  GamepadController();

  // Set up a controller using the given input system and control scheme.
  // The input_system and scheme pointers are unowned and must outlive this
  // object.
  void Initialize(InputSystem* input_system, SDL_JoystickID joystick_id);

  // Map the input from the physical inputs to logical game inputs.
  virtual void AdvanceFrame(WorldTime delta_time);

 private:
  // A pointer to the object to query for the current input state.
  InputSystem* input_system_;

#ifdef ANDROID_GAMEPAD
  // The device ID of the controller we're listening to.
  AndroidInputDeviceId controller_id_;
#endif  // ANDROID_GAMEPAD
};

}  // pie_noon
}  // fpl

#endif  // GAMEPAD_CONTROLLER_H_
