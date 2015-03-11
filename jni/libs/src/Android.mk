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
PIE_NOON_RELATIVE_DIR:=../../..
PIE_NOON_DIR:=$(LOCAL_PATH)/$(PIE_NOON_RELATIVE_DIR)
include $(PIE_NOON_DIR)/jni/android_config.mk
include $(DEPENDENCIES_FLATBUFFERS_DIR)/android/jni/include.mk

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

PIE_NOON_GENERATED_OUTPUT_DIR := $(PIE_NOON_DIR)/gen/include

LOCAL_C_INCLUDES := \
  $(DEPENDENCIES_SDL_DIR) \
  $(DEPENDENCIES_SDL_DIR)/include \
  $(DEPENDENCIES_SDL_MIXER_DIR) \
  $(DEPENDENCIES_FPLUTIL_DIR)/libfplutil/include \
  $(DEPENDENCIES_FREETYPE_DIR)/include \
  $(DEPENDENCIES_HARFBUZZ_DIR)/src \
  $(DEPENDENCIES_GPG_DIR)/include \
  $(DEPENDENCIES_WEBP_DIR)/src \
  ${PIE_NOON_DIR}/external/include/harfbuzz \
  $(PIE_NOON_GENERATED_OUTPUT_DIR) \
  src

LOCAL_SRC_FILES := \
  $(subst $(LOCAL_PATH)/,,$(DEPENDENCIES_SDL_DIR))/src/main/android/SDL_android_main.c \
  $(PIE_NOON_RELATIVE_DIR)/src/ai_controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/analytics_tracking.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/async_loader.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/cardboard_controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/character.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/character_state_machine.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/components/drip_and_vanish.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/components/player_character.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/components/scene_object.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/components/shakeable_prop.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/entity/entity_manager.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/font_manager.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/full_screen_fader.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/gamepad_controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/game_camera.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/game_state.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/gpg_manager.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/gpg_multiplayer.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/gui_menu.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/imgui.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/input.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/main.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/material.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/material_manager.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/mesh.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/player_controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/particles.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/precompiled.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/renderer.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/renderer_android.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/shader.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/pie_noon_game.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/touchscreen_button.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/touchscreen_controller.cpp \
  $(PIE_NOON_RELATIVE_DIR)/src/utilities.cpp

PIE_NOON_SCHEMA_DIR := $(PIE_NOON_DIR)/src/flatbufferschemas

PIE_NOON_SCHEMA_FILES := \
  $(PIE_NOON_SCHEMA_DIR)/character_state_machine_def.fbs \
  $(PIE_NOON_SCHEMA_DIR)/config.fbs \
  $(PIE_NOON_SCHEMA_DIR)/components.fbs \
  $(PIE_NOON_SCHEMA_DIR)/materials.fbs \
  $(PIE_NOON_SCHEMA_DIR)/particles.fbs \
  $(PIE_NOON_SCHEMA_DIR)/pie_noon_common.fbs \
  $(PIE_NOON_SCHEMA_DIR)/scoring_rules.fbs \
  $(PIE_NOON_SCHEMA_DIR)/timeline.fbs

# Make each source file dependent upon the assets
$(foreach src,$(LOCAL_SRC_FILES),$(eval $(LOCAL_PATH)/$$(src): build_assets))

ifeq (,$(PIE_NOON_RUN_ONCE))
PIE_NOON_RUN_ONCE := 1
$(call flatbuffers_header_build_rules,\
  $(PIE_NOON_SCHEMA_FILES),\
  $(PIE_NOON_SCHEMA_DIR),\
  $(PIE_NOON_GENERATED_OUTPUT_DIR),\
  $(DEPENDENCIES_PINDROP_DIR)/schemas $(DEPENDENCIES_MOTIVE_DIR)/schemas,\
  $(LOCAL_SRC_FILES))
endif

clean: clean_assets clean_generated_includes

LOCAL_STATIC_LIBRARIES := \
  libgpg \
  libmathfu \
  libwebp \
  SDL2 \
  SDL2_mixer \
  libpindrop \
  libmotive \
  libfreetype \
  libharfbuzz \
  libflatbuffers

LOCAL_SHARED_LIBRARIES :=

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz -lEGL -landroid

include $(BUILD_SHARED_LIBRARY)

$(call import-add-path,$(DEPENDENCIES_FLATBUFFERS_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MATHFU_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_MOTIVE_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_PINDROP_DIR)/..)
$(call import-add-path,$(DEPENDENCIES_WEBP_DIR)/..)

$(call import-module,flatbuffers/android/jni)
$(call import-module,audio_engine/jni)
$(call import-module,motive/jni)
$(call import-module,mathfu/jni)
$(call import-module,webp)

