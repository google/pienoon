#include "particles.h"


namespace fpl {
namespace splat {

mathfu::mat4 Particle::CalculateMatrix() const {
  return mathfu::mat4::FromTranslationVector(position_) *
         mathfu::mat4::FromRotationMatrix(orientation_.ToMatrix()) *
         mathfu::mat4::FromScaleVector(CalculateScale());
}

void Particle::reset() {
  position_ = mathfu::vec3(0, 0, 0);
  velocity_ = mathfu::vec3(0, 0, 0);
  acceleration_ = mathfu::vec3(0, 0, 0);
  orientation_ = Quat(0, 0, 0, 1);
  rotational_velocity_ = Quat(0, 0, 0, 1);
  base_scale_ = mathfu::vec3(1, 1, 1);
  base_tint_ = mathfu::vec4(1, 1, 1, 1);
  duration_remaining_= 0;
  duration_of_fade_out_ = 0;
  duration_of_shrink_out_ = 0;
}

// Returns the current tint, after taking particle effects into account.
mathfu::vec4 Particle::CalculateTint() const {
  return base_tint_ *
      ((duration_remaining_ < duration_of_fade_out_) ?
      ((float)duration_remaining_ / (float)duration_of_fade_out_) :
      1.0);
}

// Returns the current tint, after taking particle effects into account.
mathfu::vec3 Particle::CalculateScale() const {
  return base_scale_ *
      ((duration_remaining_ < duration_of_shrink_out_) ?
      (float)duration_remaining_ / (float)duration_of_shrink_out_ :
      1.0);
}

void ParticleManager::AdvanceFrame(WorldTime delta_time) {
  for (auto it = particle_list_.begin(); it != particle_list_.end(); ) {
    bool is_still_active = AdvanceParticle((*it), delta_time);
    if (!is_still_active) {
      inactive_particle_list_.push_back(*it);
      it = particle_list_.erase(it);
    } else {
      ++it;
    }
  }
}

bool ParticleManager::AdvanceParticle(Particle * p, WorldTime delta_time) {
  p->set_position(p->position() + p->velocity() * delta_time);
  p->set_velocity(p->velocity() + p->acceleration() * delta_time);
  p->set_orientation((p->rotational_velocity() * delta_time) *
                     p->orientation());
  if (p->duration_remaining() < delta_time) {
    return false;
  } else {
    p->set_duration_remaining(p->duration_remaining() - delta_time);
    return true;
  }
  return !(p->duration_remaining() <= 0);
}

Particle* ParticleManager::CreateParticle() {
  Particle* result;
  if (inactive_particle_list_.size() > 0) {
    result = inactive_particle_list_.back();
    inactive_particle_list_.pop_back();
  } else {
    result = new Particle();
  }
  particle_list_.push_back(result);
  return result;
}


}  // splat
}  // fpl
