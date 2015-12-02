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

if(fpl_ios)
  # seamless toggle between device and simulator
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
  add_definitions(-D__IOS__)
  cmake_minimum_required(VERSION 3.3.1)
else()
  cmake_minimum_required(VERSION 2.8.12)
endif()

project(pie_noon)

# Compile the game with the debug flag
set(PIE_NOON_DEBUG ON)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Temporary files (like object files) created while compiling projects.
set(tmp_dir ${CMAKE_BINARY_DIR}/obj)

# Directory which contains the source for 3rd party libraries.
get_filename_component(
    third_party_root "${CMAKE_CURRENT_LIST_DIR}/../../../../external" REALPATH)
# Directory which contains source for FPL libraries.
get_filename_component(
    fpl_root "${CMAKE_CURRENT_LIST_DIR}/../../libs" REALPATH)

# If the dependencies directory exists, assume this is the root directory for
# all libraries required by this project.
set(dependencies_root "${CMAKE_CURRENT_SOURCE_DIR}/dependencies")
if(EXISTS "${dependencies_root}")
  set(third_party_root "${dependencies_root}")
  set(fpl_root "${dependencies_root}")
endif()

# Configurable locations of dependencies of this project.
set(dependencies_gtest_dir "${fpl_root}/googletest"
    CACHE PATH "Directory containing the GoogleTest library.")
set(dependencies_flatbuffers_dir "${fpl_root}/flatbuffers"
    CACHE PATH "Directory containing the Flatbuffers library.")
set(dependencies_fplbase_dir "${fpl_root}/fplbase"
    CACHE PATH "Directory containing the FplBase library.")
set(dependencies_corgi_dir "${fpl_root}/corgi"
    CACHE PATH "Directory containing the CORGI library.")
set(dependencies_flatui_dir "${fpl_root}/flatui"
    CACHE PATH "Directory containing the FlatUI library.")
set(dependencies_fplutil_dir "${fpl_root}/fplutil"
    CACHE PATH "Directory containing the fplutil library.")
set(dependencies_pindrop_dir "${fpl_root}/pindrop"
    CACHE PATH "Directory containing the pindrop library.")
set(dependencies_mathfu_dir "${fpl_root}/mathfu"
    CACHE PATH "Directory containing the MathFu library.")
set(dependencies_sdl_dir "${third_party_root}/sdl"
    CACHE PATH "Directory containing the SDL library.")
set(dependencies_libogg_distr_dir "${third_party_root}/libogg"
    CACHE PATH "Directory containing the libogg library.")
set(dependencies_libvorbis_distr_dir "${third_party_root}/libvorbis"
    CACHE PATH "Directory containing the libvorbis library.")
set(dependencies_motive_dir "${fpl_root}/motive"
    CACHE PATH "Directory containing the motive animation library.")
set(dependencies_sdl_mixer_distr_dir "${third_party_root}/sdl_mixer"
    CACHE PATH "Directory containing the SDL Mixer library.")

# Option to enable / disable the test build.
option(pie_noon_build_tests "Build tests for this project." ON)

# Include MathFu in this project with test and benchmark builds disabled.
set(mathfu_build_benchmarks OFF CACHE BOOL "")
set(mathfu_build_tests OFF CACHE BOOL "")
add_subdirectory(${dependencies_mathfu_dir} ${tmp_dir}/mathfu)

# Include pindrop.
if(NOT TARGET pindrop)
  set(pindrop_build_demo OFF CACHE BOOL "")
  set(pindrop_build_tests OFF CACHE BOOL "")
  add_subdirectory("${dependencies_pindrop_dir}" ${tmp_dir}/pindrop)
endif()

# Include motive.
if(NOT TARGET motive)
  set(motive_build_samples OFF CACHE BOOL "")
  set(motive_build_tests OFF CACHE BOOL "")
  add_subdirectory("${dependencies_motive_dir}" ${tmp_dir}/motive)
endif()

# Include FlatBuffers in this project.
set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "")
if(NOT TARGET flatc)
  add_subdirectory("${dependencies_flatbuffers_dir}" ${tmp_dir}/flatbuffers)
endif()

# Include FPLBase in this project.
if(NOT TARGET fplbase)
  set(fplbase_build_mesh_pipeline OFF CACHE BOOL "")
  set(fplbase_build_samples OFF CACHE BOOL "")
  add_subdirectory("${dependencies_fplbase_dir}" ${tmp_dir}/fplbase)
endif()

# Include entities.
if(NOT TARGET corgi)
  add_subdirectory("${dependencies_corgi_dir}" ${tmp_dir}/corgi)
