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

#ifndef MAGNET_1F_H_
#define MAGNET_1F_H_

#include "magnet_base.h"
#include "magnet_generated.h"

namespace fpl {

// One-dimensional float precision magnet.
// Contains utility functions that all derivations can use.
class Magnet1f : public MagnetBase<float, MagnetConstraints1f> {
 protected:
  float Normalize(float x) const;
  float CalculateDifference() const;
  float CalculatePosition(WorldTime delta_time, float velocity) const;
  float ClampVelocity(float velocity) const;
};

typedef MagnetState<float> MagnetState1f;
typedef MagnetTarget<float> MagnetTarget1f;


// This magnet continuously accelerates towards a target. It will inevitably
// overshoot the target and have to break to return. The deceleration (when
// moving away from the target) is higher than the accelleration (when moving
// towards the target). This is necessary to ensure we eventually settle on
// the target; if acceleration and deceleration were equal we'd oscillate.
//
// This class implements 'Twitch' so you can fake responses to user input.
class OvershootMagnet1f : public Magnet1f {
 public:
  OvershootMagnet1f() : def_(nullptr) {}

  void Initialize(const MagnetConstraints1f& constraints,
                  const OvershootMagnet1fDef& def, const MagnetState1f& state) {
    constraints_ = &constraints;
    state_ = state;
    def_ = &def;
    target_.Reset();
  }

  virtual void AdvanceFrame(WorldTime delta_time);
  virtual void Twitch(MagnetTwitch twitch);
  virtual bool Settled() const;

  const OvershootMagnet1fDef* definition() const { return def_; }

 private:
  float CalculateVelocity(WorldTime delta_time) const;
  float CalculateTwitchVelocity(MagnetTwitch twitch) const;

  const OvershootMagnet1fDef* def_;
};

} // namespace fpl

#endif // MAGNET_1F_H_
