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

project(sdl_mixer)

set(sdl_mixer_dir ${dependencies_sdl_mixer_distr_dir})

if (${SDL_MIXER_OGG_SUPPORT})
  add_definitions("-DOGG_MUSIC")

  # Include libogg header files.
  include_directories(${dependencies_libogg_distr_dir}/include)

  # Include libvorbis header files.
  include_directories(${dependencies_libvorbis_distr_dir}/include)
endif()

# Include SDL header files.
include_directories(${dependencies_sdl_dir}/include)

include_directories(${sdl_mixer_dir})

set(sdl_mixer_SRCS
    ${sdl_mixer_dir}/dynamic_flac.c
    ${sdl_mixer_dir}/dynamic_fluidsynth.c
    ${sdl_mixer_dir}/dynamic_mod.c
    ${sdl_mixer_dir}/dynamic_modplug.c
    ${sdl_mixer_dir}/dynamic_ogg.c
    ${sdl_mixer_dir}/effect_position.c
    ${sdl_mixer_dir}/effects_internal.c
    ${sdl_mixer_dir}/effect_stereoreverse.c
    ${sdl_mixer_dir}/fluidsynth.c
    ${sdl_mixer_dir}/load_aiff.c
    ${sdl_mixer_dir}/load_flac.c
    ${sdl_mixer_dir}/load_ogg.c
    ${sdl_mixer_dir}/load_voc.c
    ${sdl_mixer_dir}/mixer.c
    ${sdl_mixer_dir}/music.c
    ${sdl_mixer_dir}/music_cmd.c
    ${sdl_mixer_dir}/music_flac.c
    ${sdl_mixer_dir}/music_mad.c
    ${sdl_mixer_dir}/music_mod.c
    ${sdl_mixer_dir}/music_modplug.c
    ${sdl_mixer_dir}/music_ogg.c
    ${sdl_mixer_dir}/wavestream.c)

add_library(sdl_mixer ${sdl_mixer_SRCS})

