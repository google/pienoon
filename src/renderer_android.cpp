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
#define TARGET_OS_IPHONE \
  1  // This one is not to turn on 'SDL_DYNAMIC_API' defitnition
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
  // Check against the real size of the device
  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID get_size =
      env->GetMethodID(fpl_class, "GetLandscapedSize", "()[I");
  jintArray size = (jintArray)env->CallObjectMethod(activity, get_size);
  jint* size_ints = env->GetIntArrayElements(size, NULL);

  int width = std::min(size_ints[0], resolution.x());
  int height = std::min(size_ints[1], resolution.y());
  g_android_scaler_resolution = vec2i(width, height);

  // Update the underlying activity with the scaled resolution
  jmethodID set_resolution =
      env->GetMethodID(fpl_class, "SetHeadMountedDisplayResolution", "(II)V");
  env->CallVoidMethod(activity, set_resolution, width, height);

  env->ReleaseIntArrayElements(size, size_ints, JNI_ABORT);
  env->DeleteLocalRef(size);
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
}

const vec2i& AndroidGetScalerResolution() {
  return g_android_scaler_resolution;
}

EGLAPI EGLSurface EGLAPIENTRY
HookEglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
                           EGLNativeWindowType win, const EGLint* attrib_list) {
  // Apply scaler setting
  ANativeWindow* window = Android_JNI_GetNativeWindow();
  ANativeWindow_setBuffersGeometry(window, g_android_scaler_resolution.x(),
                                   g_android_scaler_resolution.y(), 0);
  return eglCreateWindowSurface(dpy, config, win, attrib_list);
}

void AndroidPreCreateWindow() {
  // Apply scaler setting prior creating surface
  if (g_android_scaler_resolution.x() && g_android_scaler_resolution.y()) {
    // Initialize OpenGL function pointers inside SDL
    if (SDL_GL_LoadLibrary(NULL) < 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "couldn't initialize OpenGL library\n");
    }

    // Hook eglCreateWindowSurface call
    SDL_VideoDevice* device = SDL_GetVideoDevice();
    device->egl_data->eglCreateWindowSurface = HookEglCreateWindowSurface;
  }
}

}  // namespace fpl
