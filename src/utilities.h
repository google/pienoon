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

#ifndef SPLAT_UTILITIES_H
#define SPLAT_UTILITIES_H

#include <string>

#include "mathfu/utilities.h"

// TODO: put in alphabetical order when FlatBuffers predeclare bug fixed.
#include "splat_common_generated.h"
#include "config_generated.h"

namespace fpl
{

inline const mathfu::vec3& LoadVec3(const splat::Vec3* v) {
  return union_reinterpret_cast<mathfu::vec3, splat::Vec3>(*v);
}

inline const mathfu::vec2i& LoadVec2i(const splat::Vec2i* v) {
  return union_reinterpret_cast<mathfu::vec2i, splat::Vec2i>(*v);
}

bool ChangeToUpstreamDir(const char* const target_dir,
                         const char* const suffix_dirs[],
                         size_t num_suffixes);

std::string CamelCaseToSnakeCase(const char* const camel);

std::string FileNameFromEnumName(const char* const enum_name,
                                 const char* const prefix,
                                 const char* const suffix);

void SleepForMilliseconds(const uint32_t milliseconds);

} // namespace fpl

#endif // SPLAT_UTILITIES_H
