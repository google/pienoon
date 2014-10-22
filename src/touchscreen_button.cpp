#include "touchscreen_button.h"
#include "utilities.h"

namespace fpl {
namespace splat {

TouchscreenButton::TouchscreenButton() {}

static bool HandlePointer(const ButtonDef& def, const Pointer& pointer,
                          vec2 window_size) {
  if (pointer.used &&
      pointer.mousepos.x() / window_size.x() >= def.top_left()->x() &&
      pointer.mousepos.y() / window_size.y() >= def.top_left()->y() &&
      pointer.mousepos.x() / window_size.x() <= def.bottom_right()->x() &&
      pointer.mousepos.y() / window_size.y() <= def.bottom_right()->y()) {
    return true;
  }
  return false;
}

void TouchscreenButton::AdvanceFrame(InputSystem* input, vec2 window_size) {
  button_.AdvanceFrame();
  bool down = false;
  for (size_t i = 0; i < input->pointers_.size(); i++) {
    const Pointer& pointer = input->pointers_[i];
    if (pointer.used &&
        input->GetPointerButton(pointer.id).is_down() &&
        HandlePointer(*button_def_, pointer, window_size)) {
      down = true;
      break;
    }
  }
  button_.Update(down);
}

void TouchscreenButton::Render(Renderer& renderer, bool highlight, float time) {
  const vec2 window_size = vec2(renderer.window_size());
  Material* mat = button_.is_down() ? down_material_ : up_material_;

  if (!mat) return;  // This is an invisible button.

  vec2 base_size = LoadVec2(highlight
                      ? button_def_->draw_scale_highlighted()
                      : (button_.is_down()
                         ? button_def_->draw_scale_pressed()
                         : button_def_->draw_scale_normal()));

  auto pulse = sinf(time * 10.0f);
  if (highlight) base_size += mathfu::kOnes2f * pulse * 0.05f;

  vec3 texture_size = vec3(
       mat->textures()[0]->size().x() * base_size.x(),
      -mat->textures()[0]->size().y() * base_size.y(), 0);

  vec3 position = vec3(button_def()->texture_position()->x() * window_size.x(),
                       button_def()->texture_position()->y() * window_size.y(),
                       z_depth());
  shader_->Set(renderer);
  mat->Set(renderer);

  Mesh::RenderAAQuadAlongX(position - (texture_size / 2.0f),
                           position + (texture_size / 2.0f),
                           vec2(0, 1), vec2(1, 0));
}

}  // splat
}  // fpl

