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

#include "renderer.h"
#include "glyph_cache.h"
// TODO: Add API using font cache + texture atlas.
#include "common.h"

// Forward decls for FreeType & Harfbuzz
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_ *FT_Face;
struct hb_font_t;
struct hb_buffer_t;

namespace fpl {

// Forward decl.
class FontTexture;
class FontMetrics;

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
  FontTexture* GetTexture(const char *text, const float ysize);

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

  // Expand a texture image buffer when the font metrics is changed.
  // Returns true if the image buffer was reallocated.
  static bool ExpandBuffer(const int32_t width, const int32_t height,
                           const FontMetrics &original_metrics,
                           const FontMetrics &new_metrics,
                           std::unique_ptr<uint8_t[]> *image);

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
  std::unordered_map<std::string, std::unique_ptr<FontTexture>> map_textures_;

  // Singleton instance of Freetype library.
  static FT_Library *ft_;

  // Harfbuzz buffer
  static hb_buffer_t *harfbuzz_buf_;
};

// Font texture class inherits Texture publicly.
// The class has additional properties for font metrics.
//
// For details of font metrics, refer http://support.microsoft.com/kb/32667
// In this class, ascender and descender values are retrieved from FreeType
// font property.
// And internal/external leading values are updated based on rendering glyph
// information. When rendering a string, the leading values tracks max (min for
// internal leading) value in the string.
class FontMetrics {
public:
  // Constructor
  FontMetrics()
    : base_line_(0), internal_leading_(0), ascender_(0), descender_(0),
      external_leading_(0) {}

  // Constructor with initialization parameters.
  FontMetrics(const int32_t base_line, const int32_t internal_leading,
              const int32_t ascender, const int32_t descender,
              const int32_t external_leading)
    : base_line_(base_line), internal_leading_(internal_leading),
      ascender_(ascender), descender_(descender),
      external_leading_(external_leading) {
    assert(internal_leading >= 0);
    assert(ascender >= 0);
    assert(descender <= 0);
    assert(external_leading <= 0);
  }
  ~FontMetrics() {
  }

  // Setter/Getter of the baseline value.
  int32_t base_line() const { return base_line_; }
  void set_base_line(const int32_t base_line) { base_line_ = base_line; }

  // Setter/Getter of the internal leading parameter.
  int32_t internal_leading() const { return internal_leading_; }
  void set_internal_leading(const int32_t internal_leading) {
    assert(internal_leading >= 0);
    internal_leading_ = internal_leading;
  }

  // Setter/Getter of the ascender value.
  int32_t ascender() const { return ascender_; }
  void set_ascender(const int32_t ascender) {
    assert(ascender >= 0);
    ascender_ = ascender;
  }

  // Setter/Getter of the descender value.
  int32_t descender() const { return descender_; }
  void set_descender(const int32_t descender) {
    assert(descender <= 0);
    descender_ = descender;
  }

  // Setter/Getter of the external leading value.
  int32_t external_leading() const { return external_leading_; }
  void set_external_leading(const int32_t external_leading) {
    assert(external_leading <= 0);
    external_leading_ = external_leading;
  }

  // Returns total height of the glyph.
  int32_t total() const { return internal_leading_ + ascender_
    - descender_ - external_leading_; }

private:
  // Baseline: Baseline of the glpyhs.
  // When rendering multiple glyphs in the same horizontal group,
  // baselines need be aligned.
  // Most of glyphs fit within the area of ascender + descender.
  // However some glyphs may render in internal/external leading area.
  // (e.g. Å, underlines)
  int32_t base_line_;

  // Internal leading: Positive value that describes the space above the
  // ascender.
  int32_t internal_leading_;

  // Ascender: Positive value that describes the size of the ascender above
  // the baseline.
  int32_t ascender_;

  // Descender: Negative value that describes the size of the descender below
  // the baseline.
  int32_t descender_;

  // External leading : Negative value that describes the space below
  // the descender.
  int32_t external_leading_;
};

// Font texture class
class FontTexture : public Texture {
public:
  FontTexture(Renderer &renderer, const std::string &filename)
    : Texture(renderer, filename) {}
  FontTexture(Renderer &renderer) : Texture(renderer) {}
  ~FontTexture() {}

  // Setter/Getter of the metrics parameter of the font texture.
  const FontMetrics &metrics() { return metrics_; }
  void set_metrics(const FontMetrics &metrics) { metrics_ = metrics; }

private:
  FontMetrics metrics_;
};

}  // namespace fpl

#endif // FONT_MANAGER_H
