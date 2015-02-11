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

#include "imgui.h"

namespace fpl {
namespace gui {

enum Alignment {
  ALIGN_TOPLEFT,
  ALIGN_CENTER,
  ALIGN_BOTTOMRIGHT,
};

bool IsVertical(Layout layout) {
  return layout >= LAYOUT_VERTICAL_LEFT;
}

Alignment GetAlignment(Layout layout) {
  return static_cast<Alignment>(layout - (IsVertical(layout)
             ? LAYOUT_VERTICAL_LEFT - LAYOUT_HORIZONTAL_TOP
             : 0));
}

// This holds the transient state of a group while its layout is being
// calculated / rendered.
class Group {
 public:
  Group(bool _vertical, Alignment _align, int _spacing, size_t _element_idx)
    : vertical_(_vertical), align_(_align), spacing_(_spacing),
      size_(mathfu::kZeros2i), position_(mathfu::kZeros2i),
      element_idx_(_element_idx), margin_(mathfu::kZeros4i) {}

  // Extend this group with the size of a new element, and possibly spacing
  // if it wasn't the first element.
  void Extend(const vec2i &extension) {
    size_ = vertical_
      ? vec2i(std::max(size_.x(), extension.x()),
              size_.y() + extension.y() + (size_.y() ? spacing_ : 0))
      : vec2i(size_.x() + extension.x() + (size_.x() ? spacing_ : 0),
              std::max(size_.y(), extension.y()));
  }

  bool vertical_;
  Alignment align_;
  int spacing_;
  vec2i size_;
  vec2i position_;
  size_t element_idx_;
  vec4i margin_;
};

// This holds transient state used while a GUI is being laid out / rendered.
// It is intentionally hidden from the interface.
// It is implemented as a singleton that the GUI element functions can access.

class InternalState;
InternalState *state = nullptr;

class InternalState : public Group {
 public:
  struct Element {
    Element(const vec2i &_size, const char *_id)
      : size(_size), id(_id), interactive(false) {}

    vec2i size;        // Minimum size computed by layout pass.
    const char *id;    // Id specified by the user.
    bool interactive;  // Wants to respond to user input.
  };

  InternalState(MaterialManager &matman, FontManager &fontman,
                InputSystem &input)
      : Group(true, ALIGN_TOPLEFT, 0, 0),
        layout_pass_(true), canvas_size_(matman.renderer().window_size()),
        virtual_resolution_(IMGUI_DEFAULT_VIRTUAL_RESOLUTION),
        matman_(matman), input_(input),
        fontman_(fontman), pointer_max_active_index_(-1),
        gamepad_has_focus_element(false),
        gamepad_event(EVENT_HOVER) {
    SetScale();

    // Cache the state of multiple pointers, so we have to do less work per
    // interactive element.
    pointer_max_active_index_ = 0;  // Mouse is always active.
    // TODO: pointer_max_active_index_ should start at -1 if on a touchscreen.
    for (int i = 0; i < InputSystem::kMaxSimultanuousPointers; i++) {
      pointer_buttons_[i] = &input.GetPointerButton(i);
      if(pointer_buttons_[i]->is_down() ||
         pointer_buttons_[i]->went_down() ||
         pointer_buttons_[i]->went_up()) {
        pointer_max_active_index_ = std::max(pointer_max_active_index_, i);
      }
    }

    // If this assert hits, you likely are trying to created nested GUIs.
    assert(!state);

    state = this;

    // Load shaders ahead.
    image_shader_ = matman_.LoadShader("shaders/textured");
    assert(image_shader_);
    font_shader_ = matman_.LoadShader("shaders/font");
    assert(font_shader_);
    color_shader_ = matman_.LoadShader("shaders/color");
    assert(color_shader_);

    text_color_ = mathfu::kOnes4f;

    fontman_.StartLayoutPass();
  }

  ~InternalState() {
    state = nullptr;
  }

