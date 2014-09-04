LOCAL_PATH := $(call my-dir)

# relative to this file
SPLAT_PATH := $(realpath $(LOCAL_PATH)/../..)

# relative to project root
SDL_PATH := ../../../../external/sdl
MIXER_PATH := ../../../../external/sdl_mixer
FLATBUFFERS_PATH := ../../libs/flatbuffers
MATHFU_PATH := ../../libs/mathfu
GPG_PATH := ../../../../prebuilts/gpg-cpp-sdk/android

# The following block generates build rules which result in headers being
# rebuilt from flatbuffers schemas.

# Directory that contains the FlatBuffers compiler.
FLATBUFFERS_FLATC_PATH?=$(realpath $(SPLAT_PATH)/flatbuffers)

# Location of FlatBuffers compiler.
FLATC?=$(realpath $(firstword \
            $(wildcard $(FLATBUFFERS_FLATC_PATH)/flatc*) \
            $(wildcard $(FLATBUFFERS_FLATC_PATH)/Release/flatc*) \
            $(wildcard $(FLATBUFFERS_FLATC_PATH)/Debug/flatc*)))
ifeq (,$(wildcard $(FLATC)))
$(error flatc binary not found!)
endif

# Generated includes directory (relative to SPLAT_PATH).
GENERATED_INCLUDES_PATH := gen/include
# Flatbuffers schemas used to generate includes.
FLATBUFFERS_SCHEMAS := $(wildcard $(SPLAT_PATH)/src/flatbufferschemas/*.fbs)

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
  $(call flatbuffers_fbs_to_h,$(1)): $(1)
	$(call host-echo-build-step,generic,Generate) \
		$(subst $(SPLAT_PATH)/,,$(call flatbuffers_fbs_to_h,$(1)))
	$(hide) $(FLATC) -o $$(dir $$@) -c $$<)
endef

# Create the list of generated headers.
GENERATED_INCLUDES := \
	$(foreach schema,$(FLATBUFFERS_SCHEMAS),\
		$(call flatbuffers_fbs_to_h,$(schema)))

# Generate a build rule for each header.
$(foreach schema,$(FLATBUFFERS_SCHEMAS),\
	$(call flatbuffers_header_build_rule,$(schema)))

# Build includes as a side effect of this empty library.
.PHONY: generated_includes

generated_includes: $(GENERATED_INCLUDES)


include $(CLEAR_VARS)
LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
                    $(MIXER_PATH) \
                    $(FLATBUFFERS_PATH)/include \
                    $(MATHFU_PATH)/include \
                    $(GENERATED_INCLUDES_PATH) \
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
	$(SPLAT_PATH)/src/utilities.cpp \
	$(SPLAT_PATH)/src/gpg_manager.cpp \
	$(SPLAT_PATH)/src/audio_engine.cpp \
	$(SPLAT_PATH)/src/sound.cpp

# Make each source file dependent upon the generated_includes target.
$(foreach src,$(LOCAL_SRC_FILES),$(eval $$(src): generated_includes))

LOCAL_STATIC_LIBRARIES := libgpg

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz

include $(BUILD_SHARED_LIBRARY)

