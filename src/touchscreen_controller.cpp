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
#include "touchscreen_controller.h"
#include "utilities.h"

namespace fpl {
namespace splat {

TouchscreenController::TouchscreenController()
  : Controller(kTypeTouchScreen),
    input_system_(nullptr)
    {}

// The touchscreen mapping is defined in the config.fbs file.
// It currently looks like this:
//
// +---------------------------------+
// |                                 |
// |                                 |
// |            throw                |
// |                                 |
// +------+-------------------+------+
// |      |                   |      |
// | turn |      block        | turn |
// | left |                   | right|
// +------+-------------------+------+
//

void TouchscreenController::Initialize(InputSystem* input_system,
                                       vec2 window_size,
                                       const Config* config) {
  input_system_ = input_system;
  window_size_ = window_size;
  ClearAllLogicalInputs();
  config_ = config;
  buttons_to_debounce_ = 0;
}

// Called from outside, based on screen touches.
// Basically just translates button inputs into logical inputs.
void TouchscreenController::HandleTouchButtonInput(int input, bool value) {
  int logicalInput = 0;
  switch (input) {
    case ButtonId_Left:
      logicalInput = LogicalInputs_Left;
      buttons_to_debounce_ &= ~logicalInput;
      break;
    case ButtonId_Right:
      logicalInput = LogicalInputs_Right;
      buttons_to_debounce_ &= ~logicalInput;
      break;
    case ButtonId_Attack:
      logicalInput = LogicalInputs_ThrowPie;
      buttons_to_debounce_ &= ~logicalInput;
      break;
    case ButtonId_Defend:
      logicalInput = LogicalInputs_Deflect;
      break;
    default:
      break;
  }
  SetLogicalInputs(logicalInput, value);
}

void TouchscreenController::AdvanceFrame(WorldTime /*delta_time*/) {
  //ClearAllLogicalInputs();
  went_down_ = went_up_ = 0;
  SetLogicalInputs(buttons_to_debounce_, false);

  // Sort of a hack:  Block is the only button we want to be able
  // to hold down - everything else needs to be debounced.
  buttons_to_debounce_ = ~0;
}

}  // splat
}  // fpl