  bool EqualId(const char *id1, const char *id2) {
    // We can do pointer compare, because we receive these ids from the user
    // and then store them.
    // We require the user to provide storage for the id as long as the GUI
    // is active, which guarantees pointer identity.
    // TODO: we either need to provide a way to clear persistent ids once a
    // GUI goes away entirely, or tell users not ever pass a c_str().
    // Better yet, replace this by hashes, as that makes generating ids in
    // loops easier. Bit expensive though.
    return id1 == id2;
  }

  template<int D> mathfu::Vector<int, D> VirtualToPhysical(
                                           const mathfu::Vector<float, D> &v) {
    return mathfu::Vector<int, D>(v * pixel_scale_ + 0.5f);
  }

  // Initialize the scaling factor for the virtual resolution.
  void SetScale() {
    auto scale = vec2(matman_.renderer().window_size()) / virtual_resolution_;
    pixel_scale_ = std::min(scale.x(), scale.y());
  }

  // Compute a space offset for a particular alignment for just the x or y
  // dimension.
  static vec2i AlignDimension(Alignment align, int dim, const vec2i &space) {
    vec2i dest(0, 0);
    switch (align) {
      case ALIGN_TOPLEFT:
        break;
      case ALIGN_CENTER:
        dest[dim] += space[dim] / 2;
        break;
      case ALIGN_BOTTOMRIGHT:
        dest[dim] += space[dim];
        break;
    }
    return dest;
  }

  // Determines placement for the UI as a whole inside the available space
  // (screen).
  void PositionUI(const vec2i &canvas_size, float virtual_resolution,
                  Alignment horizontal, Alignment vertical) {
    if (layout_pass_) {
      canvas_size_ = canvas_size;
      virtual_resolution_ = virtual_resolution;
      SetScale();
    } else {
      auto space = canvas_size_ - size_;
      position_ += AlignDimension(horizontal, 0, space) +
                   AlignDimension(vertical,   1, space);
    }
  }

  // Switch from the layout pass to the render/event pass.
  void StartRenderPass() {
    // If you hit this assert, you are missing an EndGroup().
    assert(!group_stack_.size());

    // Update font manager if they need to upload font atlas texture.
    fontman_.StartRenderPass();

    size_ = elements_[0].size;

    layout_pass_ = false;
    element_it_ = elements_.begin();

    CheckGamePadNavigation();
  }

  // (render pass): retrieve the next corresponding cached element we
  // created in the layout pass. This is slightly more tricky than a straight
  // lookup because event handlers may insert/remove elements.
  const Element *NextElement(const char *id) {
    auto backup = element_it_;
    while (element_it_ != elements_.end()) {
      // This loop usually returns on the first iteration, the only time it
      // doesn't is if an event handler caused an element to removed.
      auto &element = *element_it_;
      ++element_it_;
      if (EqualId(element.id, id)) return &element;
    }
    // Didn't find this id at all, which means an event handler just caused
    // this element to be added, so we skip it.
    element_it_ = backup;
    return nullptr;
  }

  // (layout pass): create a new element.
  void NewElement(const vec2i &size, const char *id) {
    elements_.push_back(Element(size, id));
  }

  // (render pass): move the group's current position past an element of
  // the given size.
  void Advance(const vec2i &size) {
    position_ += vertical_
      ? vec2i(0, size.y() + spacing_)
      : vec2i(size.x() + spacing_, 0);
  }

  // (render pass): return the position of the current element, as a function
  // of the group's current position and the alignment.
  vec2i Position(const Element &element) {
    return position_ + margin_.xy() +
           AlignDimension(align_, vertical_ ? 0 : 1,
                          size_ - element.size - margin_.xy() - margin_.zw());
  }

  void RenderQuad(Shader *sh, const vec4 &color, const vec2i &pos,
                  const vec2i &size) {
    auto &renderer = matman_.renderer();
    renderer.color() = color;
    sh->Set(renderer);
    Mesh::RenderAAQuadAlongX(vec3(vec2(pos), 0),
                             vec3(vec2(pos + size), 0));
  }

