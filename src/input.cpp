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
#include "input.h"

namespace fpl {

void InputSystem::Initialize() {
  // Set callback to hear about lifecycle events on mobile devices.
  SDL_SetEventFilter(HandleAppEvents, this);

  // Initialize time.
  start_time_ = SDL_GetTicks();
  // Ensure first frame doesn't get a crazy delta.
  last_millis_ = start_time_ - 16;
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

void InputSystem::AdvanceFrame(vec2i *window_size) {
  // Update timing.
  int millis = SDL_GetTicks();
  frame_time_ = millis - last_millis_;
  last_millis_ = millis;
  frames_++;

  // Reset our per-frame input state.
  for (auto it = button_map_.begin(); it != button_map_.end(); ++it) {
    it->second.Reset();
  }
  for (auto it = pointers_.begin(); it != pointers_.end(); ++it) {
    it->mousedelta = vec2i(0);
  }

  // Poll events until Q is empty.
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
        exit_requested_ = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
          GetButton(event.key.keysym.sym).Update(event.key.state==SDL_PRESSED);
          break;
      }
      case SDL_FINGERDOWN: {
        int i = UpdateDragPosition(event.tfinger, event.type, *window_size);
        GetPointerButton(i).Update(true);
        break;
      }
      case SDL_FINGERUP: {
        int i = FindPointer(event.tfinger.fingerId);
        RemovePointer(i);
        GetPointerButton(i).Update(false);
        break;
      }
      case SDL_FINGERMOTION: {
        UpdateDragPosition(event.tfinger, event.type, *window_size);
        break;
      }
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP: {
        GetPointerButton(event.button.button - 1).
          Update(event.button.state != 0);
        pointers_[0].mousepos = vec2i(event.button.x, event.button.y);
        break;
      }
      case SDL_MOUSEMOTION: {
        pointers_[0].mousedelta += vec2i(event.motion.xrel, event.motion.yrel);
        pointers_[0].mousepos = vec2i(event.button.x, event.button.y);
        break;
      }
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED:
            *window_size = vec2i(event.window.data1, event.window.data2);
            break;
        }
        break;
    }
  }
}

float InputSystem::Time() const {
  return (last_millis_ - start_time_) /
      static_cast<float>(kMillisecondsPerSecond);
}

float InputSystem::DeltaTime() const {
  return frame_time_ /
      static_cast<float>(kMillisecondsPerSecond);
}

Button &InputSystem::GetButton(int button) {
  auto it = button_map_.find(button);
  return it != button_map_.end()
    ? it->second
    : (button_map_[button] = Button());
}

void InputSystem::RemovePointer(size_t i) {
  pointers_[i].id = 0;
}

size_t InputSystem::FindPointer(SDL_FingerID id) {
  for (size_t i = 0; i < pointers_.size(); i++) {
    if (pointers_[i].id == id) {
      return i;
    }
  }
  for (size_t i = 0; i < pointers_.size(); i++) {
    if (!pointers_[i].id) {
      pointers_[i].id = id;
      return i;
    }
  }
  assert(0);
  return 0;
}

size_t InputSystem::UpdateDragPosition(const SDL_TouchFingerEvent &e,
                                    uint32_t event_type,
                                    const vec2i &window_size) {
  // This is a bit clumsy as SDL has a list of pointers and so do we,
  // but they work a bit differently: ours is such that the first one is
  // always the first one that went down, making it easier to write code
  // that works well for both mouse and touch.
  int numfingers = SDL_GetNumTouchFingers(e.touchId);
  for (int i = 0; i < numfingers; i++) {
    auto finger = SDL_GetTouchFinger(e.touchId, i);
    if (finger->id == e.fingerId) {
      auto j = FindPointer(e.fingerId);
      if (event_type == SDL_FINGERUP) RemovePointer(j);
      auto &p = pointers_[j];
      auto event_position = vec2(e.x, e.y);
      auto event_delta = vec2(e.dx, e.dy);
      p.mousepos = vec2i(event_position * vec2(window_size));
      p.mousedelta += vec2i(event_delta * vec2(window_size));
      return j;
    }
  }
  return 0;
}

void Button::Update(bool down) {
  is_down_ = down;
  if (down) went_down_ = true;
  else went_up_ = true;
}


}  // namespace fpl
