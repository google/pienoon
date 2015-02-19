#include "particles.h"

namespace fpl {
namespace pie_noon {

const int kMaxParticles = 1000;

void Particle::reset() {
  base_position_ = mathfu::vec3(0, 0, 0);
  base_velocity_ = mathfu::vec3(0, 0, 0);
  acceleration_ = mathfu::vec3(0, 0, 0);
  base_orientation_ = mathfu::vec3(0, 0, 0);
  rotational_velocity_ = mathfu::vec3(0, 0, 0);
  base_scale_ = mathfu::vec3(1, 1, 1);
  base_tint_ = mathfu::vec4(1, 1, 1, 1);
  duration_ = 0;
  age_ = 0;
  duration_of_fade_out_ = 0;
  duration_of_shrink_out_ = 0;
}

mathfu::mat4 Particle::CalculateMatrix() const {
  return mathfu::mat4::FromTranslationVector(CurrentPosition()) *
         mathfu::mat4::FromRotationMatrix(CurrentOrientation().ToMatrix()) *
         mathfu::mat4::FromScaleVector(CurrentScale());
}

mathfu::vec3 Particle::CurrentPosition() const {
  return base_position_ + (base_velocity_ * age_) +
         (acceleration_ / 2.0) * age_ * age_;
}

mathfu::vec3 Particle::CurrentVelocity() const {
  return base_velocity_ + acceleration_ * age_;
}

Quat Particle::CurrentOrientation() const {
  return Quat::FromEulerAngles(base_orientation_ + rotational_velocity_ * age_);
}

TimeStep Particle::DurationRemaining() const { return duration_ - age_; }

void Particle::SetDurationRemaining(TimeStep duration) {
  duration_ = age_ + duration;
}

// Returns the current tint, after taking particle effects into account.
mathfu::vec4 Particle::CurrentTint() const {
  return base_tint_ *
         (((duration_ - age_) < duration_of_fade_out_)
              ? (float)(duration_ - age_) / (float)duration_of_fade_out_
              : 1.0f);
}

void Particle::AdvanceFrame(TimeStep delta_time) { age_ += delta_time; }

bool Particle::IsFinished() const { return age_ >= duration_; }

// Returns the current tint, after taking particle effects into account.
mathfu::vec3 Particle::CurrentScale() const {
  return base_scale_ *
         (((duration_ - age_) < duration_of_shrink_out_)
              ? (float)(duration_ - age_) / (float)duration_of_shrink_out_
              : 1.0f);
}

void ParticleManager::AdvanceFrame(TimeStep delta_time) {
  for (auto it = particle_list_.begin(); it != particle_list_.end();) {
    (*it)->AdvanceFrame(delta_time);
    if ((*it)->IsFinished()) {
      inactive_particle_list_.push_back(*it);
      it = particle_list_.erase(it);
    } else {
      ++it;
    }
  }
}

Particle* ParticleManager::CreateParticle() {
  Particle* result;
  if (inactive_particle_list_.size() > 0) {
    result = inactive_particle_list_.back();
    inactive_particle_list_.pop_back();
  } else {
    if (particle_list_.size() >= kMaxParticles) {
      return nullptr;
    }
    result = new Particle();
  }
  result->set_age(0);
  particle_list_.push_back(result);
  return result;
}

void ParticleManager::RemoveAllParticles() {
  for (auto it = particle_list_.begin(); it != particle_list_.end();) {
    inactive_particle_list_.push_back(*it);
    it = particle_list_.erase(it);
  }
}

}  // pie_noon
}  // fpl
