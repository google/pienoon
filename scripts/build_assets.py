#!/usr/bin/python
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


"""Builds all assets under rawassets/, writing the results to assets/.

Finds the flatbuffer compiler and cwebp tool and then uses them to convert the
JSON files to flatbuffer binary files and the png files to webp files so that
they can be loaded by the game. This script also includes various 'make' style
rules. If you just want to build the flatbuffer binaries you can pass
'flatbuffer' as an argument, or if you want to just build the webp files you can
pass 'cwebp' as an argument. Additionally, if you would like to clean all
generated files, you can call this script with the argument 'clean'.
"""


import distutils.dir_util
import glob
import json
import os
import sys
# The project root directory, which is two levels up from this script's
# directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                            os.path.pardir))
# Add possible paths that the scene_lab_assets_builder module might be in.
sys.path.append(os.path.join(PROJECT_ROOT, os.path.pardir, os.path.pardir,
                             'libs', 'scene_lab', 'scripts'))
sys.path.append(os.path.join(PROJECT_ROOT, 'dependencies',
                             'scene_lab', 'scripts'))
import scene_lab_asset_builder as builder

# Directory to place processed assets.
ASSETS_PATH = os.path.join(PROJECT_ROOT, 'assets')

# Directory where unprocessed assets can be found.
RAW_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'rawassets')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_PATH = os.path.join(RAW_ASSETS_PATH, 'sounds')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_BANK_PATH = os.path.join(RAW_ASSETS_PATH, 'sound_banks')

# Directory where unprocessed material flatbuffer data can be found.
RAW_MATERIAL_PATH = os.path.join(RAW_ASSETS_PATH, 'materials')

# Directory where unprocessed textures can be found.
RAW_TEXTURE_PATH = os.path.join(RAW_ASSETS_PATH, 'textures')

# Location of flatbuffer schemas in this project.
PROJECT_SCHEMA_PATH = builder.DependencyPath(os.path.join('src',
                                                          'flatbufferschemas'))

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'obj', 'assets')

# Directory where png files are written to before they are converted to webp.
INTERMEDIATE_TEXTURE_PATH = os.path.join(INTERMEDIATE_ASSETS_PATH, 'textures')

# Potential root directories for source assets.
ASSET_ROOTS = [RAW_ASSETS_PATH, INTERMEDIATE_TEXTURE_PATH]

# Overlay directories.
OVERLAY_DIRS = [os.path.relpath(f, RAW_ASSETS_PATH)
                for f in glob.glob(os.path.join(RAW_ASSETS_PATH, 'overlays',
                                                '*'))]

# A list of json files and their schemas that will be converted to binary files
# by the flatbuffer compiler.
FLATBUFFERS_CONVERSION_DATA = [
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('config.fbs'),
        extension='.pieconfig',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'config.json'),
                     os.path.join(RAW_ASSETS_PATH, 'cardboard_config.json')]),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'buses.fbs'),
        extension='.pinbus',
        input_files=[os.path.join(RAW_ASSETS_PATH, 'buses.json')]),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'sound_bank_def.fbs'),
        extension='.pinbank',
        input_files=glob.glob(os.path.join(RAW_SOUND_BANK_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=PROJECT_SCHEMA_PATH.join('character_state_machine_def.fbs'),
        extension='.piestate',
        input_files=[os.path.join(RAW_ASSETS_PATH,
                                  'character_state_machine_def.json')]),
    builder.FlatbuffersConversionData(
        schema=builder.PINDROP_ROOT.join('schemas', 'sound_collection_def.fbs'),
        extension='.pinsound',
        input_files=glob.glob(os.path.join(RAW_SOUND_PATH, '*.json'))),
    builder.FlatbuffersConversionData(
        schema=builder.FPLBASE_ROOT.join('schemas', 'materials.fbs'),
        extension='.fplmat',
        input_files=glob.glob(os.path.join(RAW_MATERIAL_PATH, '*.json')),
        overlay_globs=['*.json']),
    # Empty file lists to make other project schemas available to include.
    builder.FlatbuffersConversionData(
        schema=builder.MOTIVE_ROOT.join('schemas', 'anim_table.fbs'),
        extension='', input_files=[])
]


def fbx_files_to_convert():
  """FBX files to convert to fplmesh."""
  return glob.glob(os.path.join(RAW_MESH_PATH, '*.fbx'))


def texture_files(pattern):
  """List of files matching pattern in the texture paths.

  Args:
    pattern: glob style pattern.

  Returns:
    List of filenames.
  """
  return (glob.glob(os.path.join(RAW_TEXTURE_PATH, pattern)) +
          glob.glob(os.path.join(INTERMEDIATE_TEXTURE_PATH, pattern)))


def png_files_to_convert():
  """PNG files to convert to webp."""
  return texture_files('*.png')


def tga_files_to_convert():
  """TGA files to convert to png."""
  return texture_files('*.tga')


def anim_files_to_convert():
  """FBX files to convert to `.motiveanim`."""
  return glob.glob(os.path.join(RAW_ANIM_PATH, '*.fbx'))


def main():
  """Builds or cleans the assets needed for the game.

  To build all assets, either call this script without any arguments. Or
  alternatively, call it with the argument 'all'. To just convert the
  flatbuffer json files, call it with 'flatbuffers'. Likewise to convert the
  png files to webp files, call it with 'webp'. To clean all converted files,
  call it with 'clean'.

  Returns:
    Returns 0 on success.
  """
  return builder.main(
      project_root=PROJECT_ROOT,
      assets_path=ASSETS_PATH,
      asset_roots=ASSET_ROOTS,
      intermediate_path=INTERMEDIATE_TEXTURE_PATH,
      overlay_dirs=OVERLAY_DIRS,
      tga_files_to_convert=tga_files_to_convert,
      png_files_to_convert=png_files_to_convert,
      flatbuffers_conversion_data=lambda: FLATBUFFERS_CONVERSION_DATA)


if __name__ == '__main__':
  sys.exit(main())
