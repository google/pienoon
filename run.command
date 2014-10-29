#!/bin/bash -eu

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

# This script processes the json files and then runs the game regardless of
# where your build system of choice decides to put the executable (as long as
# it's in one of a few pre-defined locations).

# Change to current directory (becuase of how macs run .command files)
cd `dirname $0`

# Process .json files.
python ./scripts/build_assets.py

# Run the game.
for exe in ./bin/pie_noon ./bin/Release/pie_noon ./bin/Debug/pie_noon; do
  if [[ -e ${exe} ]]; then
    ${exe}
    break
  fi
done
