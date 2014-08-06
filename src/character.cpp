#include "precompiled.h"
#include "character.h"

namespace fpl {
namespace splat {

Character::Character(int health,
                     Controller* controller,
                     const CharacterStateMachineDef* character_state_machine_def)
    : health_(health),
      controller_(controller),
      state_machine_(character_state_machine_def) {
}

} //  namespace fpl
} //  namespace splat

