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
#include "bus.h"
#include "buses_generated.h"
#include "mathfu/utilities.h"

namespace fpl {

Bus::Bus(const BusDef* bus_def)
    : bus_def_(bus_def),
      duck_gain_(1.0f),
      sound_count_(0),
      transition_percentage_(0.0f) {}

void Bus::UpdateDuckGain(WorldTime delta_time) {
  if (sound_count_ > 0 && transition_percentage_ <= 1.0f) {
    // Fading to duck gain.
    float fade_in_time = bus_def_->duck_fade_in_time();
    if (fade_in_time > 0) {
      float delta_seconds = delta_time * kMillisecondsPerSecond;
      transition_percentage_ += delta_seconds / fade_in_time;
      transition_percentage_ = std::min(transition_percentage_, 1.0f);
    } else {
      transition_percentage_ = 1.0f;
    }
  } else if (sound_count_ == 0 && transition_percentage_ >= 0.0f) {
    // Fading to standard gain.
    float fade_out_time = bus_def_->duck_fade_out_time();
    if (fade_out_time > 0) {
      float delta_seconds = delta_time * kMillisecondsPerSecond;
      transition_percentage_ -= delta_seconds / fade_out_time;
      transition_percentage_ = std::max(transition_percentage_, 0.0f);
    } else {
      transition_percentage_ = 0.0f;
    }
  }
  float duck_gain = mathfu::Lerp(1.0f, bus_def_->duck_gain(),
                                 transition_percentage_);
  for (size_t i = 0; i < duck_buses_.size(); ++i) {
    Bus* bus = duck_buses_[i];
    bus->duck_gain_ = std::min(duck_gain, bus->duck_gain_);
  }
}

void Bus::UpdateGain(float parent_gain) {
  gain_ = bus_def_->gain() * parent_gain * duck_gain_;
  for (size_t i = 0; i < child_buses_.size(); ++i) {
    child_buses_[i]->UpdateGain(gain_);
  }
}

void Bus::IncrementSoundCounter() {
  ++sound_count_;
}

void Bus::DecrementSoundCounter() {
  --sound_count_;
  assert(sound_count_ >= 0);
}

}  // namespace fpl

