/*
* Copyright (c) 2014 Google, Inc.
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#include "gtest/gtest.h"
#include "glyph_cache.h"
#include "common.h"

class FontManagerTests : public ::testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

// Quick test to initialize the cache.
TEST_F(FontManagerTests, Glyph_Cache_Initialize) {
  mathfu::vec2i cache_size = mathfu::vec2i(256, 256);
  int32_t image_width = 31;
  int32_t image_height = 31;
  // Initialize Glyph cache
  std::unique_ptr<fpl::GlyphCache<uint8_t>> cache(
      new fpl::GlyphCache<uint8_t>(cache_size));
  std::unique_ptr<uint8_t[]> image(new uint8_t[image_width * image_height]);

  // Set some value
  fpl::GlyphCacheEntry entry;
  entry.set_size(mathfu::vec2i(image_width, image_height));
  cache->Set(image.get(), image_height, &entry);

  cache->Status();

  EXPECT_EQ(cache, cache);
}

// Test to
// 1) Create a cache (256x256)
// 2) Fill the cache with 32x32 entries.
// 3) Access them and see if there is no cache miss.
TEST_F(FontManagerTests, Glyph_Cache_SimpleEntries) {
  mathfu::vec2i cache_size = mathfu::vec2i(256, 256);
  int32_t image_width = 31;
  int32_t image_height = 31;

  // Initialize Glyph cache
  std::unique_ptr<fpl::GlyphCache<uint8_t>> cache(
      new fpl::GlyphCache<uint8_t>(cache_size));
  std::unique_ptr<uint8_t[]> image(new uint8_t[image_width * image_height]);

  // Set some value
  fpl::GlyphCacheEntry entry;
  entry.set_size(mathfu::vec2i(image_width, image_height));

  // Fill cache
  int32_t k = 0;
  for (int32_t i = 0;
       i < cache_size.y() / (image_height + fpl::kGlyphCachePaddingY); ++i)
    for (int32_t j = 0;
         j < cache_size.x() / (image_width + fpl::kGlyphCachePaddingX); ++j) {
      entry.set_codepoint(k);
      cache->Set(image.get(), image_height, &entry);
      k++;
    }

  // Look up
  k = 0;
  for (int32_t i = 0;
       i < cache_size.y() / (image_height + fpl::kGlyphCachePaddingY); ++i)
    for (int32_t j = 0;
         j < cache_size.x() / (image_width + fpl::kGlyphCachePaddingX); ++j) {
      cache->Find(k, image_height);
      k++;
    }

  cache->Status();

  EXPECT_EQ(cache, cache);
}

// Test to
// 1) Create a cache (256x256)
// 2) Fill the cache with 32x32 entries.
// 3) Access them and create new entry if there is no cached entry 128 times.
// 4) See if cache misses & row flush is is correct.
TEST_F(FontManagerTests, Glyph_Cache_InvolveEviction) {
  mathfu::vec2i cache_size = mathfu::vec2i(256, 256);
  int32_t image_width = 31;
  int32_t image_height = 31;

  // Initialize Glyph cache
  std::unique_ptr<fpl::GlyphCache<uint8_t>> cache(
      new fpl::GlyphCache<uint8_t>(cache_size));
  std::unique_ptr<uint8_t[]> image(new uint8_t[image_width * image_height]);

  // Set some value
  fpl::GlyphCacheEntry entry;
  entry.set_size(mathfu::vec2i(image_width, image_height));

  // Fill cache
  int32_t k = 0;
  for (int32_t i = 0;
       i < cache_size.y() / (image_height + fpl::kGlyphCachePaddingY); ++i)
    for (int32_t j = 0;
         j < cache_size.x() / (image_width + fpl::kGlyphCachePaddingX); ++j) {
      entry.set_codepoint(k);
      cache->Set(image.get(), image_height, &entry);
      k++;
    }

  // Look up
  k = 0;
  for (int32_t x = 0; x < 2; ++x)
    for (int32_t i = 0;
         i < cache_size.y() / (image_height + fpl::kGlyphCachePaddingY); ++i)
      for (int32_t j = 0;
           j < cache_size.x() / (image_width + fpl::kGlyphCachePaddingX); ++j) {
        // Increment counter.
        cache->Update();
        auto p = cache->Find(k, image_height);
        if (p == nullptr) {
          entry.set_codepoint(k);
          cache->Set(image.get(), image_height, &entry);
        }
        k++;
      }

  cache->Status();

  EXPECT_EQ(cache, cache);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
