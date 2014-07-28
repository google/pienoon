/*
* Copyright 2014 Google Inc. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef SPLAT_SRC_PRECOMPILED_H
#define SPLAT_SRC_PRECOMPILED_H

#include <stdio.h>
#include <string.h>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

#include "matrices/Matrix.h"
#include "vectors/Vector_4D.h"
#include "vectors/GLSL_Mappings.h"

#ifdef __ANDROID__
#include <AndroidUtil/AndroidMainWrapper.h>
#include <AndroidUtil/AndroidLogPrint.h>
#endif // __ANDROID__

#include <SDL.h>

#include "glplatform.h"

// TODO: renderer & input system both rely on this define,
// move elsewhere?
#if defined(__IOS__) || defined(__ANDROID__)
  #define PLATFORM_MOBILE
#endif

#ifdef _WIN32
#pragma hdrstop
#endif //  _WIN32

#endif // SPLAT_SRC_PRECOMPILED_H
