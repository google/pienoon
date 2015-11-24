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

#include "precompiled.h"

#include "utilities.h"

namespace fpl {

bool LoadFile(const char* filename, std::string* dest) {
  auto handle = SDL_RWFromFile(filename, "rb");
  if (!handle) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "LoadFile fail on %s", filename);
    return false;
  }
  auto len = static_cast<size_t>(SDL_RWseek(handle, 0, RW_SEEK_END));
  SDL_RWseek(handle, 0, RW_SEEK_SET);
  dest->assign(len + 1, 0);
  size_t rlen = static_cast<size_t>(SDL_RWread(handle, &(*dest)[0], 1, len));
  SDL_RWclose(handle);
  return len == rlen && len > 0;
}

#if defined(_WIN32)
inline char* getcwd(char* buffer, int maxlen) {
  return _getcwd(buffer, maxlen);
}

inline int chdir(const char* dirname) { return _chdir(dirname); }
#endif  // defined(_WIN32)

// Search up the directory tree from binary_dir for target_dir, changing the
// working directory to the target_dir and returning true if it's found,
// false otherwise.
bool ChangeToUpstreamDir(const char* const binary_dir,
                         const char* const target_dir) {
#if defined(__IOS__)
  // For iOS the assets are bundled under pie_noon_ios.app/assets
  (void)binary_dir;
  char real_path[256];
  std::string current_dir = getcwd(real_path, sizeof(real_path));
  current_dir += flatbuffers::kPathSeparator;
  current_dir += target_dir;
  int success = chdir(current_dir.c_str());
  if (success == 0) return true;
  SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to change directory to %s",
               current_dir.c_str());
  return false;
#elif !defined(__ANDROID__)
  {
    std::string current_dir = binary_dir;

    // Search up the tree from the directory containing the binary searching
    // for target_dir.
    for (;;) {
      size_t separator = current_dir.find_last_of(flatbuffers::kPathSeparator);
      if (separator == std::string::npos) break;
      current_dir = current_dir.substr(0, separator);
      printf("%s\n", current_dir.c_str());
      int success = chdir(current_dir.c_str());
      if (success) break;
      char real_path[256];
      current_dir = getcwd(real_path, sizeof(real_path));
      std::string target = current_dir +
                           std::string(1, flatbuffers::kPathSeparator) +
                           std::string(target_dir);
      success = chdir(target.c_str());
      if (success == 0) return true;
    }
    return false;
  }
#else
  (void)binary_dir;
  (void)target_dir;
  return true;
#endif  // !defined(__ANDROID__)
}

static inline bool IsUpperCase(const char c) { return c == toupper(c); }

std::string CamelCaseToSnakeCase(const char* const camel) {
  // Replace capitals with underbar + lowercase.
  std::string snake;
  for (const char* c = camel; *c != '\0'; ++c) {
    if (IsUpperCase(*c)) {
      const bool is_start_or_end = c == camel || *(c + 1) == '\0';
      if (!is_start_or_end) {
        snake += '_';
      }
      snake += static_cast<char>(tolower(*c));
    } else {
      snake += *c;
    }
  }
  return snake;
}

std::string FileNameFromEnumName(const char* const enum_name,
                                 const char* const prefix,
                                 const char* const suffix) {
  // Skip over the initial 'k', if it exists.
  const bool starts_with_k = enum_name[0] == 'k' && IsUpperCase(enum_name[1]);
  const char* const camel_case_name = starts_with_k ? enum_name + 1 : enum_name;

  // Assemble file name.
  return std::string(prefix) + CamelCaseToSnakeCase(camel_case_name) +
         std::string(suffix);
}

#ifdef __ANDROID__
bool AndroidSystemFeature(const char* feature_name) {
  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID has_system_feature =
      env->GetMethodID(fpl_class, "hasSystemFeature", "(Ljava/lang/String;)Z");
  jstring jfeature_name = env->NewStringUTF(feature_name);
  jboolean has_feature =
      env->CallBooleanMethod(activity, has_system_feature, jfeature_name);
  env->DeleteLocalRef(jfeature_name);
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
  return has_feature;
}
#endif

bool TouchScreenDevice() {
#ifdef __ANDROID__
  return AndroidSystemFeature("android.hardware.touchscreen");
#else
  return false;
#endif
}

