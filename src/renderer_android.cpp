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

#include <EGL/egl.h>

#include "precompiled.h"
#include "renderer.h"
#include "utilities.h"

// Include SDL internal headers and external refs
#define TARGET_OS_IPHONE 1 // This one is not to turn on 'SDL_DYNAMIC_API' defitnition
extern "C" {
#include "src/video/SDL_sysvideo.h"
#include "src/video/SDL_egl_c.h"
#include "src/core/android/SDL_android.h"
}
#undef TARGET_OS_IPHONE  // We don't need this anymore

namespace fpl {

// Quick hack for HW scaler setting
static vec2i g_android_scaler_resolution;

void AndroidSetScalerResolution(const vec2i& resolution) {
  ANativeWindow* window = Android_JNI_GetNativeWindow();
  int width = std::min(ANativeWindow_getWidth(window), resolution.x());
  int height = std::min(ANativeWindow_getHeight(window), resolution.y());
  g_android_scaler_resolution = vec2i(width, height);
}

const vec2i& AndroidGetScalerResolution() {
  return g_android_scaler_resolution;
}

EGLAPI EGLSurface EGLAPIENTRY HookEglCreateWindowSurface(
    EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list) {
  // Apply scaler setting
  AndroidPreCreateWindow();
  return eglCreateWindowSurface(dpy, config, win, attrib_list);
}

void AndroidPreCreateWindow() {
  // Apply scaler setting prior creating surface
  if (g_android_scaler_resolution.x() && g_android_scaler_resolution.y()) {
    // Enable HW scaler
    ANativeWindow* window = Android_JNI_GetNativeWindow();
    ANativeWindow_setBuffersGeometry(window,
      g_android_scaler_resolution.x(), g_android_scaler_resolution.y(), 0);
  }
}

void AndroidPostCreateWindow() {
  // Hook eglCreateWindowSurface call
  // the eglCreateWindowSurface pointer is set inside SDL_CreateWindow(),
  // so that what we need to do is:
  // 1) Set scaler setting before SDL_CreateWindow() for a surface being created in the API
  // 2) After the API call, hook eglCreateWindowSurface pointer for surfaces that will be
  //    created when EGLsurface is lost
  SDL_VideoDevice *device = SDL_GetVideoDevice();
  device->egl_data->eglCreateWindowSurface = HookEglCreateWindowSurface;
}

}  // namespace fpl

