// Copyright 2015 Google Inc. All rights reserved.
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
#include "cardboard_controller.h"
#include "character_state_machine_def_generated.h"
#include "controller.h"

namespace fpl {
namespace pie_noon {

CardboardController::CardboardController()
    : Controller(kTypeCardboard), input_system_(nullptr) {}

void CardboardController::Initialize(InputSystem* input_system) {
  input_system_ = input_system;
  ClearAllLogicalInputs();
}

void CardboardController::AdvanceFrame(WorldTime /*delta_time*/) {
  ClearAllLogicalInputs();
#ifdef ANDROID_CARDBOARD
  if (input_system_->cardboard_input().triggered()) {
    SetLogicalInputs(LogicalInputs_Select, true);
    SetLogicalInputs(LogicalInputs_ThrowPie, true);
  }
#endif  // ANDROID_CARDBOARD
}

}  // pie_noon
}  // fpl