  void RenderQuad(Shader *sh, const vec4 &color, const vec2i &pos,
                  const vec2i &size, const vec4 &uv) {
    auto &renderer = matman_.renderer();
    renderer.color() = color;
    sh->Set(renderer);
    Mesh::RenderAAQuadAlongX(vec3(vec2(pos), 0),
                             vec3(vec2(pos + size), 0),
                             uv.xy(), uv.zw());
  }

  // An image element.
  void Image(const char *texture_name, float ysize) {
    auto tex = matman_.FindTexture(texture_name);
    assert(tex);  // You need to have called LoadTexture before.
    if (layout_pass_) {
      auto virtual_image_size =
          vec2(tex->size().x() * ysize / tex->size().y(), ysize);
      // Map the size to real screen pixels, rounding to the nearest int
      // for pixel-aligned rendering.
      auto size = VirtualToPhysical(virtual_image_size);
      NewElement(size, texture_name);
      Extend(size);
    } else {
      auto element = NextElement(texture_name);
      if (element) {
        auto position = Position(*element);
        tex->Set(0);
        RenderQuad(image_shader_, mathfu::kOnes4f, position, element->size);
        Advance(element->size);
      }
    }
  }
  // Text label.
  void Label(const char *text, float ysize)
  {
    // Set text color.
    matman_.renderer().color() = text_color_;

#ifdef USE_GLYPHCACHE
    auto size = VirtualToPhysical(vec2(0, ysize));
    auto buffer = fontman_.GetBuffer(text, size.y());
    if (layout_pass_) {
      if (buffer == nullptr) {
        // Upload a texture & flush glyph cache
        fontman_.FlushAndUpdate();

        // Try to create buffer again.
        buffer = fontman_.GetBuffer(text, size.y());
        if (buffer == nullptr) {
          SDL_LogError(SDL_LOG_CATEGORY_ERROR, "The given text '%s' with ",
                       "size:%d does not fit a glyph cache. Try to "
                       "increase a cache size or use GetTexture() API ",
                       "instead.\n", text, size.y());
        }
      }
      NewElement(buffer->get_size(), text);
      Extend(buffer->get_size());
    } else {
      // Check if texture atlas needs to be updated.
      if (buffer->get_pass() > 0) {
        fontman_.StartRenderPass();
      }

      auto element = NextElement(text);
      if (element) {
        auto position = Position(*element);
        fontman_.GetAtlasTexture()->Set(0);

        font_shader_->Set(matman_.renderer());
        font_shader_->SetUniform("pos_offset",
                                 vec3(position.x(), position.y(),
                                      0.f));

        const Attribute kFormat[] = { kPosition3f, kTexCoord2f, kEND };
        Mesh::RenderArray(GL_TRIANGLES, buffer->get_indices()->size(),
                          kFormat,
                          sizeof(FontVertex),
                          reinterpret_cast<const char *>
                          (buffer->get_vertices()->data()),
                          buffer->get_indices()->data());

        Advance(element->size);
      }
    }
#else
    font_shader_->SetUniform("pos_offset", vec3(0.0f, 0.0f, 0.f));

    auto size = VirtualToPhysical(vec2(0, ysize));
    auto tex = fontman_.GetTexture(text, size.y());
    auto uv = tex->uv();
    auto scale = static_cast<float>(size.y()) /
                 static_cast<float>(tex->metrics().ascender()
                                    - tex->metrics().descender());
    if (layout_pass_) {
      auto image_size = vec2i(tex->size().x() * (uv.z() - uv.x()) * scale,
                              size.y());
      NewElement(image_size, text);
      Extend(image_size);
    } else {
      auto element = NextElement(text);
      if (element) {
        auto position = Position(*element);
        tex->Set(0);
        // Note that some glyphs may render outside of element boundary.
        vec2i pos = position -
                    vec2i(0, tex->metrics().internal_leading() * scale);
        vec2i size = vec2i(element->size) +
                     vec2i(0, (tex->metrics().internal_leading() -
                     tex->metrics().external_leading()) * scale);
        RenderQuad(font_shader_, mathfu::kOnes4f, pos, size, uv);
        Advance(element->size);
      }
    }
#endif
  }

