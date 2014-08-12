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

#ifndef SPLAT_CHARACTER_H_
#define SPLAT_CHARACTER_H_

#include "character_state_machine.h"
#include "sdl_controller.h"
#include "angle.h"

namespace fpl {
namespace splat {

class Controller;

typedef int CharacterHealth;
typedef int CharacterId;
typedef int WorldTime;

// The current state of the character. This class tracks information external
// to the state machine, like health.
class Character {
 public:
  // Creates a character with the given initial values.
  // The Character does not take ownership of the controller or
  // character_state_machine_def pointers.
  Character(CharacterId id, CharacterId target, CharacterHealth health,
            Angle face_angle, const mathfu::vec3& position,
            Controller* controller,
            const CharacterStateMachineDef* character_state_machine_def);

  // Convert the position and face angle into a matrix for rendering.
  mathfu::mat4 CalculateMatrix() const;

  // Calculate the renderable id for the character at 'anim_time'.
  uint16_t RenderableId(WorldTime anim_time) const;

  CharacterHealth health() const { return health_; }
  void set_health(CharacterHealth health) { health_ = health; }

  CharacterId id() const { return id_; }

  CharacterId target() const { return target_; }
  void set_target(CharacterId target) { target_ = target; }

  CharacterHealth pie_damage() const { return pie_damage_; }
  void set_pie_damage(CharacterHealth pie_damage) { pie_damage_ = pie_damage; }

  Angle face_angle() const { return face_angle_; }
  void set_face_angle(const Angle& angle) { face_angle_ = angle; }
  float face_angle_velocity() const { return face_angle_velocity_; }
  void set_face_angle_velocity(float vel) { face_angle_velocity_ = vel; }
  mathfu::vec3 position() const { return position_; }
  void set_position(const mathfu::vec3& position) { position_ = position; }

  const Controller* controller() const { return controller_; }
  Controller* controller() { return controller_; }

  const CharacterStateMachine* state_machine() const { return &state_machine_; }
  CharacterStateMachine* state_machine() { return &state_machine_; }

 private:
  // Our own id.
  CharacterId id_;

  // Character that we are currently aiming the pie at.
  CharacterId target_;

  // The character's current health. They're out when their health reaches 0.
  CharacterHealth health_;

  // Size of the character's pie. Does this much damage on contact.
  // If unloaded, 0.
  CharacterHealth pie_damage_;

  // World angle. Will eventually settle on the angle towards target_.
  Angle face_angle_;

  // Rate at which face_angle_ is changing. Acceleration changes instantly, but
  // face angle has some momentum.
  float face_angle_velocity_;

  // Position of the character in world space.
  mathfu::vec3 position_;

  // The controller used to translate inputs into game actions.
  Controller* controller_;

  // The current state of the character.
  CharacterStateMachine state_machine_;
};


class AirbornePie {
 public:
  AirbornePie(CharacterId source, CharacterId target, WorldTime start_time,
              WorldTime flight_time, CharacterHealth damage, float height);

  CharacterId source() const { return source_; }
  CharacterId target() const { return target_; }
  WorldTime start_time() const { return start_time_; }
  WorldTime flight_time() const { return flight_time_; }
  CharacterHealth damage() const { return damage_; }
  float height() const { return height_; }
  Quat orientation() const { return orientation_; }
  void set_orientation(const Quat& orientation) { orientation_ = orientation; }
  mathfu::vec3 position() const { return position_; }
  void set_position(const mathfu::vec3& position) { position_ = position; }
  mathfu::mat4 CalculateMatrix() const;

 private:
  CharacterId source_;
  CharacterId target_;
  WorldTime start_time_;
  WorldTime flight_time_;
  float height_;
  CharacterHealth damage_;
  Quat orientation_;
  mathfu::vec3 position_;
};


// Return index of first item with time >= t.
// T is a flatbuffer::Vector; one of the Timeline members.
template<class T>
inline int TimelineIndexAfterTime(
    const T& arr, const int start_index, const WorldTime t) {
  if (!arr)
    return 0;

  for (int i = start_index; i < static_cast<int>(arr->Length()); ++i) {
    if (arr->Get(i)->time() >= t)
      return i;
  }
  return arr->Length();
}

// Return index of last item with time <= t.
// T is a flatbuffer::Vector; one of the Timeline members.
template<class T>
inline int TimelineIndexBeforeTime(const T& arr, const WorldTime t) {
  if (!arr || arr->Length() == 0)
    return 0;

  for (int i = 1; i < static_cast<int>(arr->Length()); ++i) {
    if (arr->Get(i)->time() > t)
      return i - 1;
  }
  return arr->Length() - 1;
}


}  // splat
}  // fpl

#endif  // SPLAT_CHARACTER_H_

