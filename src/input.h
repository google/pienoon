#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

namespace fpl {

using mathfu::vec2;
using mathfu::vec2i;

// Used to record state for fingers, mousebuttons, keys and gamepad buttons.
// Allows you to know if a button went up/down this frame.
class Button {
 public:
  Button() : is_down_(false) { Reset(); }
  void Reset() { went_down_ = went_up_ = false; }
  void Update(bool down);

  bool is_down() { return is_down_; }
  bool went_down() { return went_down_; }
  bool went_up() { return went_up_; }

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

    Pointer() : id(0), mousepos(-1), mousedelta(0) {};
};

class InputSystem {
 public:
  InputSystem() : exit_requested_(false), minimized_(false) {
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

  // Get a Button object describing the current input state (see SDLK_ enum
  // above.
  Button &GetButton(int button);

  // Get a Button object for a pointer index.
  Button &GetPointerButton(int pointer) {
    return GetButton(pointer + SDLK_POINTER1);
  }

 private:
  static int HandleAppEvents(void *userdata, SDL_Event *event);
  size_t FindPointer(SDL_FingerID id);
  size_t UpdateDragPosition(const SDL_TouchFingerEvent &e, uint32_t event_type,
                            const vec2i &window_size);
  void RemovePointer(size_t i);

 public:
  bool exit_requested_;
  bool minimized_;
  std::vector<Pointer> pointers_;

 private:
  std::map<int, Button> button_map_;
};

}  // namespace fpl

#endif  // INPUT_SYSTEM_H

