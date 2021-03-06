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
namespace pie_noon {

using mathfu::vec3;

// Flatten v so that the height component is 0.
static inline const vec3 ZeroHeight(const vec3& v) {
  vec3 zeroed = v;
  zeroed.y() = 0.0f;
  return zeroed;
}

// `ray` comes out from zero and is a unit vector.
static inline float DistFromRay(const vec3& ray, const vec3& point) {
  // Project point onto ray.
  const float projection = vec3::DotProduct(ray, point);
  const float clamped_projection = std::max(0.0f, projection);
  const vec3 closest_point = clamped_projection * ray;
  const float dist = (point - closest_point).Length();
  return dist;
}

TouchscreenController::TouchscreenController()
    : Controller(kTypeTouchScreen),
      input_system_(nullptr),
      game_state_(nullptr),
      deflect_time_remaining_(0) {}

void TouchscreenController::Initialize(fplbase::InputSystem* input_system,
                                       vec2 window_size, const Config* config,
                                       const GameState* game_state) {
  input_system_ = input_system;
  window_size_ = window_size;
  config_ = config;
  game_state_ = game_state;
  ClearAllLogicalInputs();
}

// Convert screen coordinates into a ray eminating from the camera position.
vec3 TouchscreenController::CameraRayFromScreenCoord(
    const vec2i& screen) const {
  // Calculate the up and right offsets, in unit vectors, from the camera's
  // forward vector.
  const float viewport_angle = config_->viewport_angle();
  const float viewport_aspect_ratio = window_size_.x() / window_size_.y();
  const float fov_y_tan = 2.0f * tan(viewport_angle * 0.5f);
  const float fov_x_tan = fov_y_tan * viewport_aspect_ratio;
  const vec2 fov_tan(fov_x_tan, -fov_y_tan);
  const vec2 offset = fov_tan * (vec2(screen) / window_size_ - 0.5f);

  // Generate a unit vector using the camera basis.
  const GameCamera& camera = game_state_->camera();
  const vec3 ray =
      camera.Up() * offset.y() + camera.Side() * offset.x() + camera.Forward();
  return ray.Normalized();
}

// Return the character id that is closest to the `ray` eminating from
// `position`.
CharacterId TouchscreenController::CharacterIdFromRay(
    const vec3& ray, const vec3& position) const {
  const vec3 flat_ray = ZeroHeight(ray).Normalized();

  // Loop through all characters in the character arrangement.
  const CharacterArrangement& arrangement = game_state_->arrangement();
  const CharacterId num_ids =
      static_cast<CharacterId>(arrangement.character_data()->Length());

  // Find the closest one to the ray, in horizontal distance.
  float best_dist = std::numeric_limits<float>::infinity();
  CharacterId best_id = kNoCharacter;
  for (CharacterId id = 0; id < num_ids; ++id) {
    // Calculate horizontal distance from ray to character position.
    const vec3 character_position =
        LoadVec3(arrangement.character_data()->Get(id)->position());
    const vec3 flat_position = ZeroHeight(character_position - position);
    const float dist = DistFromRay(flat_ray, flat_position);

    // Keep the closest id.
    if (dist < best_dist) {
      best_dist = dist;
      best_id = id;
    }
  }

  return best_id;
}

void TouchscreenController::AdvanceFrame(WorldTime delta_time) {
  ClearAllLogicalInputs();

  for (size_t i = 0; i < input_system_->get_pointers().size(); ++i) {
    auto& pointer = input_system_->get_pointers()[i];
    if (!pointer.used) continue;

    // Both deflect and turn-and-throw are triggered by went_down().
    // Deflect is maintained on is_down().
    auto& pointer_button = input_system_->GetPointerButton(pointer.id);
    if (!pointer_button.went_down() && !pointer_button.is_down()) continue;

    // Convert the mouse pointer to a ray in the world, then cast it at
    // each of the characters. If no valid characters are found, something
    // strange is going on, but skip to next pointer.
    const vec3 ray = CameraRayFromScreenCoord(pointer.mousepos);
    const vec3 camera_position = game_state_->camera().Position();
    const CharacterId target_id = CharacterIdFromRay(ray, camera_position);
    // Do nothing if there is no character.
    if (target_id == kNoCharacter) continue;

    // Ignore KO'd characters.
    const int character_state = game_state_->CharacterState(target_id);
    if (character_state == StateId_KO) continue;

    // If we pressed ourself, deflect.
    if (target_id == character_id_) {
      set_target_id(kNoCharacter);

      // Only boost the counter on the press. For holds, we maintain only.
      const WorldTime deflect_time_boost =
          pointer_button.went_down() ? config_->touch_deflect_time() : 1;
      deflect_time_remaining_ =
          std::max(deflect_time_remaining_, deflect_time_boost);
      break;
    }

    // If we're clicking another character, turn to that character.
    // If we're not currently blocking, throw at that character, too.
    if (pointer_button.went_down()) {
      set_target_id(target_id);
      SetLogicalInputs(LogicalInputs_TurnToTarget, true);
      if (deflect_time_remaining_ <= 0) {
        SetLogicalInputs(LogicalInputs_ThrowPie, true);
      }
      break;
    }
  }

  // Deflects hold for a fixed amount of time.
  if (deflect_time_remaining_ > 0) {
    SetLogicalInputs(LogicalInputs_Deflect, true);
    deflect_time_remaining_ -= delta_time;
  }
}

}  // pie_noon
}  // fpl
