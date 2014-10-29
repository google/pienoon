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
#include <vector>
#include "common.h"
#include "controller.h"
#include "gamepad_controller.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"

namespace fpl {
namespace pie_noon {

GamepadController::GamepadController()
    : Controller(kTypeGamepad),
      input_system_(nullptr),
      joystick_id_(kInvalidJoystickId) {
}

void GamepadController::Initialize(InputSystem* input_system,
                                   SDL_JoystickID joystick_id) {
  input_system_ = input_system;
  joystick_id_ = joystick_id;
  ClearAllLogicalInputs();
}

// How far an analog stick has to be tilted before we count it.
static const float kAnalogDeadZone = 0.25f;

void GamepadController::AdvanceFrame(WorldTime /*delta_time*/) {
  went_down_ = went_up_ = 0;

  Joystick joystick = input_system_->GetJoystick(joystick_id_);
  SetLogicalInputs(LogicalInputs_Left,
      (joystick.GetAxis(0).Value() < -kAnalogDeadZone ||
      joystick.GetHat(0).Value().x() < 0));
  SetLogicalInputs(LogicalInputs_Right,
      (joystick.GetAxis(0).Value() > kAnalogDeadZone ||
      joystick.GetHat(0).Value().x() > 0));
  SetLogicalInputs(LogicalInputs_ThrowPie, joystick.GetButton(0).is_down());
  SetLogicalInputs(LogicalInputs_Deflect, joystick.GetButton(1).is_down());
}

}  // pie_noon
}  // fpl



