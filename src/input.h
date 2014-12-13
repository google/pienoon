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

#include <queue>

#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#ifdef __ANDROID__
// Enable the android gamepad code.  It receives input events from java, via
// JNI, and creates a local representation of the state of any connected
// gamepads.  Also enables the gamepad_controller controller class.
#define ANDROID_GAMEPAD
#include "pthread.h"
#endif

namespace fpl {

using mathfu::vec2;
using mathfu::vec2i;

#ifdef ANDROID_GAMEPAD
typedef int AndroidInputDeviceId;
#endif

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
  JoystickAxis() : value_(0), previous_value_(0) {}
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
  JoystickHat() : value_(mathfu::kZeros2f), previous_value_(mathfu::kZeros2f) {}
  void AdvanceFrame() { previous_value_ = value_; }
  void Update(const vec2 &new_value) {
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
  SDL_Joystick *sdl_joystick() { return sdl_joystick_; }
  void set_sdl_joystick(SDL_Joystick *joy) { sdl_joystick_ = joy; }
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

#ifdef ANDROID_GAMEPAD
// Gamepad input class.  Represents the state of a connected gamepad, based on
// events passed in from java.
class Gamepad {
 public:
  enum GamepadInputButton : int {
    kInvalid = -1,
    kUp = 0,
    kDown,
    kLeft,
    kRight,
    kButtonA,
    kButtonB,
    kButtonC,

    kControlCount
  };

  Gamepad() {
    button_list_.resize(Gamepad::kControlCount);
  }

  void AdvanceFrame();
  Button &GetButton(GamepadInputButton i);

  AndroidInputDeviceId controller_id() { return controller_id_; }
  void set_controller_id(AndroidInputDeviceId controller_id) {
    controller_id_ = controller_id;
  }

  static int GetGamepadCodeFromJavaKeyCode(int java_keycode);

 private:
  AndroidInputDeviceId controller_id_;
  std::vector<Button> button_list_;
};

const float kGamepadHatThreshold = 0.5f;

// Structure used for storing gamepad events when we get them from jni
// until we can deal with them.
struct AndroidInputEvent {
  AndroidInputEvent() {}
  AndroidInputEvent(AndroidInputDeviceId device_id_, int event_code_,
                    int control_code_, float x_, float y_)
    : device_id(device_id_),
      event_code(event_code_),
      control_code(control_code_),
      x(x_),
      y(y_){}
  AndroidInputDeviceId device_id;
  int event_code;
  int control_code;
  float x, y;
};
#endif // ANDROID_GAMEPAD

class InputSystem {
 public:
  InputSystem() : exit_requested_(false), minimized_(false),
      frame_time_(0), last_millis_(0), start_time_(0), frames_(0),
      minimized_frame_(0) {
    pointers_.assign(kMaxSimultanuousPointers, Pointer());
  }

  static const int kMaxSimultanuousPointers = 10;  // All current touch screens.

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

#ifdef ANDROID_GAMEPAD
  // Returns an object describing a gamepad, based on the android device ID.
  // Get the ID either from an android event, or by checking a known gamepad.
  Gamepad &GetGamepad(AndroidInputDeviceId gamepad_device_id);

  const std::map<AndroidInputDeviceId, Gamepad> &GamepadMap() const {
    return gamepad_map_;
  }

  // Receives events from java, and stuffs them into a vector until we're ready.
  static void ReceiveGamepadEvent(int controller_id,
                                  int event_code,
                                  int control_code,
                                  float x, float y);

  // Runs through all the received events and processes them.
  void HandleGamepadEvents();
#endif // ANDROID_GAMEPAD

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

  int minimized_frame() const { return minimized_frame_; }
  int frames() const { return frames_; }

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

#ifdef ANDROID_GAMEPAD
  std::map<AndroidInputDeviceId, Gamepad> gamepad_map_;
  static pthread_mutex_t android_event_mutex;
  static std::queue<AndroidInputEvent> unhandled_java_input_events_;
#endif // ANDROID_GAMEPAD

  // Most recent frame delta, in milliseconds.
  int frame_time_;

  // Time since start, in milliseconds.
  int last_millis_;

  // World time at start, in milliseconds. Set with SDL_GetTicks().
  int start_time_;

  // Number of frames so far. That is, number of times AdvanceFrame() has been
  // called.
  int frames_;

  // Most recent frame at which we were minimized or maximized.
  int minimized_frame_;

 public:
  static const int kMillisecondsPerSecond = 1000;
};

}  // namespace fpl

#endif  // INPUT_SYSTEM_H
