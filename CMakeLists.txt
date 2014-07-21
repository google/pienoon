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

set(FLATBUFFER_PROJECT_DIR ${CMAKE_SOURCE_DIR}/../../libs/flatbuffers)

set(FLATBUFFER_GENERERATED_INCLUDES
    ${CMAKE_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include/)
file(MAKE_DIRECTORY ${FLATBUFFER_GENERERATED_INCLUDES})

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${FLATBUFFER_PROJECT_DIR}/include)
include_directories(${FLATBUFFER_GENERERATED_INCLUDES})

file(GLOB_RECURSE FLATBUFFER_SCHEMAS ${CMAKE_SOURCE_DIR}/src *.fbs)
add_custom_target(
    generate_flatbuffers
    ${CMAKE_SOURCE_DIR}/flatbuffers/flatc -o ${FLATBUFFER_GENERERATED_INCLUDES}
        -c ${FLATBUFFER_SCHEMAS}
    SOURCES ${FLATBUFFER_SCHEMAS})

add_subdirectory(${FLATBUFFER_PROJECT_DIR} flatbuffers)

add_definitions(-std=c++11 -pedantic -Werror -Wall -Wextra)
add_executable(splat src/main.cpp)

add_dependencies(splat generate_flatbuffers)
add_dependencies(generate_flatbuffers flatc)