#ifdef __ANDROID__
bool AndroidCheckDeviceList(const char* device_list[], const int num_devices) {
  // Retrieve device name through JNI.
  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jclass build_class = env->FindClass("android/os/Build");
  jfieldID model_id =
      env->GetStaticFieldID(build_class, "MODEL", "Ljava/lang/String;");
  jstring model_obj =
      static_cast<jstring>(env->GetStaticObjectField(build_class, model_id));
  const char* device_name = env->GetStringUTFChars(model_obj, 0);

  // Check if the device name is in the list.
  bool result = true;
  for (int i = 0; i < num_devices; ++i) {
    if (strcmp(device_list[i], device_name) == 0) {
      result = false;
      break;
    }
  }

  // Clean up
  env->ReleaseStringUTFChars(model_obj, device_name);
  env->DeleteLocalRef(model_obj);
  env->DeleteLocalRef(build_class);
  return result;
}
#endif

bool MipmapGeneration16bppSupported() {
#ifdef __ANDROID__
  const char* device_list[] = {"Galaxy Nexus"};
  return AndroidCheckDeviceList(device_list,
                                sizeof(device_list) / sizeof(device_list[0]));
#else
  return true;
#endif
}

#ifdef __ANDROID__
static jobject GetSharedPreference(JNIEnv *env, jobject activity) {
  jclass activity_class = env->GetObjectClass(activity);
  jmethodID get_preferences = env->GetMethodID(
      activity_class, "getSharedPreferences",
      "(Ljava/lang/String;I)Landroid/content/SharedPreferences;");
  jstring file = env->NewStringUTF("preference");
  // The last value: Content.MODE_PRIVATE = 0.
  jobject preference =
      env->CallObjectMethod(activity, get_preferences, file, 0);
  env->DeleteLocalRef(activity_class);
  env->DeleteLocalRef(file);
  return preference;
}
#endif  // __ANDROID__

int32_t LoadPreference(const char* key, int32_t initial_value) {
#ifdef __ANDROID__
  // Use Android preference API to store an integer value.
  JNIEnv *env = reinterpret_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jobject preference = GetSharedPreference(env, activity);
  jclass preference_class = env->GetObjectClass(preference);

  // Retrieve blob as a String.
  jstring pref_key = env->NewStringUTF(key);
  jmethodID get_int= env->GetMethodID(
      preference_class, "getInt",
      "(Ljava/lang/String;I)I");
  auto value = env->CallIntMethod(preference, get_int, pref_key, initial_value);

  // Release objects references.
  env->DeleteLocalRef(pref_key);
  env->DeleteLocalRef(preference_class);
  env->DeleteLocalRef(preference);
  env->DeleteLocalRef(activity);
  return value;
#else
  (void)key;
  return initial_value;
#endif
}

bool SavePreference(const char* key, int32_t value) {
#ifdef __ANDROID__
  // Use Android preference API to store an integer value.
  JNIEnv *env = reinterpret_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jobject preference = GetSharedPreference(env, activity);
  jclass preference_class = env->GetObjectClass(preference);

  // Retrieve an editor.
  jmethodID edit = env->GetMethodID(
      preference_class, "edit", "()Landroid/content/SharedPreferences$Editor;");
  jobject editor = env->CallObjectMethod(preference, edit);
  jclass editor_class = env->GetObjectClass(editor);

  // Put blob as a String.
  jstring pref_key = env->NewStringUTF(key);
  jmethodID put_int =
      env->GetMethodID(editor_class, "putInt",
                       "(Ljava/lang/String;I)Landroid/content/"
                       "SharedPreferences$Editor;");
  env->CallObjectMethod(editor, put_int, pref_key, value);

  // Commit a change.
  jmethodID commit = env->GetMethodID(editor_class, "commit", "()Z");
  jboolean ret = env->CallBooleanMethod(editor, commit);

  // Release objects references.
  env->DeleteLocalRef(pref_key);
  env->DeleteLocalRef(editor_class);
  env->DeleteLocalRef(editor);
  env->DeleteLocalRef(preference_class);
  env->DeleteLocalRef(preference);
  env->DeleteLocalRef(activity);

  return ret;
#else
  (void)key;
  (void)value;
  return false;
#endif  // __ANDROID__
}

void RelaunchApplication() {
#ifdef __ANDROID__
  JNIEnv* env = reinterpret_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
  jobject activity = reinterpret_cast<jobject>(SDL_AndroidGetActivity());
  jclass fpl_class = env->GetObjectClass(activity);

  jmethodID mid_relaunch = env->GetMethodID(fpl_class, "relaunch", "()V");
  env->CallVoidMethod(activity, mid_relaunch);

  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
#endif  // __ANDROID__
}

}  // namespace fpl