  // An element that has sub-elements. Tracks its state in an instance of
  // Layout, that is pushed/popped from the stack as needed.
  void StartGroup(bool vertical, Alignment align, int spacing, const char *id) {
    Group layout(vertical, align, spacing, elements_.size());
    group_stack_.push_back(*this);
    if (layout_pass_) {
      NewElement(mathfu::kZeros2i, id);
    } else {
      auto element = NextElement(id);
      if (element) {
        layout.position_ = Position(*element);
        layout.size_ = element->size;
        // Make layout refer to element it originates from, iterator points
        // to next element after the current one.
        layout.element_idx_ = element_it_ - elements_.begin() - 1;
      }
    }
    *static_cast<Group *>(this) = layout;
  }

  // Clean up the Group element started by StartGroup()
  void EndGroup() {
    // If you hit this assert, you have one too many EndGroup().
    assert(group_stack_.size());

    auto size = size_;
    auto margin = margin_.xy() + margin_.zw();
    auto element_idx = element_idx_;
    *static_cast<Group *>(this) = group_stack_.back();
    group_stack_.pop_back();
    if (layout_pass_) {
      size += margin;
      // Contribute the size of this group to its parent.
      Extend(size);
      // Set the size of this group as the size of the element tracking it.
      elements_[element_idx].size = size;
    } else {
      Advance(size);
    }
  }

  void SetMargin(const Margin &margin) {
    margin_ = VirtualToPhysical(margin.borders);
  }

  void RecordId(const char *id, int i) { persistent_.pointer_element[i] = id; }
  bool SameId(const char *id, int i) {
    return EqualId(id, persistent_.pointer_element[i]);
  }

  Event CheckEvent() {
    auto &element = elements_[element_idx_];
    if (layout_pass_) {
      element.interactive = true;
    } else {
      // We only fire events during the second pass.
      auto id = element.id;
      // pointer_max_active_index_ is typically 0, so loop not expensive.
      for (int i = 0; i <= pointer_max_active_index_; i++) {
        if (mathfu::InRange2D(input_.pointers_[i].mousepos, position_,
                              position_ + size_)) {
          auto &button = *pointer_buttons_[i];
          int event = 0;

          if (button.went_down()) {
            RecordId(id, i);
            event |= EVENT_WENT_DOWN;
          }
          if (button.went_up() && SameId(id, i)) {
            event |= EVENT_WENT_UP;
          }
          else if (button.is_down() && SameId(id, i)) {
            event |= EVENT_IS_DOWN;
            // Record the last element we received an up on, as the target
            // for keyboard input.
            persistent_.keyboard_focus = id;
          }
          if (!event) event = EVENT_HOVER;
          // We only report an event for the first finger to touch an element.
          // This is intentional.
          return static_cast<Event>(event);
        }
      }
      // Generate hover events for the current element the gamepad is focused
      // on.
      if (EqualId(persistent_.gamepad_focus, id)) {
        gamepad_has_focus_element = true;
        return gamepad_event;
      }
    }
    return EVENT_NONE;
  }

  void CheckGamePadFocus() {
    if (!gamepad_has_focus_element)
      // This may happen when a GUI first appears or when elements get removed.
      // TODO: only do this when there's an actual gamepad connected.
      persistent_.gamepad_focus = NextInteractiveElement(-1, 1);
  }

