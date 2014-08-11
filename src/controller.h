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

#ifndef SPLAT_CONTROLLER_H_
#define SPLAT_CONTROLLER_H_

#include <cstdint>

namespace fpl {
namespace splat {

class Controller {
 public:
  Controller() : logical_inputs_(0u) {}
  virtual ~Controller() {}

  // Update the current state of this controller.
  virtual void AdvanceFrame() = 0;

  // Returns the current set of active logical input bits.
  uint32_t logical_inputs() const { return logical_inputs_; }

  // Updates a one or more bits.
  void SetLogicalInputs(uint32_t bitmap, bool set) {
    if (set) {
      logical_inputs_ |= bitmap;
    } else {
      logical_inputs_ &= ~bitmap;
    }
  }
 protected:
  // A bitfield of currently active logical input bits.
  uint32_t logical_inputs_;
};

}  // splat
}  // fpl

#endif  // SPLAT_CONTROLLER_H_

