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
cmake_minimum_required(VERSION 2.8.12)

include(CheckIncludeFiles)
include(CheckLibraryExists)

set(webp_dir ${dependencies_webp_distr_dir})
option(webp_build_cwebp "Build the cwebp command line tool." OFF)
option(webp_build_dwebp "Build the cwebp command line tool." OFF)

if(webp_build_cwebp OR webp_build_dwebp)
  # Find the standard C math library.
  if(UNIX)
    find_library(MATH_LIBRARY NAMES m)
  endif()

  # Configure step for command line tools.
  check_library_exists(png png_get_libpng_ver "" WEBP_HAVE_PNG)
  check_library_exists(jpeg jpeg_set_defaults "" WEBP_HAVE_JPEG)
  check_library_exists(tiff TIFFGetVersion "" WEBP_HAVE_TIFF)
  check_library_exists(gif DGifOpenFileHandle "" WEBP_HAVE_GIF)
  check_include_files(wincodec.h shlwapi.h windows.h WEBP_HAVE_WIC)
  configure_file(${CMAKE_CURRENT_LIST_DIR}/config.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/include/webp/config.h)

  # Set the paths of libraries command line tools depend upon.
  find_library(PNG_LIBRARY NAMES png)
  find_library(JPEG_LIBRARY NAMES jpeg)
  find_library(TIFF_LIBRARY NAMES tiff)
  find_library(GIF_LIBRARY NAMES gif)

  set(IMAGE_CODEC_LIBRARIES "")
  if(PNG_LIBRARY)
    list(APPEND IMAGE_CODEC_LIBRARIES ${PNG_LIBRARY})
  endif()
  if(JPEG_LIBRARY)
    list(APPEND IMAGE_CODEC_LIBRARIES ${JPEG_LIBRARY})
  endif()
  if(TIFF_LIBRARY)
    list(APPEND IMAGE_CODEC_LIBRARIES ${TIFF_LIBRARY})
  endif()
  if(GIF_LIBRARY)
    list(APPEND IMAGE_CODEC_LIBRARIES ${GIF_LIBRARY})
  endif()
endif()

# Webp source files.
set(webp_SRCS
    ${webp_dir}/src/demux/demux.c
    ${webp_dir}/src/dec/tree.c
    ${webp_dir}/src/dec/quant.c
    ${webp_dir}/src/dec/vp8li.h
    ${webp_dir}/src/dec/alpha.c
    ${webp_dir}/src/dec/vp8i.h
    ${webp_dir}/src/dec/vp8l.c
    ${webp_dir}/src/dec/vp8.c
    ${webp_dir}/src/dec/alphai.h
    ${webp_dir}/src/dec/idec.c
    ${webp_dir}/src/dec/io.c
    ${webp_dir}/src/dec/webpi.h
    ${webp_dir}/src/dec/buffer.c
    ${webp_dir}/src/dec/frame.c
    ${webp_dir}/src/dec/webp.c
    ${webp_dir}/src/dec/decode_vp8.h
    ${webp_dir}/src/utils/endian_inl.h
    ${webp_dir}/src/utils/bit_reader_inl.h
    ${webp_dir}/src/utils/filters.h
    ${webp_dir}/src/utils/huffman_encode.c
    ${webp_dir}/src/utils/huffman.c
    ${webp_dir}/src/utils/rescaler.h
    ${webp_dir}/src/utils/thread.h
    ${webp_dir}/src/utils/huffman_encode.h
    ${webp_dir}/src/utils/quant_levels_dec.c
    ${webp_dir}/src/utils/thread.c
    ${webp_dir}/src/utils/rescaler.c
    ${webp_dir}/src/utils/quant_levels.c
    ${webp_dir}/src/utils/utils.h
    ${webp_dir}/src/utils/random.h
    ${webp_dir}/src/utils/bit_reader.h
    ${webp_dir}/src/utils/filters.c
    ${webp_dir}/src/utils/bit_writer.h
    ${webp_dir}/src/utils/utils.c
    ${webp_dir}/src/utils/bit_reader.c
    ${webp_dir}/src/utils/huffman.h
    ${webp_dir}/src/utils/random.c
    ${webp_dir}/src/utils/color_cache.h
    ${webp_dir}/src/utils/color_cache.c
    ${webp_dir}/src/utils/bit_writer.c
    ${webp_dir}/src/utils/quant_levels_dec.h
    ${webp_dir}/src/utils/quant_levels.h
    ${webp_dir}/src/enc/picture.c
    ${webp_dir}/src/enc/histogram.c
    ${webp_dir}/src/enc/histogram.h
    ${webp_dir}/src/enc/picture_rescale.c
    ${webp_dir}/src/enc/tree.c
    ${webp_dir}/src/enc/quant.c
    ${webp_dir}/src/enc/cost.c
    ${webp_dir}/src/enc/vp8li.h
    ${webp_dir}/src/enc/token.c
    ${webp_dir}/src/enc/alpha.c
    ${webp_dir}/src/enc/vp8l.c
    ${webp_dir}/src/enc/picture_tools.c
    ${webp_dir}/src/enc/iterator.c
    ${webp_dir}/src/enc/filter.c
    ${webp_dir}/src/enc/cost.h
    ${webp_dir}/src/enc/picture_psnr.c
    ${webp_dir}/src/enc/picture_csp.c
    ${webp_dir}/src/enc/config.c
    ${webp_dir}/src/enc/vp8enci.h
    ${webp_dir}/src/enc/analysis.c
    ${webp_dir}/src/enc/backward_references.c
    ${webp_dir}/src/enc/backward_references.h
    ${webp_dir}/src/enc/frame.c
    ${webp_dir}/src/enc/syntax.c
    ${webp_dir}/src/enc/webpenc.c
    ${webp_dir}/src/dsp/yuv_sse2.c
    ${webp_dir}/src/dsp/enc_neon.c
    ${webp_dir}/src/dsp/dec_neon.c
    ${webp_dir}/src/dsp/lossless.c
    ${webp_dir}/src/dsp/enc_mips32.c
    ${webp_dir}/src/dsp/dec_mips32.c
    ${webp_dir}/src/dsp/dec_clip_tables.c
    ${webp_dir}/src/dsp/enc.c
    ${webp_dir}/src/dsp/yuv_mips32.c
    ${webp_dir}/src/dsp/lossless.h
    ${webp_dir}/src/dsp/dec.c
    ${webp_dir}/src/dsp/yuv.c
    ${webp_dir}/src/dsp/enc_sse2.c
    ${webp_dir}/src/dsp/cpu.c
    ${webp_dir}/src/dsp/neon.h
    ${webp_dir}/src/dsp/lossless_sse2.c
    ${webp_dir}/src/dsp/lossless_mips32.c
    ${webp_dir}/src/dsp/yuv_tables_sse2.h
    ${webp_dir}/src/dsp/dsp.h
    ${webp_dir}/src/dsp/yuv.h
    ${webp_dir}/src/dsp/upsampling.c
    ${webp_dir}/src/dsp/lossless_neon.c
    ${webp_dir}/src/dsp/dec_sse2.c
    ${webp_dir}/src/dsp/enc_avx2.c
    ${webp_dir}/src/dsp/upsampling_neon.c
    ${webp_dir}/src/dsp/alpha_processing.c
    ${webp_dir}/src/dsp/upsampling_sse2.c)

