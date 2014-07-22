#include "precompiled.h"
#include "input.h"

namespace fpl {

void InputSystem::Initialize() {
  // Set callback to hear about lifecycle events on mobile devices.
  SDL_SetEventFilter(HandleAppEvents, this);
}

int InputSystem::HandleAppEvents(void *userdata, SDL_Event *event) {
  auto renderer = reinterpret_cast<InputSystem *>(userdata);
  switch (event->type) {
    case SDL_APP_TERMINATING:
      return 0;
    case SDL_APP_LOWMEMORY:
      return 0;
    case SDL_APP_WILLENTERBACKGROUND:
      renderer->minimized_ = true;
      return 0;
    case SDL_APP_DIDENTERBACKGROUND:
      return 0;
    case SDL_APP_WILLENTERFOREGROUND:
      return 0;
    case SDL_APP_DIDENTERFOREGROUND:
      renderer->minimized_ = false;
      return 0;
    default:
      return 1;
  }
}

void InputSystem::AdvanceFrame() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
        exit_requested_ = true;
        break;
    }
  }
}

}  // namespace fpl
