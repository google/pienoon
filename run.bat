@echo off

rem Copyright 2014 Google Inc. All rights reserved.
rem
rem Licensed under the Apache License, Version 2.0 (the "License");
rem you may not use this file except in compliance with the License.
rem You may obtain a copy of the License at
rem
rem     http://www.apache.org/licenses/LICENSE-2.0
rem
rem Unless required by applicable law or agreed to in writing, software
rem distributed under the License is distributed on an "AS IS" BASIS,
rem WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
rem See the License for the specific language governing permissions and
rem limitations under the License.
rem
rem This script processes the json files and then runs the game regardless of
rem where your build system of choice decides to put the executable (as long as
rem it's in one of a few pre-defined locations).

rem Process .json files.
python .\scripts\build_assets.py

rem Run the game.
for %%A IN (.\bin\pie_noon.exe .\bin\Release\pie_noon.exe .\bin\Debug\pie_noon.exe)^
DO if exist %%A (%%A && goto :break)
:break
