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

#ifndef PIE_NOON_SRC_PRECOMPILED_H
#define PIE_NOON_SRC_PRECOMPILED_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

#if defined(_WIN32)
#include <direct.h>  // for _chdir
#define NOMINMAX
#else                // !defined(_WIN32)
#include <unistd.h>  // for chdir
#endif               // !defined(_WIN32)

#include "flatbuffers/util.h"

#include "mathfu/constants.h"
#include "mathfu/glsl_mappings.h"
#include "mathfu/matrix.h"
#include "mathfu/quaternion.h"
#include "mathfu/utilities.h"
#include "mathfu/vector_4.h"

#include "SDL.h"
#include "SDL_log.h"
#include "SDL_mixer.h"

#ifdef __ANDROID__
#include "gpg/gpg.h"
#endif

#include "flatui/flatui.h"
#include "fplbase/asset_manager.h"
#include "fplbase/flatbuffer_utils.h"
#include "fplbase/input.h"
#include "fplbase/shader.h"
#include "fplbase/utilities.h"

// TODO: This is BAD, but an older version of Pie Noon had a complete
// copy of FPLBase & FlatUI as part of its code, so this achieves the exact same
// effect. Should replace this in the future.
namespace fpl {
using namespace fplbase;
using namespace flatui;
using mathfu::mat4;
using mathfu::vec2;
using mathfu::vec2i;
using mathfu::vec3;
using mathfu::vec4;
}

#ifdef _WIN32
#pragma hdrstop
#endif  //  _WIN32

#endif  // PIE_NOON_SRC_PRECOMPILED_H
