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

#ifndef IMPEL_UTIL_H_
#define IMPEL_UTIL_H_

#include "impeller.h"

namespace impel {


enum TwitchDirection {
  kTwitchDirectionNone,        // Do nothing.
  kTwitchDirectionPositive,    // Give the velocity a positive boost.
  kTwitchDirectionNegative     // Give the velocity a negative boost.
};


// Helper to determine if we're "at the target" and "stopped".
struct Settled1f {
  // Consider ourselves "at the target" if the absolute difference between
  // the value and the target is less than this.
  float max_difference;

  // Consider ourselves "stopped" if the absolute velocity is less than this.
  float max_velocity;

  Settled1f() : max_difference(0.0f), max_velocity(0.0f) {}

  bool Settled(float dist, float velocity) const {
    return fabs(dist) <= max_difference && fabs(velocity) <= max_velocity;
  }

  bool Settled(const Impeller1f& impeller) const {
    return Settled(impeller.Difference(), impeller.Velocity());
  }
};


inline void Twitch(TwitchDirection direction, float velocity,
                   const Settled1f& settled, Impeller1f* impeller) {
  if (direction != kTwitchDirectionNone && settled.Settled(*impeller)) {
    impeller->SetVelocity(direction == kTwitchDirectionPositive ? velocity :
                                                                  -velocity);
  }
}


} // namespace impel

#endif // IMPEL_UTIL_H_

