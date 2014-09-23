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

namespace fpl {
namespace splat {

TouchscreenController::TouchscreenController()
  : Controller(kTypeTouchScreen),
    input_system_(nullptr) {}

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
}

void TouchscreenController::AdvanceFrame(WorldTime /*delta_time*/) {
  assert(input_system_);
  uint touch_inputs = 0;
  went_down_ = went_up_ = 0;

  for (size_t i = 0; i < input_system_->pointers_.size(); i++) {
    Pointer pointer = input_system_->pointers_[i];
    if (!pointer.used ||
        !input_system_->GetPointerButton(pointer.id).is_down()) {
      continue;
    }

    vec2 pointer_pos = vec2(pointer.mousepos);
    pointer_pos.x() /= window_size_.x();
    pointer_pos.y() /= window_size_.y();

    for (size_t j = 0; j < config_->touchscreen_zones()->Length(); j++) {
      const TouchInputZone* zone = config_->touchscreen_zones()->Get(j);

      if (pointer_pos.x() >= zone->left() &&
          pointer_pos.x() < zone->right() &&
          pointer_pos.y() >= zone->top() &&
          pointer_pos.y() < zone->bottom()) {
        touch_inputs |= zone->input_type();
        break;  // Pointer touches are consumed by the first zone that matches.
      }
    }
  }

  SetLogicalInputs(LogicalInputs_Left,
                   (touch_inputs & TouchZoneInputType_Left) != 0);
  SetLogicalInputs(LogicalInputs_Right,
                   (touch_inputs & TouchZoneInputType_Right) != 0);
  SetLogicalInputs(LogicalInputs_ThrowPie,
                   (touch_inputs & TouchZoneInputType_Attack) != 0);
  SetLogicalInputs(LogicalInputs_Deflect,
                   (touch_inputs & TouchZoneInputType_Defend) != 0);
}

}  // splat
}  // fpl
