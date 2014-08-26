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

include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

set(SDL_MIXER_SRCS
    dynamic_flac.c
    dynamic_fluidsynth.c
    dynamic_mod.c
    dynamic_modplug.c
    dynamic_ogg.c
    effect_position.c
    effects_internal.c
    effect_stereoreverse.c
    fluidsynth.c
    load_aiff.c
    load_flac.c
    load_ogg.c
    load_voc.c
    mixer.c
    music.c
    music_cmd.c
    music_flac.c
    music_mad.c
    music_mod.c
    music_modplug.c
    music_ogg.c
    wavestream.c)

add_library(sdl_mixer ${SDL_MIXER_SRCS})
