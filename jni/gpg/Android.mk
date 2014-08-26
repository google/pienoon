LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GPG_PATH := ../../../../prebuilts/gpg-cpp-sdk/android

PRIVATE_APP_STL := $(APP_STL)
PRIVATE_APP_STL := $(PRIVATE_APP_STL:_shared=)
PRIVATE_APP_STL := $(PRIVATE_APP_STL:_static=)

LOCAL_MODULE    := libgpg
LOCAL_SRC_FILES := \
  ../../$(GPG_PATH)/lib/$(PRIVATE_APP_STL)/$(TARGET_ARCH_ABI)/libgpg.a

include $(PREBUILT_STATIC_LIBRARY)

