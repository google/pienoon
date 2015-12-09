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

using mathfu::vec3;
using mathfu::mat4;
using motive::Angle;

CardboardController::CardboardController()
    : Controller(kTypeCardboard),
      game_state_(nullptr),
      input_system_(nullptr) {}

void CardboardController::Initialize(GameState* game_state,
                                     fplbase::InputSystem* input_system) {
  game_state_ = game_state;
  input_system_ = input_system;
  ClearAllLogicalInputs();
}

void CardboardController::AdvanceFrame(WorldTime /*delta_time*/) {
  ClearAllLogicalInputs();

  // If not in Cardboard mode, there is no need to update this.
  if (!game_state_->is_in_cardboard()) {
    return;
  }

#ifdef ANDROID_HMD
  if (input_system_->head_mounted_display_input().triggered()) {
    SetLogicalInputs(LogicalInputs_Select, true);
    SetLogicalInputs(LogicalInputs_ThrowPie, true);
  }

  if (character_id_ != kNoCharacter) {
    // The cardboard rotation is relative to the game camera, so translate
    // it back into world space.
    const mat4 camera = game_state_->CameraMatrix();
    const mat4 cardboard_transform =
        input_system_->head_mounted_display_input().head_transform() *
        camera.Inverse();
    const vec3 forward = (cardboard_transform * mathfu::kAxisZ4f).xyz();
    const Angle forward_angle = Angle::FromXZVector(forward);
    // Target the character that is closest to the angle of the view
    Angle smallest_angle;
    CharacterId target = kNoCharacter;
    const CharacterId num_ids =
        static_cast<CharacterId>(game_state_->characters().size());
    for (CharacterId id = 0; id < num_ids; ++id) {
      if (id == character_id_) {
        continue;
      }
      const auto& potential_target = game_state_->characters()[id];
      const vec3 to_target =
          potential_target->position() - game_state_->camera().Position();
      const Angle target_angle = Angle::FromXZVector(to_target);
      const Angle angle_diff = (forward_angle - target_angle).Abs();
      if (target == kNoCharacter || angle_diff < smallest_angle) {
        target = id;
        smallest_angle = angle_diff;
      }
    }

    auto& character = game_state_->characters()[character_id_];
    if (character->target() != target) {
      character->force_target(target);
    }
  }

#endif  // ANDROID_HMD
}

}  // pie_noon
}  // fpl
