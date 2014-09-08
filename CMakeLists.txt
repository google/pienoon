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
set(dependencies_sdl_mixer_dir "${third_party_root}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer library.")

# Include MathFu in this project.
add_subdirectory(${dependencies_mathfu_dir} mathfu)

# Include SDL in this project while working around SDL requirement of out of
# tree builds.
set(BIN_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/.)
if(APPLE)
  set(SDL_AUDIO_DRIVER_COREAUDIO 1)
endif(APPLE)
add_subdirectory(${dependencies_sdl_dir} sdl)
set(CMAKE_BINARY_DIR ${BIN_DIR})

# Include SDL Mixer. This must come after SDL is included.
add_subdirectory("${dependencies_sdl_mixer_dir}" sdl_mixer)

# Include FlatBuffers in this project.
add_subdirectory("${dependencies_flatbuffers_dir}" flatbuffers)

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(FLATBUFFERS_GENERATED_INCLUDES_DIR
    ${CMAKE_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include)
file(MAKE_DIRECTORY ${FLATBUFFERS_GENERATED_INCLUDES_DIR})
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_SOURCE_DIR}/src *.fbs)
get_target_property(FLATBUFFERS_FLATC flatc LOCATION)

# Generate rules to build the set of output files from the set of input
# schema files.
foreach(flatbuffers_schema ${FLATBUFFERS_SCHEMAS})
  get_filename_component(filename ${flatbuffers_schema} NAME_WE)
  set(flatbuffers_include
      ${FLATBUFFERS_GENERATED_INCLUDES_DIR}/${filename}_generated.h)
  add_custom_command(
    OUTPUT ${flatbuffers_include}
    COMMAND ${FLATBUFFERS_FLATC} -o ${FLATBUFFERS_GENERATED_INCLUDES_DIR}
        -c ${flatbuffers_schema}
    DEPENDS flatc ${flatbuffers_schema} ${FLATBUFFERS_FLATC}
        ${FLATBUFFERS_GENERATED_INCLUDES_DIR})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

# Build rule that uses make to build the assets.
add_custom_target(assets
  COMMAND make -f ${CMAKE_SOURCE_DIR}/scripts/build_assets.mk
  DEPENDS flatc)

if(APPLE)
  find_library(OPENGL_LIBRARIES NAMES GL GLU)
  SET(CMAKE_CXX_LINK_FLAGS "-framework OpenGL")
endif()

# Splat source files.
set(splat_SRCS
    src/ai_controller.cpp
    src/audio_engine.cpp
    src/character.cpp
    src/character_state_machine.cpp
    src/game_state.cpp
    src/input.cpp
    src/main.cpp
    src/material_manager.cpp
    src/player_controller.cpp
    src/shader.h
    src/shader.cpp
    src/mesh.h
    src/mesh.cpp
    src/material.h
    src/material.cpp
    src/renderer.h
    src/renderer.cpp
    src/sound.cpp
    src/splat_game.cpp
    src/utilities.cpp)

# Includes for this project.
include_directories(src)
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# SDL Mixer includes.
include_directories(${sdl_mixer_SOURCE_DIR})

# Executable target.
add_definitions(-g -std=c++0x -pedantic -Werror -Wall -Wextra)
if(SPLAT_DEBUG)
  add_definitions(-D_DEBUG)
endif()

add_executable(splat ${splat_SRCS})
# Additional flags for the target.
mathfu_configure_flags(splat)
# Dependencies for the executable target.
add_dependencies(splat generated_includes assets)
target_link_libraries(splat
  SDL2-static
  sdl_mixer
  ${OPENGL_LIBRARIES})

# Tests.
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

get_target_property(SPLAT_EXECUTABLE splat LOCATION)

# Convert executable paths to relative paths.
# NOTE: This only works if the executables are under the project directory
# (default location).
string(REPLACE "${CMAKE_SOURCE_DIR}/" "" SPLAT_EXECUTABLE_RELATIVE
       "${SPLAT_EXECUTABLE}")
string(REPLACE "${CMAKE_SOURCE_DIR}/" "" FLATBUFFERS_FLATC_RELATIVE
       "${FLATBUFFERS_FLATC}")

# The set of files and folders to export for non-programmers.
# NOTE: Paths should be absolute paths under the project directory.
set(EXPORT_FILES
    assets
    run.command
    scripts/build_assets.mk
    src/flatbufferschemas
    src/rawassets
    ${SPLAT_EXECUTABLE_RELATIVE}
    ${FLATBUFFERS_FLATC_RELATIVE})

# Create a zipped tar of all the necessary files to run the game.
add_custom_target(export
  COMMAND cd ${CMAKE_SOURCE_DIR} && tar -czvf splat.tar.gz ${EXPORT_FILES}
  DEPENDS splat
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)

