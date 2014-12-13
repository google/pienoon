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

#include "precompiled.h"

// Freetype2 header
#include <ft2build.h>
#include FT_FREETYPE_H

// Harfbuzz header
#include <hb.h>
#include <hb-ft.h>

#include "font_manager.h"
#include "utilities.h"

namespace fpl {

// Singleton object of FreeType&Harfbuzz.
FT_Library *FontManager::ft_;
hb_buffer_t *FontManager::harfbuzz_buf_;

FontManager::FontManager() : renderer_(nullptr), face_initialized_(false) {
  Initialize();
}

FontManager::~FontManager() {
  Close();
}

void FontManager::Initialize() {
  assert(ft_ == nullptr);
  ft_ = new FT_Library;
  FT_Error err;
  if ((err = FT_Init_FreeType(ft_)))
  {
    // Error! Please fix me.
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't initialize freetype. FT_Error:%d\n", err);
    assert(0);
  }

  // Create a buffer for harfbuzz.
  harfbuzz_buf_ = hb_buffer_create();
}

void FontManager::Terminate() {
  assert(ft_ != nullptr);
  hb_buffer_destroy(harfbuzz_buf_);
  harfbuzz_buf_ = nullptr;

  FT_Done_FreeType(*ft_);
  ft_ = nullptr;
}

Texture* FontManager::GetTexture(const char *text, const float ysize) {
  // Check cache if we already have a texture.
  auto t = map_textures_.find(text);
  if (t != map_textures_.end())
    return t->second.get();

  // Otherwise, create new texture.
  // TODO: Update the implemenation using
  // texture atlas + font cache + vbo for a better performance.

  // Set freetype settings.
  FT_Set_Pixel_Sizes(face_, 0, ysize);

  size_t length = strlen(text);

  // TODO: make harfbuzz settings (and other font settings) configurable.
  // Set harfbuzz settings.
  hb_buffer_set_direction(harfbuzz_buf_, HB_DIRECTION_LTR);
  hb_buffer_set_script(harfbuzz_buf_, HB_SCRIPT_LATIN);
  hb_buffer_set_language(harfbuzz_buf_, hb_language_from_string(text, length));

  // Layout the text.
  hb_buffer_add_utf8(harfbuzz_buf_, text, length, 0, length);
  hb_shape(harfbuzz_font_, harfbuzz_buf_, nullptr, 0);

  // Retrieve layout info.
  uint32_t glyph_count;
  hb_glyph_info_t *glyph_info
    = hb_buffer_get_glyph_infos(harfbuzz_buf_, &glyph_count);
  hb_glyph_position_t *glyph_pos
    = hb_buffer_get_glyph_positions(harfbuzz_buf_, &glyph_count);

  // Retrieve a width of the string.
  uint32_t string_width = 0;
  for (uint32_t i = 0; i < glyph_count; ++i) {
    string_width += glyph_pos[i].x_advance;
  }
  string_width /= kFreeTypeUnit;

  // TODO: retrieve glyph metrices from FreeType
  // and don't generate texture for each label.
  int32_t width = mathfu::RoundUpToPowerOf2(string_width);
  int32_t height = mathfu::RoundUpToPowerOf2(ysize);
  int32_t base = 18;

  // rasterized image format in FreeType is 8 bit gray scale format.
  std::unique_ptr<uint8_t[]> image(new uint8_t[width * height]);
  memset(image.get(), 0, width * height);

  // TODO: make padding values configurable.
  uint32_t kGlyphPadding = 0;
  uint32_t atlasX = kGlyphPadding;
  uint32_t atlasY = kGlyphPadding;
  FT_GlyphSlot g = face_->glyph;

  for (size_t i = 0; i < glyph_count; ++i) {
    FT_Error err;

    // Load glyph using harfbuzz layout information.
    // Note that harfbuzz takes care of ligatures.
    if ((err = FT_Load_Glyph(face_, glyph_info[i].codepoint, FT_LOAD_RENDER))) {
      // Error. This could happen typically the loaded font does not support
      // particular glyph.
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Can't load glyph %c FT_Error:%d\n", text[i], err);
      return nullptr;
    }

    if ((atlasX + g->bitmap.width + g->bitmap_left) >= (width - kGlyphPadding)) {
      atlasY += ysize + kGlyphPadding;
      atlasX = kGlyphPadding;
    }

    if (atlasY + base + g->bitmap.rows - g->bitmap_top
          >= (height - kGlyphPadding)) {
      // Error exceeding the texture size.
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "The specified text does not fit into the texture.\n");
      return nullptr;
    }

    // Copy the texture
    for (int32_t y = 0; y < g->bitmap.rows; ++y) {
      for (int32_t x = 0; x < g->bitmap.width; ++x) {
        int32_t y_offset = base - g->bitmap_top;
        if (y_offset + y >= 0)
        {
          image[ (atlasY + y + y_offset) * width + atlasX + x + g->bitmap_left]
            = g->bitmap.buffer[ y * g->bitmap.width + x ];
        }
      }
    }

    // Advance the atlas position.
    atlasX += glyph_pos[i].x_advance / kFreeTypeUnit + kGlyphPadding;
    atlasY -= glyph_pos[i].y_advance / kFreeTypeUnit + kGlyphPadding;
  }

  // Create new texture.
  Texture *tex = new Texture(*renderer_);
  tex->LoadFromMemory(image.get(),
                          vec2i(width, height), kFormatLuminance, false);

  // Setup UV.
  tex->set_uv(vec4(0.0f, 0.0f,
                   static_cast<float>(string_width) / static_cast<float>(width),
                   static_cast<float>(ysize) / static_cast<float>(height)));

  // Cleanup buffer contents.
  hb_buffer_clear_contents(harfbuzz_buf_);

  // Put to the dic.
  map_textures_[text] = std::unique_ptr<Texture>(tex);

  return tex;
}

bool FontManager::Open(const char *font_name) {
  assert(!face_initialized_);

  // Load the font file of assets.
  if (!LoadFile(font_name, &font_data_)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't load font reource: %s\n", font_name);
    return false;
  }

  // Open the font.
  FT_Error err;
  if ((err = FT_New_Memory_Face(*ft_,
        reinterpret_cast<const unsigned char*>(&font_data_[0]),
        font_data_.size(), 0, &face_ ))) {
    // Failed to open font.
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Failed to initialize font:%s FT_Error:%d\n", font_name, err);
    return false;
  }

  // Create harfbuzz font information from freetype face.
  harfbuzz_font_ = hb_ft_font_create(face_, NULL);
  if (!harfbuzz_font_) {
    // Failed to open font.
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Failed to initialize harfbuzz layout information:%s\n",
                 font_name);
    font_data_.clear();
    FT_Done_Face(face_);
    return false;
  }

  face_initialized_ = true;
  return true;
}

bool FontManager::Close() {
  if (!face_initialized_)
    return false;

  map_textures_.clear();

  hb_font_destroy(harfbuzz_font_);

  FT_Done_Face(face_);

  font_data_.clear();

  face_initialized_ = false;

  return true;
}

}  // namespace fpl

