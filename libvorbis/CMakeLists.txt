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
cmake_minimum_required(VERSION 2.8.7)

project(libvorbis)

set(libvorbis_dir ${dependencies_libvorbis_distr_dir})

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
endif()

# Include libogg header files.
include_directories(${dependencies_libogg_distr_dir}/include)

# Include libvorbis header files.
include_directories(${libvorbis_dir}/include)
include_directories(${libvorbis_dir}/lib)

set(libvorbis_SRCS
    ${libvorbis_dir}/lib/analysis.c
    ${libvorbis_dir}/lib/barkmel.c
    ${libvorbis_dir}/lib/bitrate.c
    ${libvorbis_dir}/lib/block.c
    ${libvorbis_dir}/lib/codebook.c
    ${libvorbis_dir}/lib/envelope.c
    ${libvorbis_dir}/lib/floor0.c
    ${libvorbis_dir}/lib/floor1.c
    ${libvorbis_dir}/lib/info.c
    ${libvorbis_dir}/lib/lookup.c
    ${libvorbis_dir}/lib/lpc.c
    ${libvorbis_dir}/lib/lsp.c
    ${libvorbis_dir}/lib/mapping0.c
    ${libvorbis_dir}/lib/mdct.c
    ${libvorbis_dir}/lib/psy.c
    ${libvorbis_dir}/lib/registry.c
    ${libvorbis_dir}/lib/res0.c
    ${libvorbis_dir}/lib/sharedbook.c
    ${libvorbis_dir}/lib/smallft.c
    ${libvorbis_dir}/lib/synthesis.c
    ${libvorbis_dir}/lib/vorbisenc.c
    ${libvorbis_dir}/lib/vorbisfile.c
    ${libvorbis_dir}/lib/window.c)
    # ${libvorbis_dir}/lib/tone.c

add_library(libvorbis ${libvorbis_SRCS})

