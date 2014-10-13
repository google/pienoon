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

import glob
import os
import platform
import subprocess
import sys

# The project root directory, which is one level up from this script's
# directory.
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                            os.path.pardir))

PREBUILTS_ROOT = os.path.abspath(os.path.join(os.path.join(PROJECT_ROOT),
    os.path.pardir, os.path.pardir, os.path.pardir, os.path.pardir))

# Directories that may contains the FlatBuffers compiler.
FLATBUFFERS_PATHS = [
    os.path.join(PROJECT_ROOT, 'bin'),
    os.path.join(PROJECT_ROOT, 'bin', 'Release'),
    os.path.join(PROJECT_ROOT, 'bin', 'Debug'),
]

# Directory that contains the cwebp tool.
CWEBP_PATHS = [
    os.path.join(PROJECT_ROOT, 'bin'),
    os.path.join(PROJECT_ROOT, 'bin', 'Release'),
    os.path.join(PROJECT_ROOT, 'bin', 'Debug'),
    os.path.join(PREBUILTS_ROOT, 'libwebp', 'libwebp-0.4.1-linux-x86-32', 'bin')
]

# Directory to place processed assets.
ASSETS_PATH = os.path.join(PROJECT_ROOT, 'assets')

# Directory where unprocessed assets can be found.
RAW_ASSETS_PATH = os.path.join(PROJECT_ROOT, 'src', 'rawassets')

# Directory where processed sound flatbuffer data can be found.
SOUND_PATH = os.path.join(ASSETS_PATH, 'sounds')

# Directory where unprocessed sound flatbuffer data can be found.
RAW_SOUND_PATH = os.path.join(RAW_ASSETS_PATH, 'sounds')

# Directory where processed material flatbuffer data can be found.
MATERIAL_PATH = os.path.join(ASSETS_PATH, 'materials')

# Directory where unprocessed material flatbuffer data can be found.
RAW_MATERIAL_PATH = os.path.join(RAW_ASSETS_PATH, 'materials')

# Directory where processed textures can be found.
TEXTURE_PATH = os.path.join(ASSETS_PATH, 'textures')

# Directory where unprocessed textures can be found.
RAW_TEXTURE_PATH = os.path.join(RAW_ASSETS_PATH, 'textures')

# Directory where unprocessed assets can be found.
SCHEMA_PATH = os.path.join(PROJECT_ROOT, 'src', 'flatbufferschemas')

# Windows uses the .exe extension on executables.
EXECUTABLE_EXTENSION = '.exe' if platform.system() == 'Windows' else ''

# Name of the flatbuffer executable.
FLATC_EXECUTABLE_NAME = 'flatc' + EXECUTABLE_EXTENSION

# Name of the cwebp executable.
CWEBP_EXECUTABLE_NAME = 'cwebp' + EXECUTABLE_EXTENSION

# What level of quality we want to apply to the webp files.
# Ranges from 0 to 100.
WEBP_QUALITY = 90


def processed_json_dir(path):
  """Take the path to a raw json asset and convert it to target directory."""
  return os.path.dirname(path.replace(RAW_ASSETS_PATH, ASSETS_PATH))


# json describing FlatBuffers data that will be converted to FlatBuffers binary
# files.
FLATBUFFERS_SCHEMA_JSON = [
    {
        'schema': os.path.join(SCHEMA_PATH, 'config.fbs'),
        'input_files': [os.path.join(RAW_ASSETS_PATH, 'config.json')],
        'output_path': ASSETS_PATH
    },
    {
        'schema': os.path.join(SCHEMA_PATH, 'buses.fbs'),
        'input_files': [os.path.join(RAW_ASSETS_PATH, 'buses.json')],
        'output_path': ASSETS_PATH
    },
    {
        'schema': os.path.join(SCHEMA_PATH, 'sound_assets.fbs'),
        'input_files': [os.path.join(RAW_ASSETS_PATH, 'sound_assets.json')],
        'output_path': ASSETS_PATH
    },
    {
        'schema': os.path.join(SCHEMA_PATH, 'character_state_machine_def.fbs'),
        'input_files': [os.path.join(RAW_ASSETS_PATH,
                                     'character_state_machine_def.json')],
        'output_path': ASSETS_PATH
    },
    {
        'schema': os.path.join(SCHEMA_PATH, 'sound_collection_def.fbs'),
        'input_files': glob.glob(os.path.join(RAW_SOUND_PATH, '*.json')),
        'output_path': SOUND_PATH
    },
    {
        'schema': os.path.join(SCHEMA_PATH, 'materials.fbs'),
        'input_files': glob.glob(os.path.join(RAW_MATERIAL_PATH, '*.json')),
        'output_path': MATERIAL_PATH
    }
]


