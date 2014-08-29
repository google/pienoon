LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# relative to this file
SPLAT_PATH := ../..

# relative to project root
SDL_PATH := ../../../../external/sdl
MIXER_PATH := ../../../../external/sdl_mixer
FLATBUFFERS_PATH := ../../libs/flatbuffers
MATHFU_PATH := ../../libs/mathfu
GPG_PATH := ../../../../prebuilts/gpg-cpp-sdk/android
# FIXME relies on cmake to run before this
GENERATED_FILES := CMakeFiles

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
                    $(MIXER_PATH) \
                    $(FLATBUFFERS_PATH)/include \
                    $(MATHFU_PATH)/include \
                    $(GENERATED_FILES)/include \
                    $(GPG_PATH)/include \
                    src

LOCAL_SRC_FILES := \
  $(SPLAT_PATH)/$(SDL_PATH)/src/main/android/SDL_android_main.c \
	$(SPLAT_PATH)/src/character.cpp \
	$(SPLAT_PATH)/src/character_state_machine.cpp \
	$(SPLAT_PATH)/src/game_state.cpp \
	$(SPLAT_PATH)/src/input.cpp \
	$(SPLAT_PATH)/src/main.cpp \
	$(SPLAT_PATH)/src/material_manager.cpp \
	$(SPLAT_PATH)/src/player_controller.cpp \
	$(SPLAT_PATH)/src/precompiled.cpp \
	$(SPLAT_PATH)/src/renderer.cpp \
	$(SPLAT_PATH)/src/splat_game.cpp \
	$(SPLAT_PATH)/src/utilities.cpp

LOCAL_STATIC_LIBRARIES := libgpg

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz

include $(BUILD_SHARED_LIBRARY)

