LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# relative to this file
SPLAT_PATH := ../..
SDL_PATH := $(SPLAT_PATH)/../../../../external/sdl

# relative to project root
SDL_PATH_INCLUDE := ../../../../external/sdl
FLATBUFFERS_PATH_INCLUDE := ../../libs/flatbuffers
MATHFU_PATH_INCLUDE := ../../libs/mathfu
# FIXME relies on cmake to run before this
GENERATED_FILES := CMakeFiles

LOCAL_C_INCLUDES := $(SDL_PATH_INCLUDE)/include \
                    $(FLATBUFFERS_PATH_INCLUDE)/include \
                    $(MATHFU_PATH_INCLUDE)/include \
                    $(GENERATED_FILES)/include \
                    src

LOCAL_SRC_FILES := \
  $(SDL_PATH)/src/main/android/SDL_android_main.c \
	$(SPLAT_PATH)/src/character.cpp \
	$(SPLAT_PATH)/src/character_state_machine.cpp \
	$(SPLAT_PATH)/src/game_state.cpp \
	$(SPLAT_PATH)/src/input.cpp \
	$(SPLAT_PATH)/src/main.cpp \
	$(SPLAT_PATH)/src/material_manager.cpp \
	$(SPLAT_PATH)/src/precompiled.cpp \
	$(SPLAT_PATH)/src/renderer.cpp \
	$(SPLAT_PATH)/src/sdl_controller.cpp \
	$(SPLAT_PATH)/src/splat_game.cpp \
	$(SPLAT_PATH)/src/utilities.cpp

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)

