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

namespace fpl {
namespace splat {

class Controller;

typedef int CharacterHealth;
typedef int CharacterId;
typedef int WorldTime;

// The current state of the game player. This class tracks information external
// to the state machine, like player health.
class Character {
 public:
  // Creates a character with the given initial values.
  // The Character does not take ownership of the controller or
  // character_state_machine_def pointers.
  Character(CharacterId id, CharacterId target, CharacterHealth health,
            float face_angle, Controller* controller,
            const CharacterStateMachineDef* character_state_machine_def);

  CharacterHealth health() const { return health_; }
  void set_health(CharacterHealth health) { health_ = health; }

  CharacterId id() const { return id_; }

  CharacterId target() const { return target_; }
  void set_target(CharacterId target) { target_ = target; }

  CharacterHealth pie_damage() const { return pie_damage_; }
  void set_pie_damage(CharacterHealth pie_damage) { pie_damage_ = pie_damage; }

  float face_angle() const { return face_angle_; }

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
  float face_angle_;

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
              CharacterHealth damage)
    : source_(source),
      target_(target),
      start_time_(start_time),
      damage_(damage)
  {}

  CharacterId source() const { return source_; }
  CharacterId target() const { return target_; }
  WorldTime start_time() const { return start_time_; }
  CharacterHealth damage() const { return damage_; }
  WorldTime ContactTime() const { return start_time_ + kFlightTime; }

 private:
  // Time between pie being launched and hitting target.
  // TODO: This should be in a configuration file.
  static const WorldTime kFlightTime = 30;

  CharacterId source_;
  CharacterId target_;
  WorldTime start_time_;
  CharacterHealth damage_;
};

}  // splat
}  // fpl

#endif  // SPLAT_CHARACTER_H_

