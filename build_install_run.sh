#!/bin/bash -eu
# Copyright (c) 2014 Google, Inc.
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
# Build, deploy, debug / execute a native Android package based upon
# NativeActivity.

: ${FPLUTIL:=$([[ -e $(dirname $0)/dependencies/fplutil ]] && \
               ( cd $(dirname $0)/dependencies/fplutil; pwd ) ||
               ( cd $(dirname $0)/../../libs/fplutil; pwd ))}
# Enable / disable apk installation.
: ${INSTALL=1}
# Enable / disable apk launch.
: ${LAUNCH=1}

# logcat filter which displays SDL, Play Games messages and system errors.
declare -r logcat_filter="\
  --adb_logcat_args \
  \"-s\" \
  SDL:V \
  SDL/APP:V \
  SDL/ERROR:V \
  SDL/SYSTEM:V \
  SDL/AUDIO:V \
  SDL/VIDEO:V \
  SDL/RENDER:V \
  SDL/INPUT:V \
  GamesNativeSDK:V \
  SDL_android:V \
  ValidateServiceOp:E \
  AndroidRuntime:E \*:E"

main() {
  local -r this_dir=$(cd $(dirname $0) && pwd)
  local additional_args=
  # Optionally enable installation of the built package.
  if [[ $((INSTALL)) -eq 1 ]]; then
    additional_args="${additional_args} -i"
  fi
  # Optionally enable execution of the built package.
  if [[ $((LAUNCH)) -eq 1 ]]; then
    additional_args="${additional_args} -r"
    additional_args="${additional_args} --adb_logcat_monitor \
                      ${logcat_filter}"
  fi
  # Build and sign with the test key if it's found.
  local -r key_dir=$(cd ${this_dir}/../../libraries/certs/pienoon && pwd)
  if [[ "${key_dir}" != "" ]]; then
    ${FPLUTIL}/bin/build_all_android.py \
      --apk_keypk8 ${key_dir}/pienoon.pk8 \
      --apk_keypem ${key_dir}/pienoon.x509.pem -S "$@" ${additional_args}
  else
    echo "${key_dir} not found, skipping signing step." >&2
    ${FPLUTIL}/bin/build_all_android.py -E google-play-services_lib \
      "$@" ${additional_args}
  fi
}

main "$@"
