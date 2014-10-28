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

#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

namespace fpl {

using mathfu::vec2;
using mathfu::vec2i;

// Used to record state for fingers, mousebuttons, keys and gamepad buttons.
// Allows you to know if a button went up/down this frame.
class Button {
 public:
  Button() : is_down_(false) { AdvanceFrame(); }
  void AdvanceFrame() { went_down_ = went_up_ = false; }
  void Update(bool down);

  bool is_down() const { return is_down_; }
  bool went_down() const { return went_down_; }
  bool went_up() const { return went_up_; }

 private:
  bool is_down_;
  bool went_down_;
  bool went_up_;
};

// This enum extends the SDL_Keycode (an int) which represent all keyboard
// keys using positive values. Negative values will represent finger / mouse
// and gamepad buttons.
// `button_map` below maps from one of these values to a Button.
enum {
  SDLK_POINTER1 = -10,  // Left mouse or first finger down.
  SDLK_POINTER2,        // Right mouse or second finger.
  SDLK_POINTER3,        // Middle mouse or third finger.
  SDLK_POINTER4,
  SDLK_POINTER5,
  SDLK_POINTER6,
  SDLK_POINTER7,
  SDLK_POINTER8,
  SDLK_POINTER9,
  SDLK_POINTER10,

  SDLK_PAD_UP = -20,
  SDLK_PAD_DOWN,
  SDLK_PAD_LEFT,
  SDLK_PAD_RIGHT,
  SDLK_PAD_A,
  SDLK_PAD_B
};

// Additional information stored for the pointer buttons.
struct Pointer {
    SDL_FingerID id;
    vec2i mousepos;
    vec2i mousedelta;
    bool used;

    Pointer() : id(0), mousepos(-1), mousedelta(0), used(false) {};
};



// Used to record state for axes
class JoystickAxis {
 public:
  JoystickAxis() : value_(0), previous_value_(0) { AdvanceFrame(); }
  void AdvanceFrame() { previous_value_ = value_; }
  void Update(float new_value) { value_ = new_value; }
  float Value() const { return value_; }
  float PreviousValue() const { return previous_value_; }

 private:
  float value_;  //current value
  float previous_value_;  //value last update
};

// Used to record state for hats
class JoystickHat {
 public:
  JoystickHat() { AdvanceFrame(); }
  void AdvanceFrame() { previous_value_ = value_; }
  void Update(vec2 new_value) {
    value_ = new_value;
  }
  const vec2 &Value() const { return value_; }
  vec2 PreviousValue() const { return previous_value_; }

 private:
  vec2 value_;  //current value
  vec2 previous_value_;  //value last update
};


class Joystick {
 public:

  // Get a Button object for a pointer index.
  Button &GetButton(size_t button_index);
  JoystickAxis &GetAxis(size_t axis_index);
  JoystickHat &GetHat(size_t hat_index);
  void AdvanceFrame();
  SDL_Joystick* sdl_joystick() { return sdl_joystick_; }
  void set_sdl_joystick(SDL_Joystick* joy) { sdl_joystick_ = joy; }
  SDL_JoystickID GetJoystickId() const;
  int GetNumButtons() const;
  int GetNumAxes() const;
  int GetNumHats() const;

 private:
  SDL_Joystick* sdl_joystick_;
  std::vector<JoystickAxis> axis_list_;
  std::vector<Button> button_list_;
  std::vector<JoystickHat> hat_list_;
};


class InputSystem {
 public:
  InputSystem() : exit_requested_(false), minimized_(false),
    frame_time_(0), last_millis_(0), frames_(0), start_time_(0) {
    const int kMaxSimultanuousPointers = 10;  // All current touch screens.
    pointers_.assign(kMaxSimultanuousPointers, Pointer());
  }

  // Initialize the input system. Call this after SDL is initialized by
  // the renderer.
  void Initialize();

  // Call this once a frame to process all new events and update the input
  // state. The window_size argument may get updated whenever the window
  // resizes.
  void AdvanceFrame(vec2i *window_size);

  // Get time in second since the start of the game, or since the last frame.
  float Time() const;
  float DeltaTime() const;

  // Get a Button object describing the current input state (see SDLK_ enum
  // above.
  Button &GetButton(int button);

  // Get a joystick object describing the current input state of the specified
  // joystick ID.  (Contained in every joystick event.)
  Joystick &GetJoystick(SDL_JoystickID joystick_id);

  const std::map<SDL_JoystickID, Joystick> &JoystickMap() const {
    return joystick_map_;
  }

  // Get a Button object for a pointer index.
  Button &GetPointerButton(SDL_FingerID pointer) {
    return GetButton(static_cast<int>(pointer + SDLK_POINTER1));
  }

  void OpenConnectedJoysticks();
  void CloseOpenJoysticks();
  void UpdateConnectedJoystickList();
  void HandleJoystickEvent(SDL_Event event);

  typedef std::function<void(SDL_Event*)> AppEventCallback;
  std::vector<AppEventCallback>& app_event_callbacks() {
    return app_event_callbacks_;
  }
  void AddAppEventCallback(AppEventCallback callback);

 private:
  std::vector<SDL_Joystick*> open_joystick_list;
  static int HandleAppEvents(void *userdata, SDL_Event *event);
  size_t FindPointer(SDL_FingerID id);
  size_t UpdateDragPosition(const SDL_TouchFingerEvent &e, uint32_t event_type,
                            const vec2i &window_size);
  void RemovePointer(size_t i);
  vec2 ConvertHatToVector(uint32_t hat_enum) const;
  std::vector<AppEventCallback> app_event_callbacks_;

 public:
  bool exit_requested_;
  bool minimized_;
  std::vector<Pointer> pointers_;

 private:
  std::map<int, Button> button_map_;
  std::map<SDL_JoystickID, Joystick> joystick_map_;

  // Frame timing related, all in milliseconds.
  // records the most recent frame delta, most recent time since start,
  // number of frames so far, and start time.
  int frame_time_, last_millis_, frames_, start_time_;

 public:
  static const int kMillisecondsPerSecond = 1000;
};

}  // namespace fpl

#endif  // INPUT_SYSTEM_H
