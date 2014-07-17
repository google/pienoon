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

LOCAL_PATH:=$(call my-dir)/..

include $(CLEAR_VARS)
external_path:=$(LOCAL_PATH)/../../libs
source_path:=$(LOCAL_PATH)/src

LOCAL_MODULE:=splat
LOCAL_MODULE_TAGS:=optional

LOCAL_C_INCLUDES:=\
	$(source_path) \
	$(external_path)/fplutil/libfplutil/include
LOCAL_SRC_FILES:=\
	$(source_path)/main.cpp

LOCAL_CFLAGS:=\
	-Wall \
	-Werror \
	-Wno-long-long \
	-Wno-variadic-macros \
	-Wno-array-bounds

LOCAL_STATIC_LIBRARIES:=\
	android_native_app_glue \
    libandroidutil_static
LOCAL_LDLIBS:=-llog -landroid
LOCAL_ARM_MODE:=arm
include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
$(call import-module,liquidfun/Box2D/AndroidUtil/jni)
