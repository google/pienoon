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

if(APPLE)
  find_library(OPENGL_LIBRARIES NAMES GL GLU)
endif()

# Splat source files.
set(splat_SRCS
    src/main.cpp
    src/precompiled.cpp
    src/renderer.cpp
    src/input.cpp
    src/material_manager.cpp
    src/character_state_machine.cpp)

# Includes for this project.
include_directories(src)
include_directories(${FLATBUFFERS_PROJECT_DIR}/include)
include_directories(${FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${MATHFU_PROJECT_DIR})

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# Executable target.
add_definitions(-g -std=c++0x -pedantic -Werror -Wall -Wextra
                -DCOMPILE_WITHOUT_SIMD_SUPPORT)
add_executable(splat ${splat_SRCS})
# Dependencies for the executable target.
add_dependencies(splat generated_includes)
target_link_libraries(splat
  SDL2-static
  ${OPENGL_LIBRARIES})

# Include GoogleTest in this project.
add_subdirectory(${GOOGLETEST_PROJECT_DIR} googletest)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
