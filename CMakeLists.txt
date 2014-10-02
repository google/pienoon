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
cmake_minimum_required(VERSION 2.8.12)

project(splat)

# Compile the game with the debug flag
set(SPLAT_DEBUG ON)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_BINARY_DIR}/obj)

# Directory which contains the source for 3rd party libraries.
get_filename_component(
    third_party_root "${CMAKE_SOURCE_DIR}/../../../../external" REALPATH)
# Directory which contains source for FPL libraries.
get_filename_component(
    fpl_root "${CMAKE_SOURCE_DIR}/../../libs" REALPATH)

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
set(dependencies_root "${CMAKE_SOURCE_DIR}/dependencies")
if(EXISTS "${dependencies_root}")
  set(third_party_root "${dependencies_root}")
  set(fpl_root "${dependencies_root}")
endif()

# Configurable locations of dependencies of this project.
set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_sdl_dir "${third_party_root}/sdl"
    CACHE PATH "Directory containing the SDL library.")
set(dependencies_libogg_cmake_dir "${CMAKE_MODULE_PATH}/libogg"
    CACHE PATH "Directory containing the libogg cmake project.")
set(dependencies_libogg_distr_dir "${third_party_root}/libogg"
    CACHE PATH "Directory containing the libogg library.")
set(dependencies_libvorbis_cmake_dir "${CMAKE_MODULE_PATH}/libvorbis"
    CACHE PATH "Directory containing the libvorbis cmake project.")
set(dependencies_libvorbis_distr_dir "${third_party_root}/libvorbis"
    CACHE PATH "Directory containing the libvorbis library.")
set(dependencies_sdl_mixer_cmake_dir "${CMAKE_MODULE_PATH}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer cmake project.")
set(dependencies_sdl_mixer_distr_dir "${third_party_root}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer library.")
set(dependencies_webp_cmake_dir "${CMAKE_MODULE_PATH}/webp"
    CACHE PATH "Directory containing the Webp cmake project.")
set(dependencies_webp_distr_dir "${third_party_root}/webp"
    CACHE PATH "Directory containing the Webp distribution.")

# Option to enable / disable the test build.
option(splat_build_tests "Build tests for this project." ON)

# Option to enable / disable the build of cwebp from source.
option(splat_build_cwebp "Build cwebp from source." OFF)

if(splat_build_cwebp)
  # Depend upon the cwebp target to build assets.
  set(CWEBP_TARGET cwebp)
  # Enable the build of cwebp from source.
  set(webp_build_cwebp ON CACHE BOOL "")
endif()

# Include MathFu in this project with test and benchmark builds disabled.
set(mathfu_build_benchmarks OFF CACHE BOOL "")
set(mathfu_build_tests OFF CACHE BOOL "")
add_subdirectory(${dependencies_mathfu_dir} ${tmp_dir}/mathfu)

# Include SDL in this project while working around SDL requirement of out of
# tree builds.
set(BIN_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/.)
if(APPLE)
  set(SDL_AUDIO_DRIVER_COREAUDIO 1)
endif(APPLE)
add_subdirectory(${dependencies_sdl_dir} ${tmp_dir}/sdl)
set(CMAKE_BINARY_DIR ${BIN_DIR})

# Include libogg.
add_subdirectory("${dependencies_libogg_cmake_dir}" ${tmp_dir}/libogg)

# Include libvorbis.
add_subdirectory("${dependencies_libvorbis_cmake_dir}" ${tmp_dir}/libvorbis)

# Include SDL Mixer.
set(SDL_MIXER_OGG_SUPPORT ON CACHE BOOL "Compile in Ogg Vorbis support")
add_subdirectory("${dependencies_sdl_mixer_cmake_dir}" ${tmp_dir}/sdl_mixer)

# Include Webp in this project, and force build in release mode for speed.
set(original_build_type ${CMAKE_BUILD_TYPE})
set(CMAKE_BUILD_TYPE Release)
add_subdirectory("${dependencies_webp_cmake_dir}" ${tmp_dir}/webp)
set(CMAKE_BUILD_TYPE ${original_build_type})