def processed_texture_path(path):
  """Take the path to a raw png asset and convert it to target webp path."""
  return path.replace(RAW_ASSETS_PATH, ASSETS_PATH).replace('png', 'webp')


# PNG files to convert to webp.
PNG_TEXTURES = {
    'input_files': glob.glob(os.path.join(RAW_TEXTURE_PATH, '*.png')),
    'output_files': [processed_texture_path(png_path)
                     for png_path in glob.glob(os.path.join(RAW_TEXTURE_PATH,
                                                            '*.png'))]
}


def find_executable(name, paths):
  """Searches for a file with named `name` in the given paths and returns it."""
  for path in paths:
    full_path = os.path.join(path, name)
    if os.path.isfile(full_path):
      return full_path
  # If not found, just assume it's in the PATH.
  return name


# Location of FlatBuffers compiler.
FLATC = find_executable(FLATC_EXECUTABLE_NAME, FLATBUFFERS_PATHS)

# Location of webp compression tool.
CWEBP = find_executable(CWEBP_EXECUTABLE_NAME, CWEBP_PATHS)


class BuildError(Exception):
  """Error indicating there was a problem building assets."""

  def __init__(self, argv, error_code):
    Exception.__init__(self)
    self.argv = argv
    self.error_code = error_code


def run_subprocess(argv):
  process = subprocess.Popen(argv)
  process.wait()
  if process.returncode:
    raise BuildError(argv, process.returncode)


def convert_json_to_flatbuffer_binary(json, schema, out_dir):
  """Run the flatbuffer compiler on the given json file and schema.

  Args:
    json: The path to the json file to convert to a flatbuffer binary.
    schema: The path to the schema to use in the conversion process.
    out_dir: The directory to write the flatbuffer binary.

  Raises:
    BuildError: Process return code was nonzero.
  """
  command = [FLATC, '-o', out_dir, '-b', schema, json]
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
    True if the source file is newer than the target, or if the target file does
    not exist.
  """
  return not os.path.isfile(target) or (
      os.path.getmtime(source) > os.path.getmtime(target))


def processed_json_path(path):
  """Take the path to a raw json asset and convert it to target bin path."""
  return path.replace(RAW_ASSETS_PATH, ASSETS_PATH).replace('.json', '.bin')


def generate_flatbuffer_binaries():
  """Run the flatbuffer compiler on the all of the flatbuffer json files."""
  for element in FLATBUFFERS_SCHEMA_JSON:
    schema = element['schema']
    output_path = element['output_path']
    if not os.path.exists(output_path):
      os.makedirs(output_path)
    for json in element['input_files']:
      target = processed_json_path(json)
      if needs_rebuild(json, target) or needs_rebuild(schema, target):
        convert_json_to_flatbuffer_binary(
            json, schema, output_path)


def generate_webp_textures():
  """Run the webp converter on off of the png files."""
  input_files = PNG_TEXTURES['input_files']
  output_files = PNG_TEXTURES['output_files']
  if not os.path.exists(TEXTURE_PATH):
    os.makedirs(TEXTURE_PATH)
  for png, out in zip(input_files, output_files):
    if needs_rebuild(png, out):
      convert_png_image_to_webp(png, out, WEBP_QUALITY)


def clean_webp_textures():
  """Delete all the processed webp textures."""
  for webp in PNG_TEXTURES['output_files']:
    if os.path.isfile(webp):
      os.remove(webp)


def clean_flatbuffer_binaries():
  """Delete all the processed flatbuffer binaries."""
  for element in FLATBUFFERS_SCHEMA_JSON:
    for json in element['input_files']:
      path = processed_json_path(json)
      if os.path.isfile(path):
        os.remove(path)


def clean():
  """Delete all the processed files."""
  clean_flatbuffer_binaries()
  clean_webp_textures()


def handle_build_error(error):
  """Prints an error message to stderr for BuildErrors."""
  sys.stderr.write('Error running command `%s`. Returned %s.\n' % (
      ' '.join(error.argv), str(error.error_code)))


def main(argv):
  """Builds or cleans the assets needed for the game.

  To build all assets, either call this script without any arguments. Or
  alternatively, call it with the argument 'all'. To just convert the flatbuffer
  json files, call it with 'flatbuffers'. Likewise to convert the png files to
  webp files, call it with 'webp'. To clean all converted files, call it with
  'clean'.

  Args:
    argv: The command line argument containing which command to run.

  Returns:
    Returns 0 on success.
  """
  target = argv[1] if len(argv) >= 2 else 'all'
  if target not in ('all', 'flatbuffers', 'webp', 'clean'):
    sys.stderr.write('No rule to build target %s.\n' % target)

  if target in ('all', 'flatbuffers'):
    try:
      generate_flatbuffer_binaries()
    except BuildError as error:
      handle_build_error(error)
      return 1
  if target in ('all', 'webp'):
    try:
      generate_webp_textures()
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

