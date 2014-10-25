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

#ifndef TOUCHSCREEN_CONTROLLER_H_
#define TOUCHSCREEN_CONTROLLER_H_

#include "precompiled.h"
#include <vector>
#include "SDL_keycode.h"
#include "audio_config_generated.h"
#include "character_state_machine_def_generated.h"
#include "config_generated.h"
#include "controller.h"
#include "input.h"
#include "player_controller.h"
#include "splat_common_generated.h"
#include "timeline_generated.h"

namespace fpl {
namespace splat {

// A TouchscreenController tracks the current state of a human player's logical
// inputs. It is responsible for polling the touchscreen for the current state
// of the physical inputs that map to logical actions.
class TouchscreenController : public Controller {
 public:
  TouchscreenController();

  // Set up a controller using the given input system and control scheme.
  // The input_system and scheme pointers are unowned and must outlive this
  // object.
  void Initialize(InputSystem* input_system, vec2 window_size,
                  const Config* config);

  // Map the input from the physical inputs to logical game inputs.
  virtual void AdvanceFrame(WorldTime delta_time);

  void HandleTouchButtonInput(int input, bool value);

 private:
  // A pointer to the object to query for the current input state.
  InputSystem* input_system_;
  vec2 window_size_;
  const Config* config_;

  const uint32_t kDirectionControls = LogicalInputs_Left | LogicalInputs_Right;
};

}  // splat
}  // fpl

#endif  // TOUCHSCREEN_CONTROLLER_H_

