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
PIE_NOON_DIR:=$(realpath $(LOCAL_PATH)/../..)
include $(PIE_NOON_DIR)/jni/android_config.mk

# relative to project root

# The following block generates build rules which result in headers being
# rebuilt from flatbuffers schemas.

# Directory that contains the FlatBuffers compiler.
FLATBUFFERS_FLATC_PATH?=$(realpath $(PIE_NOON_DIR)/bin)

# Macro which searches build locations for the flatbuffers compiler.
# The FLATC can be used to override the location of flatc.
define find_flatc
  $(wildcard \
    $(realpath $(firstword \
      $(wildcard $(FLATC)) \
      $(wildcard $(FLATBUFFERS_FLATC_PATH)/flatc*) \
      $(wildcard $(FLATBUFFERS_FLATC_PATH)/Release/flatc*) \
      $(wildcard $(FLATBUFFERS_FLATC_PATH)/Debug/flatc*))))
endef

PROJECT_OS:=$(OS)
ifeq (,$(OS))
PROJECT_OS:=$(shell uname -s)
else
ifneq ($(findstring Windows,$(PROJECT_OS)),)
PROJECT_OS:=Windows
endif
endif

# Search for cmake.
CMAKE_ROOT:=$(realpath $(PIE_NOON_DIR)/../../../../prebuilts/cmake)
ifeq (,$(CMAKE))
ifeq (Linux,$(PROJECT_OS))
CMAKE:=$(wildcard $(CMAKE_ROOT)/linux-x86/current/bin/cmake*)
endif
ifeq (Darwin,$(PROJECT_OS))
CMAKE:=$(wildcard $(CMAKE_ROOT)/darwin-x86_64/current/*.app/Contents/bin/cmake)
endif
ifeq (Windows,$(PROJECT_OS))
CMAKE:=$(wildcard $(CMAKE_ROOT)/windows/current/bin/cmake*)
endif
endif
ifeq (,$(CMAKE))
CMAKE:=cmake
endif

# Generate a host build rule for the flatbuffers compiler.
ifeq (Windows,$(PROJECT_OS))
# TODO(smiles): Need to find msbuild correctly in here.
define build_flatc_recipe
	cd $(PIE_NOON_DIR) & $(CMAKE) -G'Visual Studio 11 2012' . & msbuild
endef
endif
ifeq (Linux,$(PROJECT_OS))
define build_flatc_recipe
	cd $(PIE_NOON_DIR) && $(CMAKE) . && $(MAKE) flatc
endef
endif
ifeq (Darwin,$(PROJECT_OS))
define build_flatc_recipe
	cd $(PIE_NOON_DIR) && "$(CMAKE)" -GXcode . && xcodebuild -target flatc
endef
endif
ifeq (,$(build_flatc_recipe))
ifeq (,$(call find_flatc))
$(error flatc binary not found!)
endif
endif

# Generated includes directory (relative to PIE_NOON_DIR).
GENERATED_INCLUDES_PATH := gen/include
# Flatbuffers schemas used to generate includes.
FLATBUFFERS_SCHEMAS := $(wildcard $(PIE_NOON_DIR)/src/flatbufferschemas/*.fbs)

# Generate a build rule for flatc.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
ifeq ($(strip $(call find_flatc)),)
flatc_target:=build_flatc
.PHONY: $(flatc_target)
else
flatc_target:=$(call find_flatc)
endif
$(flatc_target):
	$(call build_flatc_recipe)
endif

# Convert the specified fbs path to a Flatbuffers generated header path.
# For example: src/flatbuffers/schemas/config.fbs will be converted to
# gen/include/config.h.
define flatbuffers_fbs_to_h
$(subst src/flatbufferschemas,$(GENERATED_INCLUDES_PATH),\
	$(patsubst %.fbs,%_generated.h,$(1)))
endef

# Generate a build rule that will convert a Flatbuffers schema to a generated
# header derived from the schema filename using flatbuffers_fbs_to_h.
define flatbuffers_header_build_rule
$(eval \
  $(call flatbuffers_fbs_to_h,$(1)): $(1) $(flatc_target)
	$(call host-echo-build-step,generic,Generate) \
		$(subst $(PIE_NOON_DIR)/,,$(call flatbuffers_fbs_to_h,$(1)))
	$(hide) $$(call find_flatc) --gen-includes -o $$(dir $$@) -c $$<)
endef

# Create the list of generated headers.
GENERATED_INCLUDES := \
	$(foreach schema,$(FLATBUFFERS_SCHEMAS),\
		$(call flatbuffers_fbs_to_h,$(schema)))

# Generate a build rule for each header.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
$(foreach schema,$(FLATBUFFERS_SCHEMAS),\
	$(call flatbuffers_header_build_rule,$(schema)))

.PHONY: generated_includes
generated_includes: $(GENERATED_INCLUDES)

clean_generated_includes:
	$(hide) $(host-rm) $(GENERATED_INCLUDES)
endif

# Build rule which builds assets for the game.
ifeq (,$(PROJECT_GLOBAL_BUILD_RULES_DEFINED))
.PHONY: build_assets
build_assets: $(flatc_target)
	$(hide) python $(PIE_NOON_DIR)/scripts/build_assets.py

.PHONY: clean_assets
clean_assets:
	$(hide) python $(PIE_NOON_DIR)/scripts/build_assets.py clean
endif
PROJECT_GLOBAL_BUILD_RULES_DEFINED:=1


include $(CLEAR_VARS)

LOCAL_MODULE := main
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := $(DEPENDENCIES_SDL_DIR) \
                    $(DEPENDENCIES_SDL_DIR)/include \
                    $(DEPENDENCIES_SDL_MIXER_DIR) \
                    $(DEPENDENCIES_FLATBUFFERS_DIR)/include \
                    $(DEPENDENCIES_GPG_DIR)/include \
                    $(DEPENDENCIES_WEBP_DIR)/src \
                    $(PIE_NOON_DIR)/$(GENERATED_INCLUDES_PATH) \
                    src

LOCAL_SRC_FILES := \
  $(DEPENDENCIES_SDL_DIR)/src/main/android/SDL_android_main.c \
  $(PIE_NOON_DIR)/src/ai_controller.cpp \
  $(PIE_NOON_DIR)/src/async_loader.cpp \
  $(PIE_NOON_DIR)/src/audio_engine.cpp \
  $(PIE_NOON_DIR)/src/bus.cpp \
  $(PIE_NOON_DIR)/src/character.cpp \
  $(PIE_NOON_DIR)/src/character_state_machine.cpp \
  $(PIE_NOON_DIR)/src/controller.cpp \
  $(PIE_NOON_DIR)/src/full_screen_fader.cpp \
  $(PIE_NOON_DIR)/src/gamepad_controller.cpp \
  $(PIE_NOON_DIR)/src/game_camera.cpp \
  $(PIE_NOON_DIR)/src/game_state.cpp \
  $(PIE_NOON_DIR)/src/gpg_manager.cpp \
  $(PIE_NOON_DIR)/src/gui_menu.cpp \
  $(PIE_NOON_DIR)/src/impel_engine.cpp \
  $(PIE_NOON_DIR)/src/impel_flatbuffers.cpp \
  $(PIE_NOON_DIR)/src/impel_processor_overshoot.cpp \
  $(PIE_NOON_DIR)/src/impel_processor_smooth.cpp \
  $(PIE_NOON_DIR)/src/input.cpp \
  $(PIE_NOON_DIR)/src/main.cpp \
  $(PIE_NOON_DIR)/src/material.cpp \
  $(PIE_NOON_DIR)/src/material_manager.cpp \
  $(PIE_NOON_DIR)/src/mesh.cpp \
  $(PIE_NOON_DIR)/src/player_controller.cpp \
  $(PIE_NOON_DIR)/src/particles.cpp \
  $(PIE_NOON_DIR)/src/precompiled.cpp \
  $(PIE_NOON_DIR)/src/renderer.cpp \
  $(PIE_NOON_DIR)/src/renderer_android.cpp \
  $(PIE_NOON_DIR)/src/shader.cpp \
  $(PIE_NOON_DIR)/src/sound.cpp \
  $(PIE_NOON_DIR)/src/sound_collection.cpp \
  $(PIE_NOON_DIR)/src/pie_noon_game.cpp \
  $(PIE_NOON_DIR)/src/touchscreen_button.cpp \
  $(PIE_NOON_DIR)/src/touchscreen_controller.cpp \
  $(PIE_NOON_DIR)/src/utilities.cpp

# Make each source file dependent upon the generated_includes and build_assets
# targets.
$(foreach src,$(LOCAL_SRC_FILES),$(eval $$(src): generated_includes))
$(foreach src,$(LOCAL_SRC_FILES),$(eval $$(src): build_assets))

clean: clean_assets clean_generated_includes

LOCAL_STATIC_LIBRARIES := libgpg libmathfu libwebp SDL2 SDL2_mixer

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(abspath $(DEPENDENCIES_MATHFU_DIR)/..))
$(call import-add-path,$(abspath $(DEPENDENCIES_WEBP_DIR)/..))

$(call import-module,mathfu/jni)
$(call import-module,webp)

