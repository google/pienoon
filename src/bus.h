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

#ifndef FPL_BUSES_H_
#define FPL_BUSES_H_

#include "precompiled.h"
#include <vector>
#include "common.h"

namespace fpl {

struct BusDef;

class Bus {
 public:
  Bus(const BusDef* bus_def);

  // Return the bus definition.
  const BusDef* bus_def() const { return bus_def_; }

  // Return the final gain after all modifiers have been applied (parent gain,
  // duck gain, bus gain).
  float gain() const { return gain_; }

  // Resets the duck gain to 1.0f. Duck gain must be reset each frame before
  // modifying it.
  void ResetDuckGain() { duck_gain_ = 1.0f; }

  // Return the vector of child buses.
  std::vector<Bus*>& child_buses() { return child_buses_; }

  // Return the vector of duck buses, the buses to be ducked when a sound is
  // playing on this bus.
  std::vector<Bus*>& duck_buses() { return duck_buses_; }

  // When a sound begins playing or finishes playing, the sound counter should
  // be incremented or decremented appropriately to track whether or not to
  // apply the duck gain.
  void IncrementSoundCounter();
  void DecrementSoundCounter();

  // Apply appropriate duck gain to all ducked buses.
  void UpdateDuckGain(WorldTime delta_time);

  // Recursively update the final gain of the bus.
  void UpdateGain(float parent_gain);

 private:
  const BusDef* bus_def_;

  // Children of a given bus have their gain multiplied against their parent's
  // gain.
  std::vector<Bus*> child_buses_;

  // When a sound is played on this bus, sounds played on these buses should be
  // ducked.
  std::vector<Bus*> duck_buses_;

  // The current duck_gain_ of this bus to be applied to all buses in
  // duck_buses_.
  float duck_gain_;

  // The final gain to be applied to all sounds on this bus.
  float gain_;

  // Keeps track of how many sounds are being played on this bus.
  int sound_count_;

  // If a sound is playing on this bus, all duck_buses_ should lower in volume
  // over time. This tracks how far we are into that transition.
  float transition_percentage_;
};

}  // namespace fpl

#endif  // FPL_BUSES_H_
