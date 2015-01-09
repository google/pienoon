// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <unordered_map>

#include "renderer.h"
#include "common.h"

// Forward decls for FreeType & Harfbuzz
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;
struct hb_font_t;
struct hb_buffer_t;

namespace fpl {

// Constant to convert FreeType unit to pixel unit.
// In FreeType & Harfbuzz, the position value unit is 1/64 px whereas
// configurable in imgui. The constant is used to convert FreeType unit
// to px.
const int32_t kFreeTypeUnit = 64;

// FontManager manages font rendering with OpenGL utilizing freetype
// and harfbuzz as a glyph rendering and layout back end.
//
// It opens speficied OpenType/TrueType font and rasterize to OpenGL texture.
// An application can use the generated texture for a text rendering.
// TODO: Will change implementation using font atlas + vbo later.
//
// The class is not threadsafe, it's expected to be only used from
// within OpenGL rendering thread.
class FontManager {
 public:
  FontManager();
  ~FontManager();

  // Open font face, TTF, OT fonts are supported.
  // In this version it supports only single face at a time.
  // Return value: false when failing to open font, such as
  // a file open error, an invalid file format etc.
  bool Open(const char *font_name);

  // Discard font face that has been opened via Open().
  bool Close();

  // Retrieve a texture with the text.
  // Temporary API until it has vbo & font cache implementation.
  Texture* GetTexture(const char *text, const float ysize);

  // Set renderer. Renderer is used to create a texture instance.
  void SetRenderer(Renderer &renderer) {
    renderer_ = &renderer;
  }

  // Returns if a font has been loaded.
  bool FontLoaded() {
    return face_initialized_;
  }

private:
  // Initialize static data associated with the class.
  static void Initialize();

  // Clean up static data associated with the class.
  // Note that after the call, an application needs to call Initialize() again
  // to use the class.
  static void Terminate();

  // Renderer instance.
  Renderer *renderer_;

  // freetype's fontface instance. In this version of FontManager, it supports
  // only 1 font opened at a time.
  FT_Face face_;

  // harfbuzz's font information instance.
  hb_font_t *harfbuzz_font_;

  // Opened font file data.
  // The file needs to be kept open until FreeType finishes using the file.
  std::string font_data_;

  // flag indicating if a font file has loaded.
  bool face_initialized_;

  // Texture cache for a rendered image.
  // TODO:replace implementation using fontAtlas.
  std::unordered_map<std::string, std::unique_ptr<Texture>> map_textures_;

  // Singleton instance of Freetype library.
  static FT_Library *ft_;

  // Harfbuzz buffer
  static hb_buffer_t *harfbuzz_buf_;
};

}  // namespace fpl

#endif // FONT_MANAGER_H