endif()

# Include FlatUI in this project.
if(NOT TARGET flatui)
  add_subdirectory("${dependencies_flatui_dir}" ${tmp_dir}/flatui)
endif()

# Generate source files for all FlatBuffers schema files under the src
# directory.
set(FLATBUFFERS_GENERATED_INCLUDES_DIR
    ${CMAKE_SOURCE_DIR}${CMAKE_FILES_DIRECTORY}/include)
file(GLOB_RECURSE FLATBUFFERS_SCHEMAS ${CMAKE_CURRENT_LIST_DIR}/src/*.fbs)

set(flatbuffers_schema_includes
    ${dependencies_motive_dir}/schemas
    ${dependencies_fplbase_dir}/schemas
    ${dependencies_pindrop_dir}/schemas)

set (flatbuffers_additional_dependencies
     motive_generated_includes
     fplbase_generated_includes
     pindrop_generated_includes)
# Generate rules to build the set of output files from the set of input
# schema files.
build_flatbuffers("${FLATBUFFERS_SCHEMAS}"
                  "${flatbuffers_schema_includes}"
                  "generated_includes"
                  "${flatbuffers_additional_dependencies}"
                  "${FLATBUFFERS_GENERATED_INCLUDES_DIR}"
                  ""
                  "")

# Build rule that uses make to build the assets.
add_custom_target(assets
  COMMAND python ${CMAKE_CURRENT_LIST_DIR}/scripts/build_assets.py
              --flatc $<TARGET_FILE:flatc> --output ${CMAKE_BINARY_DIR}/assets
  DEPENDS flatc ${CWEBP_TARGET})

# Set iOS specific attributes
# This is required for the output dependencies for generated
# includes (e.g. motive pindrop) to end up the right directory
# This is necessary because iOS adds $(EFFECTIVE_PLATFORM_NAME)
# viz. -iphoneos and -iphonesimulator in front of the configuration name
# e.g. Debug-iphoneos instead of Debug
mathfu_set_ios_attributes(generated_includes)

# Set iOS specific attributes in for dependent libs
mathfu_set_ios_attributes(flatbuffers)

if(APPLE AND fpl_ios)
  set(CMAKE_CXX_LINK_FLAGS "-lSDL2 -lSDL2_mixer -framework OpenGLES
      -framework CoreGraphics -framework Foundation
      -framework CoreLocation -framework CoreFoundation
      -framework QuartzCore -framework UIKit
      -framework AudioToolbox -framework GLKit")
endif()



# PieNoon source files.
set(pie_noon_SRCS
    src/ai_controller.cpp
    src/ai_controller.h
    src/analytics_tracking.cpp
    src/analytics_tracking.h
    src/cardboard_controller.cpp
    src/cardboard_controller.h
    src/character.cpp
    src/character.h
    src/character_state_machine.cpp
    src/character_state_machine.h
    src/common.h
    src/controller.cpp
    src/controller.h
    src/components/cardboard_player.cpp
    src/components/cardboard_player.h
    src/components/drip_and_vanish.cpp
    src/components/drip_and_vanish.h
    src/components/player_character.cpp
    src/components/player_character.h
    src/components/scene_object.cpp
    src/components/scene_object.h
    src/components/shakeable_prop.cpp
    src/components/shakeable_prop.h
    src/full_screen_fader.cpp
    src/full_screen_fader.h
    src/game_camera.cpp
    src/game_camera.h
    src/game_state.cpp
    src/game_state.h
    src/gpg_manager.h
    src/gpg_multiplayer.h
    src/gui_menu.cpp
    src/gui_menu.h
    src/main.cpp
    src/multiplayer_controller.cpp
    src/multiplayer_controller.h
    src/multiplayer_director.cpp
    src/multiplayer_director.h
    src/player_controller.cpp
    src/player_controller.h
    src/main.cpp
    src/particles.cpp
    src/particles.h
    src/player_controller.cpp
    src/player_controller.h
    src/precompiled.h
    src/scene_description.h
    src/pie_noon_game.cpp
    src/pie_noon_game.h
    src/touchscreen_button.h
    src/touchscreen_button.cpp
    src/touchscreen_controller.cpp
    src/touchscreen_controller.h)

# Includes for this project.
include_directories(src)
if(WIN32)
  include_directories(external/include/windows)
endif()
get_property(FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET fplbase_generated_includes
             PROPERTY GENERATED_INCLUDES_DIR)
include_directories(${FPLBASE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET motive_generated_includes
             PROPERTY GENERATED_INCLUDES_DIR)
include_directories(${MOTIVE_FLATBUFFERS_GENERATED_INCLUDES_DIR})
get_property(PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR
             TARGET pindrop_generated_includes
             PROPERTY GENERATED_INCLUDES_DIR)
include_directories(${PINDROP_FLATBUFFERS_GENERATED_INCLUDES_DIR})
include_directories(${dependencies_flatbuffers_dir}/include)
include_directories(${dependencies_fplutil_dir}/libfplutil/include)
include_directories(${dependencies_fplbase_dir}/include)
include_directories(${dependencies_flatui_dir}/include)
include_directories(${dependencies_motive_dir}/include)
include_directories(${dependencies_pindrop_dir}/include)
include_directories(${dependencies_corgi_dir}/include)

# SDL includes.
include_directories(${SDL2_BINARY_DIR}/include ${SDL2_SOURCE_DIR}/include)

# Detect clang
if(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang" OR
   CMAKE_CXX_COMPILER_ID MATCHES ".*clang")
  set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

# Compiler flags.
set(C_FLAGS_WARNINGS "")
if(MSVC)
  set(C_FLAGS_WARNINGS "/W4 /WX /wd4065 /wd4244 /wd4511 /wd4512 /wd4706")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR
       CMAKE_COMPILER_IS_CLANGXX)
  add_definitions(-g)
  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic -Werror -Wextra -Wno-unused-function")
  # -Wno-unused-function is because of webp encode.h
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS_WARNINGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${C_FLAGS_WARNINGS}")

if(PIE_NOON_DEBUG)
  # if we want to define this, it needs to be only in debug builds
  add_definitions(-D_DEBUG)
endif()

if(WIN32)
  add_definitions(-D_USE_MATH_DEFINES)
  link_directories("$ENV{DXSDK_DIR}/Lib/$ENV{PROCESSOR_ARCHITECTURE}")
endif()

if(NOT fpl_ios)
  # Executable target.
  add_executable(pie_noon ${pie_noon_SRCS})
  # Additional flags for the target.
  mathfu_configure_flags(pie_noon)
  # Dependencies for the executable target.
  add_dependencies(pie_noon generated_includes assets motive)
  target_link_libraries(pie_noon
    motive
    corgi
    fplbase
    flatui
    pindrop
    sdl_mixer
    libvorbis
    libogg)
else()
  # Copy resources from macosx version
  file(GLOB_RECURSE pie_noon_RESOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/macosx/assets/*.*)
  foreach(FILE ${pie_noon_RESOURCES})
    file(RELATIVE_PATH variable
      ${CMAKE_CURRENT_BINARY_DIR}/macosx/assets ${FILE})
    get_filename_component(dir ${variable} DIRECTORY)
    set_source_files_properties(
      ${FILE}
      PROPERTIES
      MACOSX_PACKAGE_LOCATION pie_noon.app/assets/${dir}
      GENERATED TRUE)
  endforeach()

  # Link directories for SDL2
  link_directories("${dependencies_sdl_mixer_distr_dir}/Xcode-iOS/build")

  # Executable target.
  add_executable(pie_noon MACOSX_BUNDLE ${pie_noon_SRCS} ${pie_noon_RESOURCES})

  # Additional flags for the target.
  mathfu_configure_flags(pie_noon)

  # Dependencies for the executable target.
  add_dependencies(pie_noon pindrop motive
    sdl2_mixer_device sdl2_mixer_simulator
    generated_includes)

  # Skip the platform compiler checks for cross compiling
  set(CMAKE_CXX_COMPILER_WORKS TRUE)
  set(CMAKE_C_COMPILER_WORKS TRUE)

  # Set iOS specific attributes
  mathfu_set_ios_attributes(pie_noon)

  set_target_properties(pie_noon PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
  set_target_properties(pie_noon PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST pie_noon_ios.plist)
  set_target_properties(pie_noon PROPERTIES
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2")

  # Use the TARGET_FILE generator expression to get the full path of these libs
  # This is necessary because iOS adds $(EFFECTIVE_PLATFORM_NAME)
  # viz. -iphoneos and -iphonesimulator in front of the configuration name
  # e.g. Debug-iphoneos instead of Debug
  target_link_libraries(pie_noon
    $<TARGET_FILE:motive>
    $<TARGET_FILE:pindrop>)
endif()

# Create a zipped tar of all the necessary files to run the game.
add_custom_target(export
  COMMAND python ${CMAKE_CURRENT_LIST_DIR}/scripts/export.py
  DEPENDS pie_noon)

# Tests.
if(pie_noon_build_tests)
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/tests)
endif()
