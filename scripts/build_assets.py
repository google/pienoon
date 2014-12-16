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

"""Builds all assets under src/rawassets/, writing the results to assets/.

Finds the flatbuffer compiler and cwebp tool and then uses them to convert the
JSON files to flatbuffer binary files and the png files to webp files so that
they can be loaded by the game. This script also includes various 'make' style
rules. If you just want to build the flatbuffer binaries you can pass
'flatbuffer' as an argument, or if you want to just build the webp files you can
pass 'cwebp' as an argument. Additionally, if you would like to clean all
generated files, you can call this script with the argument 'clean'.
"""

import argparse
import distutils.spawn
import glob
import os
import platform
import shutil
import subprocess
import sys

# The project root directory, which is one level up from this script's
# directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                            os.path.pardir))

PREBUILTS_ROOT = os.path.abspath(os.path.join(os.path.join(PROJECT_ROOT),
                                              os.path.pardir, os.path.pardir,
                                              os.path.pardir, os.path.pardir,
                                              'prebuilts'))

PINDROP_ROOT = os.path.abspath(os.path.join(os.path.join(PROJECT_ROOT),
                                            os.path.pardir, os.path.pardir,
                                            'libs', "audio_engine"))

# Directories that may contains the FlatBuffers compiler.
FLATBUFFERS_PATHS = [
    os.path.join(PROJECT_ROOT, 'bin'),
    os.path.join(PROJECT_ROOT, 'bin', 'Release'),
    os.path.join(PROJECT_ROOT, 'bin', 'Debug')
]

# Directory that contains the cwebp tool.
CWEBP_BINARY_IN_PATH = distutils.spawn.find_executable('cwebp')
CWEBP_PATHS = [
    os.path.join(PROJECT_ROOT, 'bin'),
    os.path.join(PROJECT_ROOT, 'bin', 'Release'),
    os.path.join(PROJECT_ROOT, 'bin', 'Debug'),
    os.path.join(PREBUILTS_ROOT, 'libwebp',
                 '%s-x86' % platform.system().lower(),
                 'libwebp-0.4.1-%s-x86-32' % platform.system().lower(), 'bin'),
    os.path.dirname(CWEBP_BINARY_IN_PATH) if CWEBP_BINARY_IN_PATH else '',
]

# Directory to place processed assets.
ASSETS_PATH = os.path.join(PROJECT_ROOT, 'assets')

# Directory where unprocessed assets can be found.
RAW_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'src', 'rawassets')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_PATH = os.path.join(RAW_ASSETS_PATH, 'sounds')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_BANK_PATH = os.path.join(RAW_ASSETS_PATH, 'sound_banks')

# Directory where unprocessed material flatbuffer data can be found.
RAW_MATERIAL_PATH = os.path.join(RAW_ASSETS_PATH, 'materials')

# Directory where unprocessed textures can be found.
RAW_TEXTURE_PATH = os.path.join(RAW_ASSETS_PATH, 'textures')

# Directory where unprocessed assets can be found.
SCHEMA_PATHS = [
    os.path.join(PROJECT_ROOT, 'src', 'flatbufferschemas'),
    os.path.join(PINDROP_ROOT, 'schemas')
]

# Windows uses the .exe extension on executables.
EXECUTABLE_EXTENSION = '.exe' if platform.system() == 'Windows' else ''

# Name of the flatbuffer executable.
FLATC_EXECUTABLE_NAME = 'flatc' + EXECUTABLE_EXTENSION

# Name of the cwebp executable.
CWEBP_EXECUTABLE_NAME = 'cwebp' + EXECUTABLE_EXTENSION

# What level of quality we want to apply to the webp files.
# Ranges from 0 to 100.
WEBP_QUALITY = 90


class FlatbuffersConversionData(object):
  """Holds data needed to convert a set of json files to flatbuffer binaries.

  Attributes:
    schema: The path to the flatbuffer schema file.
    input_files: A list of input files to convert.
  """

  def __init__(self, schema, input_files):
    """Initializes this object's schema and input_files."""
    self.schema = schema
    self.input_files = input_files


def find_in_paths(name, paths):
  """Searches for a file with named `name` in the given paths and returns it."""
  for path in paths:
    full_path = os.path.join(path, name)
    if os.path.isfile(full_path):
      return full_path
  # If not found, just assume it's in the PATH.
  return name


