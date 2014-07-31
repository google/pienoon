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

#include <vector>
#include "controller.h"
#include "sdl_controller.h"
#include "character_state_machine_def_generated.h"

namespace fpl {
namespace splat {

SdlController::SdlController(InputSystem* input_system,
                             const ControlScheme* scheme)
    : input_system_(input_system),
      scheme_(scheme) {
}

void SdlController::AdvanceFrame() {
  for (auto it = scheme_->keybinds.begin();
       it != scheme_->keybinds.end(); ++it) {
    bool pressed = input_system_->GetButton(it->physical_input).went_down();
    if (pressed) {
      logical_inputs_ |= it->logical_input;
    } else {
      logical_inputs_ &= ~it->logical_input;
    }
  }
}

// static
const ControlScheme* ControlScheme::GetDefaultControlScheme(int i) {
  static const ControlScheme kDefaultSchemes[] = {
    {
      {
        { SDLK_w, LogicalInputs_ThrowPie },
        { SDLK_s, LogicalInputs_Deflect },
        { SDLK_a, LogicalInputs_Left },
        { SDLK_d, LogicalInputs_Right }
      }
    },
    {
      {
        { SDLK_i, LogicalInputs_ThrowPie },
        { SDLK_k, LogicalInputs_Deflect },
        { SDLK_j, LogicalInputs_Left },
        { SDLK_l, LogicalInputs_Right }
      }
    },
    {
      {
        { SDLK_UP, LogicalInputs_ThrowPie },
        { SDLK_DOWN, LogicalInputs_Deflect },
        { SDLK_LEFT, LogicalInputs_Left },
        { SDLK_RIGHT, LogicalInputs_Right }
      }
    },
    {
      {
        { SDLK_KP_8, LogicalInputs_ThrowPie },
        { SDLK_KP_5, LogicalInputs_Deflect },
        { SDLK_KP_3, LogicalInputs_Left },
        { SDLK_KP_6, LogicalInputs_Right }
      }
    }
  };
  return &kDefaultSchemes[i %
      (sizeof(kDefaultSchemes)/sizeof(kDefaultSchemes[0]))];
}

}  // splat
}  // fpl