# Includes for this project.
include_directories(${webp_dir}/src)
include_directories(${webp_dir}/include)
# Include config.h if the command line tools are being built.
if(webp_build_cwebp OR webp_build_dwebp)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/include)
  include_directories(${CMAKE_CURRENT_BINARY_DIR}/include/webp)
endif()

# Executable target.
add_definitions(-Wall)
if(webp_build_cwebp OR webp_build_dwebp)
  add_definitions(-DHAVE_CONFIG_H)
endif()

add_library(webp ${webp_SRCS})

# Create a build rule for an example executable.
function(add_example_executable executable_name)
  add_executable(${executable_name} ${ARGN})
  target_link_libraries(${executable_name} exampleutil webp ${MATH_LIBRARY}
                        ${IMAGE_CODEC_LIBRARIES})
endfunction()

if(webp_build_cwebp OR webp_build_dwebp)
  # Example utility library.
  set(exampleutil_SRCS
      ${webp_dir}/examples/example_util.c
      ${webp_dir}/examples/example_util.h
      ${webp_dir}/examples/stopwatch.h)
  add_library(exampleutil ${exampleutil_SRCS})
endif()

if(webp_build_dwebp)
  # dwebp
  set(dwebp_SRCS
      ${webp_dir}/examples/dwebp.c
      ${webp_dir}/examples/stopwatch.h)
  add_example_executable(dwebp ${dwebp_SRCS})
endif()

if(webp_build_cwebp)
  # cwebp
  set(cwebp_SRCS
      ${webp_dir}/examples/cwebp.c
      ${webp_dir}/examples/metadata.c
      ${webp_dir}/examples/metadata.h
      ${webp_dir}/examples/stopwatch.h
      ${webp_dir}/examples/jpegdec.c
      ${webp_dir}/examples/jpegdec.h
      ${webp_dir}/examples/pngdec.c
      ${webp_dir}/examples/pngdec.h
      ${webp_dir}/examples/tiffdec.c
      ${webp_dir}/examples/tiffdec.h
      ${webp_dir}/examples/webpdec.c
      ${webp_dir}/examples/webpdec.h
      ${webp_dir}/examples/wicdec.c
      ${webp_dir}/examples/wicdec.h)
  add_example_executable(cwebp ${cwebp_SRCS})
endif()
