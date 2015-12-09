#!/bin/bash
# Copyright (c) 2015 Google, Inc.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software
# in a product, an acknowledgment in the product documentation would be
# appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

#
# Script to build Pie Noon iOS.
#
# fpl_ios=1 variable should be set while invoking CMake
#
# Headers file are generated using OS X build as dependent tools such as "flatc"
# are only build on OS X.
#
# Assets that are bundled with the iOS App are also generated using OS X build
# for same reason as above
set -eux
readonly script_directory=$(cd $(dirname $0); pwd)
readonly project_root=$(cd .. ; pwd)
readonly osx_directory=${script_directory}/macosx
# Use OSX assets and headers for the iOS build.
mkdir -p ${osx_directory}
pushd ${osx_directory}
cmake ${project_root} -GXcode
xcodebuild -target generated_includes -configuration Release
xcodebuild -target assets -configuration Release
popd

# Build the iOS project
cd ${script_directory}
cmake ${project_root}  -GXcode -Dfpl_ios=1
xcodebuild
