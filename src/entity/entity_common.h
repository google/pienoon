// Copyright 2015 Google Inc. All rights reserved.
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

#include <stdint.h>
#include <stddef.h>

#ifndef FPL_ENTITY_COMMON_H_
#define FPL_ENTITY_COMMON_H_

namespace fpl {
namespace entity {

// Maximum number of components in the system.  This should be set with care,
// because it affects the size of each entity.  Ideally this should be set
// to something that is as close as possible to the actual number of components
// used by the program.
#define FPL_ENTITY_MAX_COMPONENT_COUNT 20

// Pick an index type based on the max number of components.
#if kMaxComponentCount <= 0xff
typedef uint8_t ComponentId;
const ComponentId kInvalidComponent = 0xff;
#elif kMaxComponentCount <= 0xffff
typedef uint16_t ComponentId;
const ComponentId kInvalidComponent = 0xffff;
#else
// if you hit this one, you are probably doing something really odd.
typedef uint32_t ComponentId;
const ComponentId kInvalidComponent = 0xffffffffL;
#endif

static const ComponentId kMaxComponentCount = FPL_ENTITY_MAX_COMPONENT_COUNT;

typedef int WorldTime;
const int kMillisecondsPerSecond = 1000;
typedef uint16_t ComponentIndex;
static const ComponentIndex kUnusedComponentIndex =
    static_cast<ComponentIndex>(-1);

}  // entity
}  // fpl
#endif  // FPL_ENTITY_COMMON_H_