  void CheckGamePadNavigation() {
    int dir = 0;
    // FIXME: this should work on other platforms too.
#   ifdef ANDROID_GAMEPAD
    auto &gamepads = input_.GamepadMap();
    for (auto &gamepad : gamepads) {
      dir = CheckButtons(gamepad.second.GetButton(Gamepad::kLeft),
                         gamepad.second.GetButton(Gamepad::kRight),
                         gamepad.second.GetButton(Gamepad::kButtonA));
    }
#   endif
    // For testing, also support keyboard:
    dir = CheckButtons(input_.GetButton(SDLK_LEFT),
                       input_.GetButton(SDLK_RIGHT),
                       input_.GetButton(SDLK_RETURN));
    // Now find the current element, and move to the next.
    if (dir) {
      for (auto &e : elements_) {
        if (EqualId(e.id, persistent_.gamepad_focus)) {
          persistent_.gamepad_focus =
            NextInteractiveElement(&e - &elements_[0], dir);
          break;
        }
      }
    }
  }

  int CheckButtons(const Button &left, const Button &right,
                   const Button &action) {
    int dir = 0;
    if (left.went_up()) dir = -1;
    if (right.went_up()) dir = 1;
    if (action.went_up()) gamepad_event = EVENT_WENT_UP;
    if (action.went_down()) gamepad_event = EVENT_WENT_DOWN;
    if (action.is_down()) gamepad_event = EVENT_IS_DOWN;
    return dir;
  }

  const char *NextInteractiveElement(int start, int direction) {
    auto range = static_cast<int>(elements_.size());
    for (auto i = start; ; ) {
      i += direction;
      // Wrap around.. just once.
      if (i < 0) i = range - 1;
      else if (i >= range) i = -1;
      // Back where we started, either there's no interactive elements, or
      // the vector is empty.
      if (i == start) return dummy_id();
      if (elements_[i].interactive) return elements_[i].id;
    }
  }

  void ColorBackground(const vec4 &color) {
    RenderQuad(color_shader_, color, position_, size_);
  }

  // Set Label's text color.
  void SetTextColor(const vec4 &color) {
    text_color_ = color;
  }

  static const char *dummy_id() { return "__null_id__"; }

  bool layout_pass_;
  std::vector<Element> elements_;
  std::vector<Element>::const_iterator element_it_;
  std::vector<Group> group_stack_;
  vec2i canvas_size_;
  float virtual_resolution_;
  float pixel_scale_;

  MaterialManager &matman_;
  InputSystem &input_;
  FontManager &fontman_;
  Shader *image_shader_;
  Shader *font_shader_;
  Shader *color_shader_;

  // Widget properties.
  mathfu::vec4 text_color_;

  int pointer_max_active_index_;
  const Button *pointer_buttons_[InputSystem::kMaxSimultanuousPointers];
  bool gamepad_has_focus_element;
  Event gamepad_event;

  // Intra-frame persistent state.
  static struct PersistentState {
    PersistentState() {
      // This is effectively a global, so no memory allocation or other
      // complex initialization here.
      for (int i = 0; i < InputSystem::kMaxSimultanuousPointers; i++) {
        pointer_element[i] = dummy_id();
      }
      gamepad_focus = keyboard_focus = dummy_id();
    }

