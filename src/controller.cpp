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

#include "controller.h"

namespace fpl {
namespace pie_noon {

void Controller::ClearAllLogicalInputs() {
  is_down_ = 0;
  went_down_ = 0;
  went_up_ = 0;
}

void Controller::SetLogicalInputs(uint32_t bitmap, bool set) {
  if (set) {
    uint32_t already_down = bitmap & is_down_;
    went_down_ |= bitmap & ~already_down;
    is_down_ |= bitmap;
  } else {
    went_up_ |= bitmap & is_down_;
    is_down_ &= ~bitmap;
  }
}

}  // pie_noon
}  // fpl
