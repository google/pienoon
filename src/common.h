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

#ifndef PIE_NOON_COMMON_H
#define PIE_NOON_COMMON_H

#include "mathfu/glsl_mappings.h"
#include "mathfu/quaternion.h"

namespace fpl {

// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
//
// For disallowing only assign or copy, write the code directly, but declare
// the intend in a comment, for example:
// void operator=(const TypeName&);  // DISALLOW_ASSIGN
// Note, that most uses of DISALLOW_ASSIGN and DISALLOW_COPY are broken
// semantically, one should either use disallow both or neither. Try to
// avoid these in new code.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

// Return the number of elements in an array 'a', as type `size_t`.
// If 'a' is not an array, generates an error by dividing by zero.
#define PIE_ARRAYSIZE(a)        \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

typedef mathfu::Quaternion<float> Quat;

// 1 WorldTime = 1 millisecond.
typedef int WorldTime;

const int kMillisecondsPerSecond = 1000;

typedef int CharacterId;

typedef int ControllerId;
const ControllerId kUndefinedController = -1;
const ControllerId kTouchController = -2;

}  // namespace fpl

#endif  // PIE_NOON_COMMON_H
