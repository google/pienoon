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

#ifndef SPLAT_SRC_PRECOMPILED_H
#define SPLAT_SRC_PRECOMPILED_H

#include <assert.h>
#include <cstdint>
#include <map>
#if defined(_WIN32) && !defined(_USE_MATH_DEFINES)
#define _USE_MATH_DEFINES
#endif // defined(_WIN32) && !defined(_USE_MATH_DEFINES)
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#if defined(_WIN32)
#include <direct.h> // for _chdir
#define NOMINMAX
#include <windows.h> // for Sleep
#else // !defined(_WIN32)
#include <unistd.h> // for chdir, usleep
#endif // !defined(_WIN32)

#include "flatbuffers/util.h"
#include "mathfu/matrix.h"
#include "mathfu/vector_4.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/quaternion.h"
#include "mathfu/constants.h"

#include <SDL.h>
#include "SDL_log.h"

#include "glplatform.h"

#ifdef _WIN32
#pragma hdrstop
#endif //  _WIN32

#endif // SPLAT_SRC_PRECOMPILED_H