# A list of json files and their schemas that will be converted to binary files
# by the flatbuffer compiler.
FLATBUFFERS_CONVERSION_DATA = [
    FlatbuffersConversionData(
        schema=find_in_paths('config.fbs', SCHEMA_PATHS),
        input_files=[os.path.join(RAW_ASSETS_PATH, 'config.json')]),
    FlatbuffersConversionData(
        schema=find_in_paths('buses.fbs', SCHEMA_PATHS),
        input_files=[os.path.join(RAW_ASSETS_PATH, 'buses.json')]),
    FlatbuffersConversionData(
        schema=find_in_paths('sound_bank_def.fbs', SCHEMA_PATHS),
        input_files=glob.glob(os.path.join(RAW_SOUND_BANK_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=find_in_paths('character_state_machine_def.fbs', SCHEMA_PATHS),
        input_files=[os.path.join(RAW_ASSETS_PATH,
                                  'character_state_machine_def.json')]),
    FlatbuffersConversionData(
        schema=find_in_paths('sound_collection_def.fbs', SCHEMA_PATHS),
        input_files=glob.glob(os.path.join(RAW_SOUND_PATH, '*.json'))),
    FlatbuffersConversionData(
        schema=find_in_paths('materials.fbs', SCHEMA_PATHS),
        input_files=glob.glob(os.path.join(RAW_MATERIAL_PATH, '*.json')))
]


def processed_texture_path(path, target_directory):
  """Take the path to a raw png asset and convert it to target webp path.

  Args:
    target_directory: Path to the target assets directory.
  """
  return path.replace(RAW_ASSETS_PATH, target_directory).replace('png', 'webp')


# PNG files to convert to webp.
PNG_TEXTURES = glob.glob(os.path.join(RAW_TEXTURE_PATH, '*.png'))

# Location of FlatBuffers compiler.
FLATC = find_in_paths(FLATC_EXECUTABLE_NAME, FLATBUFFERS_PATHS)

# Location of webp compression tool.
CWEBP = find_in_paths(CWEBP_EXECUTABLE_NAME, CWEBP_PATHS)


class BuildError(Exception):
  """Error indicating there was a problem building assets."""

  def __init__(self, argv, error_code, message=None):
    Exception.__init__(self)
    self.argv = argv
    self.error_code = error_code
    self.message = message if message else ''


def run_subprocess(argv):
  try:
    process = subprocess.Popen(argv)
  except OSError as e:
    raise BuildError(argv, 1, message=str(e))
  process.wait()
  if process.returncode:
    raise BuildError(argv, process.returncode)


def convert_json_to_flatbuffer_binary(flatc, json, schema, out_dir):
  """Run the flatbuffer compiler on the given json file and schema.

  Args:
    flatc: Path to the flatc binary.
    json: The path to the json file to convert to a flatbuffer binary.
    schema: The path to the schema to use in the conversion process.
    out_dir: The directory to write the flatbuffer binary.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [flatc, '-o', out_dir]
  for path in SCHEMA_PATHS:
    command.extend(['-I', path])
  command.extend(['-b', schema, json])
  run_subprocess(command)


def convert_png_image_to_webp(png, out, quality=80):
  """Run the webp converter on the given png file.

  Args:
    png: The path to the png file to convert into a webp file.
    out: The path of the webp to write to.
    quality: The quality of the processed image, where quality is between 0
        (poor) to 100 (very good). Typical value is around 80.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [CWEBP, '-q', str(quality), png, '-o', out]
  run_subprocess(command)


def needs_rebuild(source, target):
  """Checks if the source file needs to be rebuilt.

  Args:
    source: The source file to be compared.
    target: The target file which we may need to rebuild.

  Returns:
    True if the source file is newer than the target, or if the target file
    does not exist.
  """
  return not os.path.isfile(target) or (
      os.path.getmtime(source) > os.path.getmtime(target))


def processed_json_path(path, target_directory):
  """Take the path to a raw json asset and convert it to target bin path.

  Args:
    target_directory: Path to the target assets directory.
  """
  return path.replace(RAW_ASSETS_PATH, target_directory).replace(
    '.json', '.bin')


def generate_flatbuffer_binaries(flatc, target_directory):
  """Run the flatbuffer compiler on the all of the flatbuffer json files.

  Args:
    flatc: Path to the flatc binary.
    target_directory: Path to the target assets directory.
  """
  for element in FLATBUFFERS_CONVERSION_DATA:
    schema = element.schema
    for json in element.input_files:
      target = processed_json_path(json, target_directory)
      target_file_dir = os.path.dirname(target)
      if not os.path.exists(target_file_dir):
        os.makedirs(target_file_dir)
      if needs_rebuild(json, target) or needs_rebuild(schema, target):
        convert_json_to_flatbuffer_binary(flatc, json, schema, target_file_dir)


def generate_webp_textures(target_directory):
  """Run the webp converter on off of the png files.

  Args:
    target_directory: Path to the target assets directory.
  """
  input_files = PNG_TEXTURES
  for png in input_files:
    out = processed_texture_path(png, target_directory)
    out_dir = os.path.dirname(out)
    if not os.path.exists(out_dir):
      os.makedirs(out_dir)
    if needs_rebuild(png, out):
      convert_png_image_to_webp(png, out, WEBP_QUALITY)

def copy_assets(target_directory):
  """Copy modified assets to the target assets directory.

  All files are copied from ASSETS_PATH to the specified target_directory if
  they're newer than the destination files.

  Args:
    target_directory: Directory to copy assets to.
  """
  assets_dir = target_directory
  source_dir = os.path.realpath(ASSETS_PATH)
  if source_dir != os.path.realpath(assets_dir):
    for dirpath, _, files in os.walk(source_dir):
      for name in files:
        source_filename = os.path.join(dirpath, name)
        relative_source_dir = os.path.relpath(source_filename, source_dir)
        target_dir = os.path.dirname(os.path.join(assets_dir,
                                                  relative_source_dir))
        target_filename = os.path.join(target_dir, name)
        if not os.path.exists(target_dir):
          os.makedirs(target_dir)
        if (not os.path.exists(target_filename) or
            (os.path.getmtime(target_filename) <
             os.path.getmtime(source_filename))):
          shutil.copy2(source_filename, target_filename)

def clean_webp_textures():
  """Delete all the processed webp textures."""
  for webp in PNG_TEXTURES['output_files']:
    if os.path.isfile(webp):
      os.remove(webp)


def clean_flatbuffer_binaries(target_directory):
  """Delete all the processed flatbuffer binaries.

  Args:
    target_directory: Path to the target assets directory.
  """
  for element in FLATBUFFERS_CONVERSION_DATA:
    for json in element.input_files:
      path = processed_json_path(json, target_directory)
      if os.path.isfile(path):
        os.remove(path)


def clean():
  """Delete all the processed files."""
  clean_flatbuffer_binaries()
  clean_webp_textures()


def handle_build_error(error):
  """Prints an error message to stderr for BuildErrors."""
  sys.stderr.write('Error running command `%s`. Returned %s.\n%s\n' % (
      ' '.join(error.argv), str(error.error_code), str(error.message)))


def main(argv):
  """Builds or cleans the assets needed for the game.

  To build all assets, either call this script without any arguments. Or
  alternatively, call it with the argument 'all'. To just convert the
  flatbuffer json files, call it with 'flatbuffers'. Likewise to convert the
  png files to webp files, call it with 'webp'. To clean all converted files,
  call it with 'clean'.

  Args:
    argv: The command line argument containing which command to run.

  Returns:
    Returns 0 on success.
  """
  parser = argparse.ArgumentParser()
  parser.add_argument('--flatc', default=FLATC,
                      help='Location of the flatbuffers compiler.')
  parser.add_argument('--output', default=ASSETS_PATH,
                      help='Assets output directory.')
  parser.add_argument('args', nargs=argparse.REMAINDER)
  args = parser.parse_args()
  target = args.args[1] if len(args.args) >= 2 else 'all'
  if target not in ('all', 'flatbuffers', 'webp', 'clean'):
    sys.stderr.write('No rule to build target %s.\n' % target)

  if target != 'clean':
    copy_assets(args.output)
  if target in ('all', 'flatbuffers'):
    try:
      generate_flatbuffer_binaries(args.flatc, args.output)
    except BuildError as error:
      handle_build_error(error)
      return 1
  if target in ('all', 'webp'):
    try:
      generate_webp_textures(args.output)
    except BuildError as error:
      handle_build_error(error)
      return 1
  if target == 'clean':
    try:
      clean()
    except OSError as error:
      sys.stderr.write('Error cleaning: %s' % str(error))
      return 1

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))

