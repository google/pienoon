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
#include "character_state_machine_def_generated.h"
#include "common.h"
#include "controller.h"
#include "player_controller.h"
#include "timeline_generated.h"

namespace fpl {
namespace pie_noon {

PlayerController::PlayerController() : Controller(kTypePlayer) {}

void PlayerController::Initialize(fplbase::InputSystem* input_system,
                                  const ControlScheme* scheme) {
  input_system_ = input_system;
  scheme_ = scheme;
}

void PlayerController::AdvanceFrame(WorldTime /*delta_time*/) {
  ClearAllLogicalInputs();
  for (size_t i = 0; i < scheme_->keybinds.size(); ++i) {
    const Keybind& keybind = scheme_->keybinds[i];
    if (input_system_->GetButton(keybind.physical_input).is_down()) {
      is_down_ |= keybind.logical_input;
    }
    if (input_system_->GetButton(keybind.physical_input).went_down()) {
      went_down_ |= keybind.logical_input;
    }
    if (input_system_->GetButton(keybind.physical_input).went_up()) {
      went_up_ |= keybind.logical_input;
    }
  }
}

const Keybind kKeyBinds0[] = {
    {fplbase::FPLK_e, LogicalInputs_Select},
    {fplbase::FPLK_w, LogicalInputs_ThrowPie},
    {fplbase::FPLK_s, LogicalInputs_Deflect},
    {fplbase::FPLK_w, LogicalInputs_Up},
    {fplbase::FPLK_s, LogicalInputs_Down},
    {fplbase::FPLK_a, LogicalInputs_Left},
    {fplbase::FPLK_d, LogicalInputs_Right}};

const Keybind kKeyBinds1[] = {
    {fplbase::FPLK_o, LogicalInputs_Select},
    {fplbase::FPLK_i, LogicalInputs_ThrowPie},
    {fplbase::FPLK_k, LogicalInputs_Deflect},
    {fplbase::FPLK_i, LogicalInputs_Up},
    {fplbase::FPLK_k, LogicalInputs_Down},
    {fplbase::FPLK_j, LogicalInputs_Left},
    {fplbase::FPLK_l, LogicalInputs_Right}};

const Keybind kKeyBinds2[] = {
    {fplbase::FPLK_RETURN, LogicalInputs_Select},
    {fplbase::FPLK_UP, LogicalInputs_ThrowPie},
    {fplbase::FPLK_DOWN, LogicalInputs_Deflect},
    {fplbase::FPLK_UP, LogicalInputs_Up},
    {fplbase::FPLK_DOWN, LogicalInputs_Down},
    {fplbase::FPLK_LEFT, LogicalInputs_Left},
    {fplbase::FPLK_RIGHT, LogicalInputs_Right}};

const Keybind kKeyBinds3[] = {
    {fplbase::FPLK_KP_ENTER, LogicalInputs_Select},
    {fplbase::FPLK_KP_8, LogicalInputs_ThrowPie},
    {fplbase::FPLK_KP_5, LogicalInputs_Deflect},
    {fplbase::FPLK_KP_8, LogicalInputs_Up},
    {fplbase::FPLK_KP_5, LogicalInputs_Down},
    {fplbase::FPLK_KP_4, LogicalInputs_Left},
    {fplbase::FPLK_KP_6, LogicalInputs_Right}};

const ControlScheme ControlScheme::kDefaultSchemes[] = {
    {std::vector<Keybind>(kKeyBinds0, &kKeyBinds0[PIE_ARRAYSIZE(kKeyBinds0)])},
    {std::vector<Keybind>(kKeyBinds1, &kKeyBinds1[PIE_ARRAYSIZE(kKeyBinds1)])},
    {std::vector<Keybind>(kKeyBinds2, &kKeyBinds2[PIE_ARRAYSIZE(kKeyBinds2)])},
    {std::vector<Keybind>(kKeyBinds3, &kKeyBinds3[PIE_ARRAYSIZE(kKeyBinds3)])}};

const int ControlScheme::kDefinedControlSchemeCount =
    PIE_ARRAYSIZE(kDefaultSchemes);

// static
const ControlScheme* ControlScheme::GetDefaultControlScheme(int i) {
  return &kDefaultSchemes[i % PIE_ARRAYSIZE(kDefaultSchemes)];
}

}  // pie_noon
}  // fpl
