#include "precompiled.h"
#include "character.h"

namespace fpl {
namespace splat {

Character::Character(
    CharacterId id, CharacterId target, CharacterHealth health,
    float face_angle, Controller* controller,
    const CharacterStateMachineDef* character_state_machine_def)
  : id_(id),
    target_(target),
    health_(health),
    pie_damage_(0),
    face_angle_(face_angle),
    controller_(controller),
    state_machine_(character_state_machine_def)
{}

} //  namespace fpl
} //  namespace splat

