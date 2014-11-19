# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:=$(call my-dir)

# Project directory relative to this file.
PIE_NOON_DIR:=$(LOCAL_PATH)/../../..
include $(PIE_NOON_DIR)/jni/android_config.mk

LOCAL_PATH:=$(DEPENDENCIES_GPG_DIR)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

PRIVATE_APP_STL:=$(APP_STL)
PRIVATE_APP_STL:=$(PRIVATE_APP_STL:_shared=)
PRIVATE_APP_STL:=$(PRIVATE_APP_STL:_static=)

LOCAL_MODULE:=libgpg
LOCAL_SRC_FILES:=\
  lib/$(PRIVATE_APP_STL)/$(TARGET_ARCH_ABI)/libgpg.a

include $(PREBUILT_STATIC_LIBRARY)

