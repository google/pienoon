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


#ifndef PARTICLES_H
#define PARTICLES_H

#include "common.h"
#include "scene_description.h"
#include <list>

namespace fpl {
namespace splat {


class Particle {
 public:
  Particle() { reset(); }

  void reset();

  mathfu::vec3 position() const { return position_; }
  void set_position(mathfu::vec3 position) { position_ = position; }

  mathfu::vec3 velocity() const { return velocity_; }
  void set_velocity(mathfu::vec3 velocity) { velocity_ = velocity; }

  mathfu::vec3 acceleration() const { return acceleration_; }
  void set_acceleration(mathfu::vec3 acceleration) { acceleration_ = acceleration; }

  Quat orientation() const { return orientation_; }
  void set_orientation(Quat orientation) { orientation_ = orientation; }

  Quat rotational_velocity() const { return rotational_velocity_; }
  void set_rotational_velocity(Quat rotational_velocity) {
    rotational_velocity_ = rotational_velocity;
  }

  mathfu::vec4 base_tint() const { return base_tint_; }
  void set_base_tint(mathfu::vec4 base_tint) { base_tint_ = base_tint; }

  mathfu::vec3 base_scale() const { return base_scale_; }
  void set_base_scale(mathfu::vec3 base_scale) { base_scale_ = base_scale; }

  uint16_t duration_remaining() const { return duration_remaining_; }
  void set_duration_remaining(uint16_t duration_remaining) {
    duration_remaining_ = duration_remaining;
  }

  uint16_t duration_of_fade_out() const { return duration_of_fade_out_; }
  void set_duration_of_fade_out(uint16_t duration_of_fade_out) {
    duration_of_fade_out_ = duration_of_fade_out;
  }

  uint16_t duration_of_shrink_out() const { return duration_of_shrink_out_; }
  void set_duration_of_shrink_out(uint16_t duration_of_shrink_out) {
    duration_of_shrink_out_ = duration_of_shrink_out;
  }

  uint16_t renderable_id() const { return renderable_id_; }
  void set_renderable_id(uint16_t renderable_id) {
    renderable_id_ = renderable_id;
  }

  // Generate the matrix we'll need to draw it:
  mathfu::mat4 CalculateMatrix() const;

  // Returns the current tint, after taking particle effects into account.
  mathfu::vec4 CalculateTint() const;
  // Returns the current scale, after taking particle effects into account.
  mathfu::vec3 CalculateScale() const;

 private:
  mathfu::vec3 position_;
  mathfu::vec3 velocity_;
  mathfu::vec3 acceleration_;
  Quat orientation_;
  Quat rotational_velocity_;
  mathfu::vec3 base_scale_;
  mathfu::vec4 base_tint_;

  // How much longer the particle will last, in milliseconds.
  uint16_t duration_remaining_;

  // How long it will take the particle to fade or shrink away, when it reaches
  // the end of its life span.  (In milliseconds)
  uint16_t duration_of_fade_out_;
  uint16_t duration_of_shrink_out_;

  // the renderable ID we should use when drawing this particle.
  uint16_t renderable_id_;
};

class ParticleManager {
 public:
  void AdvanceFrame(WorldTime delta_time);

  const std::list<Particle*>& get_particle_list() const {
    return particle_list_;
  }

  // Returns a pointer to a new particle, ready to be populated.
  // Note that this pointer is not guaranteed to be valid indefinitely!
  // Also note that the initial state of the particle is undetermined, so
  // users should either populate every field explicitly, or call
  // Particle::reset().
  Particle *CreateParticle();

 private:
  void DeactivateParticle();

  bool AdvanceParticle(Particle *p, WorldTime delta_time);

  std::list<Particle*> particle_list_;
  std::list<Particle*> inactive_particle_list_;
};

}  // splat
}  // fpl
#endif // PARTICLES_H