# Include FlatBuffers in this project.
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(FLATBUFFERS_GENERATED_INCLUDES_DIR
    ${CMAKE_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include)
file(MAKE_DIRECTORY ${FLATBUFFERS_GENERATED_INCLUDES_DIR})
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_SOURCE_DIR}/src *.fbs)

# Generate rules to build the set of output files from the set of input
# schema files.
foreach(flatbuffers_schema ${FLATBUFFERS_SCHEMAS})
  get_filename_component(filename ${flatbuffers_schema} NAME_WE)
  set(flatbuffers_include
      ${FLATBUFFERS_GENERATED_INCLUDES_DIR}/${filename}_generated.h)
  add_custom_command(
    OUTPUT ${flatbuffers_include}
    COMMAND flatc -H -o ${FLATBUFFERS_GENERATED_INCLUDES_DIR}
        -c ${flatbuffers_schema}
    DEPENDS flatc ${flatbuffers_schema})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

# Build rule that uses make to build the assets.
add_custom_target(assets
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/build_assets.py
  DEPENDS flatc ${CWEBP_TARGET})

if(APPLE)
  find_library(OPENGL_LIBRARIES NAMES GL GLU)
  SET(CMAKE_CXX_LINK_FLAGS "-framework OpenGL")
endif()

# Splat source files.
set(splat_SRCS
    src/ai_controller.h
    src/ai_controller.cpp
    src/angle.h
    src/audio_engine.h
    src/audio_engine.cpp
    src/character.h
    src/character.cpp
    src/character_state_machine.h
    src/character_state_machine.cpp
    src/common.h
    src/controller.h
    src/controller.cpp
    src/game_state.h
    src/game_state.cpp
    src/gamepad_controller.h
    src/gamepad_controller.cpp
    src/glplatform.h
    src/gpg_manager.h
    src/input.h
    src/input.cpp
    src/main.cpp
    src/material_manager.h
    src/material_manager.cpp
    src/player_controller.h
    src/player_controller.cpp
    src/shader.h
    src/shader.cpp
    src/mesh.h
    src/mesh.cpp
    src/material.h
    src/magnet_base.h
    src/magnet1f.cpp
    src/magnet1f.h
    src/material.cpp
    src/particles.h
    src/particles.cpp
    src/precompiled.h
    src/renderer.h
    src/renderer.cpp
    src/scene_description.h
    src/sound.h
    src/sound.cpp
    src/splat_game.h
    src/splat_game.cpp
    src/touchscreen_controller.h
    src/touchscreen_controller.cpp
    src/utilities.h
    src/utilities.cpp)

# Includes for this project.
include_directories(src)
if(WIN32)
  include_directories(external/include/windows)
endif()
include_directories(${dependencies_webp_distr_dir}/include)
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${dependencies_webp_distr_dir}/include)
include_directories(${dependencies_webp_distr_dir}/src)

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# libogg includes.
include_directories(${dependencies_libogg_distr_dir}/include)

# libvorbis includes.
include_directories(${dependencies_libvorbis_distr_dir}/include)

# SDL Mixer includes.
include_directories(${dependencies_sdl_mixer_distr_dir})

# Detect clang
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  set(C_FLAGS_WARNINGS "/W4 /WX")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra -Wno-unused-function")
  # -Wno-unused-function is because of webp encode.h
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

if(SPLAT_DEBUG)
  add_definitions(-D_DEBUG)
endif()

set (SDL_LIBRARIES SDL2-static)
if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  set(OPENGL_LIBRARIES opengl32)
  set(SDL_LIBRARIES SDL2main ${SDL_LIBRARIES})
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

# Executable target.
add_executable(splat ${splat_SRCS})
# Additional flags for the target.
mathfu_configure_flags(splat)
# Dependencies for the executable target.
add_dependencies(splat generated_includes assets)
target_link_libraries(splat
  ${SDL_LIBRARIES}
  sdl_mixer
  libvorbis
  libogg
  ${OPENGL_LIBRARIES}
  webp)

# Tests.
if(splat_build_tests)
  add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif()

# Create a zipped tar of all the necessary files to run the game.
add_custom_target(export
  COMMAND python ${CMAKE_SOURCE_DIR}/scripts/export.py
  DEPENDS splat)

