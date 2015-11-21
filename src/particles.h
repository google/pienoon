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

#include <list>
#include "common.h"
#include "scene_description.h"

namespace fpl {
namespace pie_noon {

typedef float TimeStep;

class Particle {
 public:
  Particle() { reset(); }

  void reset();

  mathfu::vec3 CurrentPosition() const;
  mathfu::vec3 CurrentVelocity() const;
  Quat CurrentOrientation() const;
  mathfu::vec4 CurrentTint() const;
  mathfu::vec3 CurrentScale() const;
  TimeStep DurationRemaining() const;

  void SetDurationRemaining(TimeStep duration);

  mathfu::vec3 base_position() const { return base_position_; }

  void set_base_position(const mathfu::vec3& base_position) {
    base_position_ = base_position;
  }

  mathfu::vec3 base_velocity() const { return base_velocity_; }
  void set_base_velocity(const mathfu::vec3& base_velocity) {
    base_velocity_ = base_velocity;
  }

  mathfu::vec3 acceleration() const { return acceleration_; }
  void set_acceleration(const mathfu::vec3& acceleration) {
    acceleration_ = acceleration;
  }

  mathfu::vec3 base_orientation() const { return base_orientation_; }
  void set_base_orientation(const mathfu::vec3& base_orientation) {
    base_orientation_ = base_orientation;
  }

  mathfu::vec3 rotational_velocity() const { return rotational_velocity_; }
  void set_rotational_velocity(const mathfu::vec3& rotational_velocity) {
    rotational_velocity_ = rotational_velocity;
  }

  mathfu::vec4 base_tint() const { return base_tint_; }
  void set_base_tint(const mathfu::vec4& base_tint) { base_tint_ = base_tint; }

  mathfu::vec3 base_scale() const { return base_scale_; }
  void set_base_scale(const mathfu::vec3& base_scale) {
    base_scale_ = base_scale;
  }

  TimeStep duration_of_fade_out() const { return duration_of_fade_out_; }
  void set_duration_of_fade_out(TimeStep duration_of_fade_out) {
    duration_of_fade_out_ = duration_of_fade_out;
  }

  TimeStep duration_of_shrink_out() const { return duration_of_shrink_out_; }
  void set_duration_of_shrink_out(TimeStep duration_of_shrink_out) {
    duration_of_shrink_out_ = duration_of_shrink_out;
  }

  uint16_t renderable_id() const { return renderable_id_; }
  void set_renderable_id(uint16_t renderable_id) {
    renderable_id_ = renderable_id;
  }

  TimeStep duration() const { return duration_; }
  void set_duration(TimeStep duration) { duration_ = duration; }

  TimeStep age() const { return age_; }
  void set_age(TimeStep age) { age_ = age; }

  // Generate the matrix we'll need to draw it:
  mathfu::mat4 CalculateMatrix() const;

  void AdvanceFrame(TimeStep delta_time);
  bool IsFinished() const;

 private:
  mathfu::vec3 base_position_;
  mathfu::vec3 base_velocity_;
  mathfu::vec3 acceleration_;

  // Expressed in Euler angles:
  mathfu::vec3 base_orientation_;
  mathfu::vec3 rotational_velocity_;

  mathfu::vec3 base_scale_;
  mathfu::vec4 base_tint_;

  // How long the particle will last, in milliseconds.
  TimeStep duration_;

  // How long the particle has been alive so far, in milliseconds
  TimeStep age_;

  // How long it will take the particle to fade or shrink away, when it reaches
  // the end of its life span.  (In milliseconds)
  TimeStep duration_of_fade_out_;
  TimeStep duration_of_shrink_out_;

  // the renderable ID we should use when drawing this particle.
  uint16_t renderable_id_;
};

class ParticleManager {
 public:
  void AdvanceFrame(TimeStep delta_time);

  const std::list<Particle*>& get_particle_list() const {
    return particle_list_;
  }

  // Returns a pointer to a new particle, ready to be populated.
  // Note that this pointer is not guaranteed to be valid indefinitely!
  // Also note that the initial state of the particle is undetermined, so
  // users should either populate every field explicitly, or call
  // Particle::reset().
  Particle* CreateParticle();

  // Removes all active particles.
  void RemoveAllParticles();

 private:
  std::list<Particle*> particle_list_;
  std::list<Particle*> inactive_particle_list_;
};

}  // pie_noon
}  // fpl
#endif  // PARTICLES_H
