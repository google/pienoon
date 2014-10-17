#!/bin/bash -eux

# Copyright 2014 Google Inc. All Rights Reserved.
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

SDK_DIR=$(cd $(dirname $(which android))/..; pwd)

package_version() {
  grep Revision $SDK_DIR/$1/source.properties | sed 's/.*=//;s/\(\.0\)*$//'
}

# Updates a package if the revision in the target directory is not what is
# expected
#
# Args:
#   1: package name
#   2: package directory
update_package() {
  line="$(android list sdk -a -u | awk "/$1/ {print \$0; exit 0}")"
  version="$(echo $line | awk '{print $NF}')"
  if [[ $version != $(package_version $2) ]]; then
    echo 'y' | android update sdk -u -a -t $(echo $line | awk -F- '{print $1}')
  fi
}

main() {
  local output_dir=
  local dist_dir=
  while getopts "o:d:" options; do
     case ${options} in
       o ) output_dir=${OPTARG};;
       d ) dist_dir=${OPTARG};;
     esac
  done

  # Make sure we have all the latest tools installed.
  android list sdk -a -u
  update_package 'Android SDK Tools,' tools/
  update_package 'Android SDK Platform-tools,' platform-tools/
  update_package 'Android SDK Build-tools,' build-tools/20.0.0/

  # Make sure Google Play services is installed.
  android list sdk -a -u
  update_package 'Google Play services,' extras/google/google_play_services/

  cd vendor/unbundled_google/packages/splat
  rm -rf bin
  ../../../../prebuilts/cmake/linux-x86/cmake-2.8.12.1-Linux-i386/bin/cmake .
  make assets
  ./build_apk_sdl.sh LAUNCH=0 DEPLOY=0
  cp ./bin/splat-release-unsigned.apk $dist_dir/PieNoon.apk
}

main "$@"

