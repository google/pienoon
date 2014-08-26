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

# This makefile builds all assets under src/rawassets/ for the project, writing
# the results to assets/.

# Path of this project.
TOP:=$(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)

# Directory that contains the FlatBuffers compiler.
FLATBUFFERS_PATH?=$(realpath $(TOP)/flatbuffers)

# Location of FlatBuffers compiler.
FLATC?=$(realpath $(firstword \
            $(wildcard $(FLATBUFFERS_PATH)/flatc*) \
            $(wildcard $(FLATBUFFERS_PATH)/Release/flatc*) \
            $(wildcard $(FLATBUFFERS_PATH)/Debug/flatc*)))

# If the FlatBuffers compiler is not specified, just assume it's in the PATH.
ifeq ($(FLATC),)
FLATC:=flatc
endif

# Convert specified json paths to FlatBuffers binary data output paths.
# For example: src/rawassets/materials/splatter1.json will be converted to
# assets/materials/splatter1.bin.
define flatbuffers_json_to_binary
$(subst $(TOP)/src/rawassets,assets,$(patsubst %.json,%.bin,$(1)))
endef

# Convert specified json path to the associated FlatBuffers schema path.
# For example: src/rawassets/config.json will be converted to
# src/flatbufferschemas/config.fbs.
define flatbuffers_json_to_fbs
$(subst rawassets/,flatbufferschemas/,$(patsubst %.json,%.fbs,$(1)))
endef

# Generate a build rule that will convert a json FlatBuffer to a binary
# FlatBuffer using a schema derived from the source json filename.
define flatbuffers_single_schema_build_rule
$(eval \
  $(call flatbuffers_json_to_binary,$(1)): $(1)
	$(FLATC) -o $$(dir $$@) -b $$(call flatbuffers_json_to_fbs,$$<) $$<)
endef

# Generate a build rule that will convert a json FlatBuffer to a binary
# FlatBuffer using the materials schema.
define flatbuffers_material_build_rule
$(eval \
  $(call flatbuffers_json_to_binary,$(1)): $(1)
	$(FLATC) -o $$(dir $$@) -b $(TOP)/src/flatbufferschemas/materials.fbs $$<)
endef

# json describing FlatBuffers data that will be converted to FlatBuffers binary
# files.
flatbuffers_single_schema_json:=\
	$(TOP)/src/rawassets/config.json \
	$(TOP)/src/rawassets/character_state_machine_def.json \
	$(TOP)/src/rawassets/splat_rendering_assets.json

# json describing FlatBuffers material data (using the material.fbs schema)
# that will be converted to FlatBuffers binary files.
flatbuffers_material_json:=\
	$(wildcard $(TOP)/src/rawassets/materials/*.json)

# All binary FlatBuffers that should be built.
flatbuffers_binary=\
	$(call flatbuffers_json_to_binary,\
		$(flatbuffers_single_schema_json) \
		$(flatbuffers_material_json))

# Top level build rule for all assets.
all: $(flatbuffers_binary)

# Create a build rule for each FlatBuffer binary file that will be built from
# a .json files using a schema that is derived form the source json filename.
$(foreach flatbuffers_json_file,$(flatbuffers_single_schema_json),\
	$(call flatbuffers_single_schema_build_rule,$(flatbuffers_json_file)))

# Create build rules for each FlatBuffer binary file that will be
# built from a .json material file.
$(foreach flatbuffers_json_file,$(flatbuffers_material_json),\
	$(call flatbuffers_material_build_rule,$(flatbuffers_json_file)))
