#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

namespace fpl {

class InputSystem {
 public:
  void Initialize();
  void AdvanceFrame();

  InputSystem() : exit_requested_(false), minimized_(false) {}

 private:
    static int HandleAppEvents(void *userdata, SDL_Event *event);

 public:
  bool exit_requested_;
  bool minimized_;
};

}  // namespace fpl

#endif  // INPUT_SYSTEM_H

