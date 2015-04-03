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

#ifndef GUI_MENU_H
#define GUI_MENU_H

#include <queue>
#include "common.h"
#include "config_generated.h"
#include "controller.h"
#include "input.h"
#include "material_manager.h"
#include "renderer.h"
#include "touchscreen_button.h"
#include "imgui.h"

namespace fpl {
namespace pie_noon {

class StaticImage;

// Simple struct for transporting a menu selection, and the controller that
// triggered it.
struct MenuSelection {
  MenuSelection(ButtonId buttonId, ControllerId controllerId)
      : button_id(buttonId), controller_id(controllerId) {}

  ButtonId button_id;
  ControllerId controller_id;
};

class GuiMenu {
 public:
  GuiMenu();

  void AdvanceFrame(WorldTime delta_time, InputSystem* input,
                    const vec2& window_size);
  void Setup(const UiGroup* menudef, MaterialManager* matman);
  void LoadAssets(const UiGroup* menu_def, MaterialManager* matman);
  void Render(Renderer* renderer);
  void AdvanceFrame(WorldTime delta_time);
  MenuSelection GetRecentSelection();
  void HandleControllerInput(uint32_t logical_input,
                             ControllerId controller_id);
  ButtonId GetFocus() const;
  void SetFocus(ButtonId new_focus);
  TouchscreenButton* FindButtonById(ButtonId id);
  StaticImage* FindImageById(ButtonId id);
  const UiGroup* menu_def() const { return menu_def_; }

 private:
  void ClearRecentSelections();
  void UpdateFocus(const flatbuffers::Vector<uint16_t>* destination_list);

  // imgui custom button definition.
  gui::Event ImguiButton(const ImguiButtonDef& data);
  void RenderTexture(const Texture& tex, const vec2& pos,
                     const vec2& size, const vec2& scale);

  const UiGroup* menu_def_;
  InputSystem* input_;
  MaterialManager* matman_;
  FontManager* fontman_;

  ButtonId current_focus_;
  std::queue<MenuSelection> unhandled_selections_;
  std::vector<TouchscreenButton> button_list_;
  std::vector<StaticImage> image_list_;

  // Total Worldtime since the menu was initialized.
  // Used for animating selections and such.
  WorldTime time_elapsed_;
};

}  // pie_noon
}  // fpl
#endif  // GUI_MENU_H