    // For each pointer, the element id that last received a down event.
    const char *pointer_element[InputSystem::kMaxSimultanuousPointers];
    // The element the gamepad is currently "over", simulates the mouse
    // hovering over an element.
    const char *gamepad_focus;
    // The element that last received an up event. Keystrokes should be
    // directed to this element, e.g. for a text edit widget
    const char *keyboard_focus;
  } persistent_;
};

InternalState::PersistentState InternalState::persistent_;

void Run(MaterialManager &matman, FontManager &fontman, InputSystem &input,
         const std::function<void ()> &gui_definition) {

  // Create our new temporary state.
  InternalState internal_state(matman, fontman, input);

  // Run two passes, one for layout, one for rendering.
  // First pass:
  gui_definition();

  // Second pass:
  internal_state.StartRenderPass();

  // Set up an ortho camera for all 2D elements, with (0, 0) in the top left,
  // and the bottom right the windows size in pixels.
  auto &renderer = matman.renderer();
  auto res = renderer.window_size();
  auto ortho_mat = mathfu::OrthoHelper<float>(
      0.0f, static_cast<float>(res.x()), static_cast<float>(res.y()), 0.0f,
      -1.0f, 1.0f);
  renderer.model_view_projection() = ortho_mat;

  renderer.SetBlendMode(kBlendModeAlpha);
  renderer.DepthTest(false);

  gui_definition();

  internal_state.CheckGamePadFocus();
}

InternalState *Gui() { assert(state); return state; }

void Image(const char *texture_name, float size)
{
  Gui()->Image(texture_name, size);
}

void Label(const char *text, float size)
{
  Gui()->Label(text, size);
}


void StartGroup(Layout layout, int spacing, const char *id) {
  Gui()->StartGroup(IsVertical(layout), GetAlignment(layout), spacing, id);
}

void EndGroup() {
  Gui()->EndGroup();
}

void SetMargin(const Margin &margin) { Gui()->SetMargin(margin); }

void SetTextColor(const mathfu::vec4 &color) { Gui()->SetTextColor(color); }

Event CheckEvent() { return Gui()->CheckEvent(); }

void ColorBackground(const vec4 &color) { Gui()->ColorBackground(color); }

void PositionUI(const vec2i &canvas_size, float virtual_resolution,
                Layout horizontal, Layout vertical) {
  Gui()->PositionUI(canvas_size, virtual_resolution, GetAlignment(horizontal),
                    GetAlignment(vertical));
}

// Example how to create a button. We will provide convenient pre-made
// buttons like this, but it is expected many games will make custom buttons.
Event ImageButton(const char *texture_name, float size, const char *id) {
  StartGroup(LAYOUT_VERTICAL_LEFT, size, id);
    SetMargin(Margin(10));
    auto event = CheckEvent();
    if (event & EVENT_IS_DOWN) ColorBackground(vec4(1.0f, 1.0f, 1.0f, 0.5f));
    else if (event & EVENT_HOVER) ColorBackground(vec4(0.5f, 0.5f, 0.5f, 0.5f));
    Image(texture_name, size);
  EndGroup();
  return event;
}

void TestGUI(MaterialManager &matman, FontManager &fontman,
             InputSystem &input) {
  static float f = 0.0f;
  f += 0.04f;

  Run(matman, fontman, input, [&matman]() {
    PositionUI(matman.renderer().window_size(), 1000, LAYOUT_HORIZONTAL_CENTER,
               LAYOUT_VERTICAL_RIGHT);
    StartGroup(LAYOUT_HORIZONTAL_TOP, 10);
      StartGroup(LAYOUT_VERTICAL_LEFT, 20);
        if (ImageButton("textures/text_about.webp", 50, "my_id") ==
            EVENT_WENT_UP)
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "You clicked!");
        StartGroup(LAYOUT_HORIZONTAL_TOP, 0);
          Label("Property T", 30);
          SetTextColor(mathfu::vec4(1.0f, 0.0f, 0.0f, 1.0f));
          Label("Test ", 30);
          SetTextColor(mathfu::kOnes4f);
          Label("ffWAWÄテスト", 30);
        EndGroup();
        Label("The quick brown fox jumps over the lazy dog", 32);
        Label("The quick brown fox jumps over the lazy dog", 24);
        Label("The quick brown fox jumps over the lazy dog", 20);
      EndGroup();
      StartGroup(LAYOUT_VERTICAL_CENTER, 40);
        if (ImageButton("textures/text_about.webp", 50, "my_id2") ==
            EVENT_WENT_UP)
          SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "You clicked 2!");
        Image("textures/text_about.webp", 40);
        Image("textures/text_about.webp", 30);
      EndGroup();
      StartGroup(LAYOUT_VERTICAL_RIGHT, 0);
        SetMargin(Margin(100));
        Image("textures/text_about.webp", 50);
        Image("textures/text_about.webp", 40);
        Image("textures/text_about.webp", 30);
      EndGroup();
    EndGroup();
  });
}

}  // namespace gui
}  // namespace fpl
