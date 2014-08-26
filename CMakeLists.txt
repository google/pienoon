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

project(splat)

# Compile the game with the debug flag
set(SPLAT_DEBUG ON)

# Directory which contains the source for 3rd party libraries.
set(THIRD_PARTY_ROOT ${CMAKE_SOURCE_DIR}/../../../../external)
# Directory which contains source for FPL libraries.
set(FPL_ROOT ${CMAKE_SOURCE_DIR}/../../libs)

set(GOOGLETEST_PROJECT_DIR ${FPL_ROOT}/googletest)
set(FLATBUFFERS_PROJECT_DIR ${FPL_ROOT}/flatbuffers)
set(MATHFU_PROJECT_DIR ${FPL_ROOT}/mathfu)

# Include SDL in this project while working around SDL requirement of out of
# tree builds.
set(BIN_DIR ${CMAKE_BINARY_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}/.)
add_subdirectory(${THIRD_PARTY_ROOT}/sdl sdl)
set(CMAKE_BINARY_DIR ${BIN_DIR})

# Include FlatBuffers in this project.
add_subdirectory(${FLATBUFFERS_PROJECT_DIR} flatbuffers)

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
    DEPENDS ${flatbuffers_schema} ${FLATBUFFERS_FLATC}
        ${FLATBUFFERS_GENERATED_INCLUDES_DIR})
  list(APPEND FLATBUFFERS_GENERATED_INCLUDES ${flatbuffers_include})
endforeach()
add_custom_target(generated_includes DEPENDS ${FLATBUFFERS_GENERATED_INCLUDES})

# Build rule that uses make to build the assets.
add_custom_target(assets
  COMMAND make -f scripts/build_assets.mk
  DEPENDS ${FLATBUFFERS_FLATC})

if(APPLE)
  find_library(OPENGL_LIBRARIES NAMES GL GLU)
  SET(CMAKE_CXX_LINK_FLAGS "-framework OpenGL")
endif()

# Splat source files.
set(splat_SRCS
    src/character.cpp
    src/character_state_machine.cpp
    src/game_state.cpp
    src/input.cpp
    src/main.cpp
    src/material_manager.cpp
    src/renderer.cpp
    src/sdl_controller.cpp
    src/splat_game.cpp
    src/utilities.cpp)

# Includes for this project.
include_directories(src)
include_directories(${FLATBUFFERS_PROJECT_DIR}/include)
include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${MATHFU_PROJECT_DIR}/include)

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# Executable target.
add_definitions(-g -std=c++0x -pedantic -Werror -Wall -Wextra
                -DCOMPILE_WITHOUT_SIMD_SUPPORT)
if(SPLAT_DEBUG)
  add_definitions(-D_DEBUG)
endif()

add_executable(splat ${splat_SRCS})
# Dependencies for the executable target.
add_dependencies(splat generated_includes assets)
target_link_libraries(splat
  SDL2-static
  ${OPENGL_LIBRARIES})

# Include GoogleTest in this project.
add_subdirectory(${GOOGLETEST_PROJECT_DIR} googletest)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

get_target_property(SPLAT_EXECUTABLE splat LOCATION)

# The set of files and folders to export for non-programmers.
set(EXPORT_FILES
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_SOURCE_DIR}/run.command
    ${CMAKE_SOURCE_DIR}/scripts/jsontobin.sh
    ${CMAKE_SOURCE_DIR}/src/flatbufferschemas
    ${CMAKE_SOURCE_DIR}/src/rawassets
    ${SPLAT_EXECUTABLE}
    ${FLATBUFFERS_FLATC})

# Create a zipped tar of all the necessary files to run the game
add_custom_target(export
  COMMAND tar -czvf ${CMAKE_SOURCE_DIR}/splat.tar.gz ${EXPORT_FILES}
  DEPENDS splat
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)

