#include "touchscreen_button.h"


namespace fpl {
namespace splat {

TouchscreenButton::TouchscreenButton() {}

bool TouchscreenButton::HandlePointer(Pointer pointer, vec2 window_size) {
  if (pointer.used &&
      pointer.mousepos.x()/window_size.x() >= button_def_->top_left()->x() &&
      pointer.mousepos.y()/window_size.y() >= button_def_->top_left()->y() &&
      pointer.mousepos.x()/window_size.x() <= button_def_->bottom_right()->x() &&
      pointer.mousepos.y()/window_size.y() <= button_def_->bottom_right()->y()) {
    return button_state_ = true;
  }
  return false;
}

void TouchscreenButton::Render(Renderer& renderer) {
  const vec2 window_size = vec2(renderer.window_size());
  Material* mat = button_state_ ? down_material_ : up_material_;

  vec3 texture_size = vec3(mat->textures()[0]->size().x() *
      (button_state_ ? button_def_->draw_scale_pressed()->x() :
                       button_def_->draw_scale_normal()->x()),
      -mat->textures()[0]->size().y() *
      (button_state_ ? button_def_->draw_scale_pressed()->y() :
                       button_def_->draw_scale_normal()->y()), 0);

  vec3 position = vec3(button_def()->texture_position()->x() * window_size.x(),
                       button_def()->texture_position()->y() * window_size.y(),
                       0);
  shader_->Set(renderer);
  mat->Set(renderer);

  Mesh::RenderAAQuadAlongX(position - (texture_size / 2.0f),
                           position + (texture_size / 2.0f),
                           vec2(0, 1), vec2(1, 0));
}

}  // splat
}  // fpl


