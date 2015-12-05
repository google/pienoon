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
#include "analytics_tracking.h"

namespace fpl {

void SendTrackerEvent(const char *category, const char *action) {
#ifdef __ANDROID__
  fplbase::LogInfo(fplbase::kApplication, "SendTrackerEvent (%s, %s)\n",
                   category, action);
  JNIEnv *env = fplbase::AndroidGetJNIEnv();
  jobject activity = fplbase::AndroidGetActivity();
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID send_tracker_event = env->GetMethodID(
      fpl_class, "SendTrackerEvent", "(Ljava/lang/String;Ljava/lang/String;)V");
  jstring category_string = env->NewStringUTF(category);
  jstring action_string = env->NewStringUTF(action);
  env->CallVoidMethod(activity, send_tracker_event, category_string,
                      action_string);
  env->DeleteLocalRef(action_string);
  env->DeleteLocalRef(category_string);
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
#else
  (void)category;
  (void)action;
#endif
}

void SendTrackerEvent(const char *category, const char *action,
                      const char *label) {
#ifdef __ANDROID__
  fplbase::LogInfo(fplbase::kApplication, "SendTrackerEvent (%s, %s, %s)\n",
                   category, action,
          label);
  JNIEnv *env = fplbase::AndroidGetJNIEnv();
  jobject activity = fplbase::AndroidGetActivity();
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID send_tracker_event = env->GetMethodID(
      fpl_class, "SendTrackerEvent",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
  jstring category_string = env->NewStringUTF(category);
  jstring action_string = env->NewStringUTF(action);
  jstring label_string = env->NewStringUTF(label);
  env->CallVoidMethod(activity, send_tracker_event, category_string,
                      action_string, label_string);
  env->DeleteLocalRef(label_string);
  env->DeleteLocalRef(action_string);
  env->DeleteLocalRef(category_string);
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
#else
  (void)category;
  (void)action;
  (void)label;
#endif
}

void SendTrackerEvent(const char *category, const char *action,
                      const char *label, int value) {
#ifdef __ANDROID__
  fplbase::LogInfo(fplbase::kApplication, "SendTrackerEvent (%s, %s, %s, %i)\n",
                   category, action,
          label, value);
  JNIEnv *env = fplbase::AndroidGetJNIEnv();
  jobject activity = fplbase::AndroidGetActivity();
  jclass fpl_class = env->GetObjectClass(activity);
  jmethodID send_tracker_event = env->GetMethodID(
      fpl_class, "SendTrackerEvent",
      "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");
  jstring category_string = env->NewStringUTF(category);
  jstring action_string = env->NewStringUTF(action);
  jstring label_string = env->NewStringUTF(label);
  env->CallVoidMethod(activity, send_tracker_event, category_string,
                      action_string, label_string, value);
  env->DeleteLocalRef(label_string);
  env->DeleteLocalRef(action_string);
  env->DeleteLocalRef(category_string);
  env->DeleteLocalRef(fpl_class);
  env->DeleteLocalRef(activity);
#else
  (void)category;
  (void)action;
  (void)label;
  (void)value;
#endif
}

}  // namespace fpl
