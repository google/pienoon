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

project(libogg)

set(libogg_dir ${dependencies_libogg_distr_dir})

include_directories(${libogg_dir}/include)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux"
    AND NOT EXISTS "${libogg_dir}/include/ogg/config_types.h")
    # Run libogg's autogen.sh to create ogg/config_types.h, needed under Linux.
    message(STATUS "Running libogg's autogen.sh to create ogg/config_types.h.")
    execute_process(COMMAND ${libogg_dir}/autogen.sh
                    WORKING_DIRECTORY ${libogg_dir}
                    RESULT_VARIABLE autogen_result)
    if (NOT ${autogen_result} EQUAL 0)
        message(FATAL_ERROR
          "CMake encountered an error running libogg's autogen.sh.\n"
          "You must have autoconf, automake, and libtool installed to "
          "build on Linux.")
    endif()
endif()

set(libogg_SRCS
    ${libogg_dir}/src/bitwise.c
    ${libogg_dir}/src/framing.c)

add_library(libogg ${libogg_SRCS})

