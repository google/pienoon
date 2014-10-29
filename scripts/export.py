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

"""Export the game and all necessary assets.

This script will find the pie_noon and flatc executable and package them along
with all other assets required the play the game.
"""

import os
import platform
import sys
import zipfile


# The list of directories to export.
EXPORT_DIRECTORIES = [
    'assets',
    'src/flatbufferschemas',
    'src/rawassets',
]


# The list of additional individual files to export.
EXPORT_FILES = [
    'run.command',
    'run.bat',
    'scripts/build_assets.py'
]


def find_file(path, filename):
  """Find a file somewhere in the given directory."""
  for root, _, files in os.walk(path):
    if filename in files:
      return os.path.join(root, filename)


class MissingBinaryError(Exception):
  """Error indicating a binary could not be found."""

  def __init__(self, filename):
    Exception.__init__(self)
    self.filename = filename


def handle_io_error(error, filename):
  """Prints an error message to stderr about being unable to write a file."""
  sys.stderr.write('Could not write file %s. (%s)\n' % (filename, str(error)))


def zip_binary(zip_file, path, filename, output_dir):
  """Finds and adds a binary to the given zip file.

  Given a path to search, find the binary file and add it to the zip archive at
  the location given in the output_dir.

  Args:
    zip_file: The zip archive to add the binary to.
    path: Where to search for the binary.
    filename: The binary to search for. This will search for both `filename` and
        `filename.exe` variants.
    output_dir: The location in the zip archive to the binary. For example, if
        the file is found in `path/to/file` and output_dir is `foo`, the final
        location in the archive will be `foo/path/to/file`.
  Raises:
    MissingBinaryError: If `filename` and `filename.exe` cannot be found.
    IOError: If there is an error writing to zip_zipfile.
  """
  binary = find_file(path, filename) or find_file(path, filename + '.exe')
  if binary:
    output_path = os.path.join(output_dir, binary)
    try:
      zip_file.write(binary, output_path)
    except IOError as error:
      error.filename = filename
      raise error
  else:
    raise MissingBinaryError(filename)


def main():
  """Zips up all files needed to run the game.

  Zips up all files listed in EXPORT_FILES, all directories listed in
  EXPORT_DIRECTORIES, and the binary executables `pie_noon` and `flatc`, which
  have different locations when built using different tool chains.

  Returns:
    0 on success, 1 otherwise.
  """
  dir_name = os.path.basename(os.getcwd())
  zip_file_name = dir_name + '.zip'
  try:
    zip_file = zipfile.ZipFile(zip_file_name, 'w', zipfile.ZIP_DEFLATED)
  except IOError as error:
    sys.stderr.write(
        'Could not open %s for writing. (%s)\n' % (zip_file_name, str(error)))
    return 1

  # glob cannot recurse into subdirectories, so just use os.walk instead.
  for relative_path in EXPORT_DIRECTORIES:
    for root, _, filenames in os.walk(relative_path):
      for filename in filenames:
        input_path = os.path.join(root, filename)
        output_path = os.path.join(dir_name, root, filename)
        try:
          zip_file.write(input_path, output_path)
        except IOError as error:
          handle_io_error(error, filename)
          return 1

  for filename in EXPORT_FILES:
    output_path = os.path.join(dir_name, filename)
    try:
      zip_file.write(filename, output_path)
    except IOError as error:
      handle_io_error(error, filename)
      return 1

  try:
    zip_binary(zip_file, 'bin', 'pie_noon', dir_name)
    zip_binary(zip_file, 'bin', 'flatc', dir_name)
    if platform.system() == 'Windows':
      zip_binary(zip_file, 'bin', 'SDL2.dll', dir_name)
  except MissingBinaryError as error:
    sys.stderr.write('Could not find %s binary\n' % error.filename)
    return 1
  except IOError as error:
    handle_io_error(error, error.filename)
    return 1

  zip_file.close()

  return 0


if __name__ == '__main__':
  sys.exit(main())

