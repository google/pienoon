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

#ifndef GLYPH_CACH_H
#define GLYPH_CACH_H

#include <map>
#include <unordered_map>
#include <list>

#include "SDL_log.h"
#include "common.h"

namespace fpl {

// The glyph cache maintains a list of GlyphCacheRow. Each row has a fixed sizes
// of height, which is determined at a row creation time. A row can include
// multiple GlyphCacheEntry with a same or smaller height and they can have
// variable width. In a row,
// GlyphCacheEntry are stored from left to right in the order of registration
// and won't be evicted per entry, but entire row is flushed when necessary to
// make a room for new GlyphCacheEntry.
// The purpose of this design is to cache as many glyphs and to achieve high
// caching perfomance estimating same size of glphys tends to be stored in a
// cache at same time. (e.g. Caching a string in a same size.)
//
// When looking up a cached entry, the API looks up unordered_map which is O(1)
// operation.
// If there is no cached entry for given codepoint, the caller needs to invoke
// Set() API to fill in a cache.
// Set() operation takes
// O(log N (N=# of rows)) when there is a room in the cache for the request,
// + O(N (N=# of rows)) to look up and evict least recently used row with
// sufficient height.

// Enable tracking stats in Debug build.
#ifdef _DEBUG
#define GLYPH_CACHE_STATS (1)
#endif

// Forward decl.
template <typename T>
class GlyphCache;
class GlyphCacheRow;
class GlyphCacheEntry;

// Constants for a cache entry size rounding up and padding between glyphs.
// Adding a padding between cached glyph images to avoid sampling artifacts of
// texture fetches.
const int32_t kGlyphCacheHeightRound = 4;
const int32_t kGlyphCachePaddingX = 1;
const int32_t kGlyphCachePaddingY = 1;

// Cache entry for a glyph.
class GlyphCacheEntry {
 public:
  // Typedef for cache entry map's iterator.
  typedef std::unordered_map<uint64_t, GlyphCacheEntry>::iterator iterator;
  typedef std::list<GlyphCacheRow>::iterator iterator_row;

  GlyphCacheEntry() : codepoint_(0), size_(0, 0) {}

  // Setter/Getter of codepoint.
  // Codepoint is an entry in a font file, not a direct transform of Unicode.
  uint32_t get_codepoint() const { return codepoint_; }
  void set_codepoint(const uint32_t codepoint) { codepoint_ = codepoint; }

  // Setter/Getter of cache entry size.
  mathfu::vec2i get_size() const { return size_; }
  void set_size(const mathfu::vec2i& size) { size_ = size; }

  // Setter/Getter of UV
  mathfu::vec4 get_uv() const { return uv_; }
  void set_uv(const mathfu::vec4& uv) { uv_ = uv; }

 private:
  // Friend class, GlyphCache needs an access to internal variables of the
  // class.
  template <typename T>
  friend class GlyphCache;

  // Codepoint of the glyph.
  uint32_t codepoint_;

  // Cache entry sizes.
  mathfu::vec2i size_;

  // UV
  mathfu::vec4 uv_;

  // Iterator to the row entry.
  GlyphCacheEntry::iterator_row it_row;

  // Iterator to the row LRU entry.
  std::list<GlyphCacheEntry::iterator_row>::iterator it_lru_row_;
};

// Single row in a cache. A row correspond to a horizontal slice of a texture.
// (e.g if a texture has a 256x256 of size, and a row has a max glyph height of
// 16, the row corresponds to 256x16 pixels of the overall texture.)
//
// One cache row contains multiple GlyphCacheEntry with a same or smaller
// height. GlyphCacheEntry entries are stored from left to right and not evicted
// per glyph, but entire row at once for a performance reason.
// GlyphCacheRow is an internal class for GlyphCache.
class GlyphCacheRow {
 public:
  GlyphCacheRow() { Initialize(0, mathfu::vec2i(0, 0)); }
  // Constructor with an arguments.
  // y_pos : vertical position of the row in the buffer.
  // witdh : width of the row. Typically same value of the buffer width.
  // height : height of the row.
  GlyphCacheRow(const int32_t y_pos, const mathfu::vec2i& size) {
    Initialize(y_pos, size);
  }
  ~GlyphCacheRow() {}

  // Initialize the row width and height.
  void Initialize(const int32_t y_pos, const mathfu::vec2i& size) {
    last_used_counter_ = 0;
    y_pos_ = y_pos;
    remaining_width_ = size.x();
    size_ = size;
    cached_entries_.clear();
  }

  // Check if the row has a room for a requested width and height.
  bool DoesFit(const mathfu::vec2i& size) {
    return !(size.x() > remaining_width_ || size.y() > size_.y());
  }

  // Reserve an area in the row.
  int32_t Reserve(const GlyphCacheEntry::iterator it,
                  const mathfu::vec2i& size) {
    assert(DoesFit(size));

    // Update row info.
    int32_t pos = size.x() - remaining_width_;
    remaining_width_ -= size.x();
    cached_entries_.push_back(it);
    return pos;
  }

  // Setter/Getter of last used counter.
  uint32_t get_last_used_counter() { return last_used_counter_; }
  void set_last_used_counter(const uint32_t counter) {
    last_used_counter_ = counter;
  }

  // Setter/Getter of row size.
  mathfu::vec2i get_size() const { return size_; }
  void set_size(const mathfu::vec2i size) { size_ = size; }

  // Setter/Getter of row y pos.
  int32_t get_y_pos() const { return y_pos_; }
  void set_y_pos(const int32_t y_pos) { y_pos_ = y_pos; }

  // Getter of cached glyphs.
  int32_t get_num_glyphs() const { return cached_entries_.size(); }

  // Setter/Getter of iterator to row LRU.
  std::list<GlyphCacheEntry::iterator_row>::iterator get_it_lru_row() {
    return it_lru_row_;
  }
  void set_it_lru_row(
      const std::list<GlyphCacheEntry::iterator_row>::iterator it_lru_row) {
    it_lru_row_ = it_lru_row;
  }

  // Getter of cached glyph entries.
  std::vector<GlyphCacheEntry::iterator> get_cached_entries() {
    return cached_entries_;
  }

 private:
  // Last used counter value of the entry. The value is used to determine
  // if the entry can be evicted from the cache.
  uint32_t last_used_counter_;

  // Remaining width of the row.
  // As new contents are added to the row, remaining width decreases.
  int32_t remaining_width_;

  // Size of the row.
  mathfu::vec2i size_;

  // Vertical position of the row in the entire cache buffer.
  uint32_t y_pos_;

  // Iterator to the row LRU list.
  std::list<GlyphCacheEntry::iterator_row>::iterator it_lru_row_;

  // Tracking cached entries in the row.
  // When flushing the row, entries in the map is removed using the vector.
  std::vector<GlyphCacheEntry::iterator> cached_entries_;
};

template <typename T>
class GlyphCache {
 public:
  // Constructor with parameters.
  // width: width of the glyph cache texture. Rounded up to power of 2.
  // height: height of the glyph cache texture. Rounded up to power of 2.
  GlyphCache(mathfu::vec2i& size) {
    // Round up cache sizes to power of 2.
    size_.x() = mathfu::RoundUpToPowerOf2(size.x());
    size_.y() = mathfu::RoundUpToPowerOf2(size.y());

    // Allocate the glyph cache buffer.
    // A buffer format can be 8/32 bpp (32 bpp is mostly used for Emoji).
    buffer_.reset(new T[size_.x() * size_.y()]);
    memset(buffer_.get(), 0, size_.x() * size_.y() * sizeof(T));

    // Create first (empty) row entry.
    InsertNewRow(0, size_, list_row_.end());

#ifdef GLYPH_CACHE_STATS
    ResetStats();
#endif
  }
  ~GlyphCache() {};

  // Look up a cached entries.
  // Return value: A pointer to a cached glyph entry.
  // nullptr if not found.
  const GlyphCacheEntry* Find(const uint32_t codepoint, const int32_t y_size) {
#ifdef GLYPH_CACHE_STATS
    // Update debug variable.
    stats_lookup_++;
#endif
    auto it =
        map_entries_.find(static_cast<uint64_t>(codepoint) << 32 | y_size);
    if (it != map_entries_.end()) {
      // Found an entry!

      // Mark the row as being used in current cycle.
      it->second.it_row->set_last_used_counter(counter_);

      // Update row LRU entry. The row is now most recently used.
      lru_row_.splice(lru_row_.end(), lru_row_, it->second.it_lru_row_);

#ifdef GLYPH_CACHE_STATS
      // Update debug variable.
      stats_hit_++;
#endif
      return &it->second;
    }

    // Didn't find a cached entry. A caller may call Store() function to store
    // new entriy to the cache.
    return nullptr;
  }

  // Set an entry to the cache.
  // Return value: true if caching succeeded. false if there is no room in the
  // cache for a requested entry.
  bool Set(const T* const image, const int32_t y_size, GlyphCacheEntry* entry) {
    // Lookup entries if the entry is already stored in the cache.
    auto p = Find(entry->get_codepoint(), y_size);
#ifdef GLYPH_CACHE_STATS
    // Adjust debug variable.
    stats_lookup_--;
#endif
    if (p) {
      // Make sure cached entry has same properties.
      // The cache only support one entry per a glyph codepoint for now.
      assert(p->get_size().x() == entry->get_size().x());
      assert(p->get_size().y() == entry->get_size().y());
#ifdef GLYPH_CACHE_STATS
      // Adjust debug variable.
      stats_hit_--;
#endif
      return true;
    }

    // Adjust requested height & width.
    // Height is rounded up to multiple of kGlyphCacheHeightRound.
    // Expecting kGlyphCacheHeightRound is base 2.
    int32_t req_width = entry->get_size().x() + kGlyphCachePaddingX;
    int32_t req_height = ((entry->get_size().y() + kGlyphCachePaddingY +
                           (kGlyphCacheHeightRound - 1)) &
                          ~(kGlyphCacheHeightRound - 1));

    // Look up the row map to retrieve a row iterator to start with.
    auto it = map_row_.lower_bound(req_height);
    while (it != map_row_.end()) {
      if (it->second->DoesFit(mathfu::vec2i(req_width, req_height))) {
        break;
      }
      it++;
    }

    if (it != map_row_.end()) {
      // Found sufficient space in the buffer.
      auto it_row = it->second;

      if (it_row->get_num_glyphs() == 0) {
        // Putting first entry to the row.
        // In this case, we create new empty row to track rest of free space.
        auto original_height = it_row->get_size().y();
        auto original_y_pos = it_row->get_y_pos();

        if (original_height - req_height >= kGlyphCacheHeightRound) {
          // Create new row for free space.
          it_row->set_size(mathfu::vec2i(size_.x(), req_height));
          InsertNewRow(original_y_pos + req_height,
                       mathfu::vec2i(size_.x(), original_height - req_height),
                       list_row_.end());
        }
      }

      // Create new entry in the look-up map.
      auto pair = map_entries_.insert(std::pair<uint64_t, GlyphCacheEntry>(
          static_cast<uint64_t>(entry->get_codepoint()) << 32 | y_size,
          *entry));
      auto it_entry = pair.first;

      // Reserve a region in the row.
      auto pos = mathfu::vec2i(
          it_row->Reserve(it_entry, mathfu::vec2i(req_width, req_height)),
          it_row->get_y_pos());

      // Store given image into the buffer.
      CopyImage(pos, image, entry);

      // TODO: Update texture atlas dirty rect.

      // Update UV of the entry.
      auto uv1 = pos / size_;
      auto uv2 = (pos + entry->get_size()) / size_;
      auto p = mathfu::vec4(uv1.x(), uv1.y(), uv2.x(), uv2.y());
      it_entry->second.set_uv(p);

      // Establish links.
      it_entry->second.it_row = it_row;
      it_entry->second.it_lru_row_ = it_row->get_it_lru_row();

      // Update row LRU entry.
      lru_row_.splice(lru_row_.end(), lru_row_, it_row->get_it_lru_row());
      it_row->set_last_used_counter(counter_);

    } else {
      // Couldn't find sufficient row entry nor free space to create new row.

      // Try to find a row that is not used in current cycle and has enough
      // height from LRU list.
      for (auto row : lru_row_) {
        if (row->get_last_used_counter() == counter_) {
          // Rows are used in current cycle. We can not evict any of rows.
          break;
        }
        if (row->get_size().y() <= req_width) {
          // Now flush & initialize the row.
          FlushRow(row);
          row->Initialize(row->get_y_pos(), row->get_size());

          // Call the function recursively.
          return Set(image, y_size, entry);
        }
      }
#ifdef GLYPH_CACHE_STATS
      stats_set_fail_++;
#endif
      // TODO: Try to flush multiple rows and merge them to free up space.
      // Now we don't have any space in the cache.
      // It's caller's responsivility to recover from the situation.
      // Possible work arounds are:
      // - Draw glyphs with current glyph cache contents and then flush them,
      // start new caching.
      // - Just increase cache size.
      return false;
    }

    return true;
  }

  // Flush all cache entries.
  bool Flush() {
#ifdef GLYPH_CACHE_STATS
    ResetStats();
#endif
    map_entries_.clear();
    lru_row_.clear();
    list_row_.clear();
    map_row_.clear();

    // Create first (empty) row entry.
    InsertNewRow(0, size_, list_row_.end());
    return true;
  }

  // Update internal counter, check dierty rect and update OpenGL texture if
  // necessary.
  void Update() {
    counter_++;
    // TODO: Upload OpenGL texture and clear a dirty rect.
  }

  // Debug API to show cache statistics.
  void Status() {
#ifdef GLYPH_CACHE_STATS
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cache size: %dx%d", size_.x(),
                size_.y());
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cache hit: %d / %d", stats_hit_,
                stats_lookup_);

    auto total_glyph = 0;
    for (auto row : list_row_) {
      SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                  "Row start:%d height:%d glyphs:%d", row.get_y_pos(),
                  row.get_size().y(), row.get_num_glyphs());
      total_glyph += row.get_num_glyphs();
    }
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Cached glyphs: %d", total_glyph);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Row flush: %d",
                stats_row_flush_);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Set fail: %d", stats_set_fail_);
#endif
  }

 private:
  // Insert new row to the row list with a given size.
  // It tries to merge 2 rows if next row is also empty one.
  void InsertNewRow(const int32_t y_pos, const mathfu::vec2i& size,
                    const GlyphCacheEntry::iterator_row pos) {
    // First, check if we can merge the requested row with next row to free up
    // more spaces.
    // New row is always inserted right after valid row entry. So we don't have
    // to check previous row entry to merge.
    if (pos != list_row_.end()) {
      auto next_entry = std::next(pos);
      if (next_entry->get_num_glyphs() == 0) {
        // We can merge them.
        mathfu::vec2i next_size = next_entry->get_size();
        next_size.y() += size.y();
        next_entry->set_y_pos(next_entry->get_y_pos() - size.y());
        next_entry->set_size(next_size);
        next_entry->set_last_used_counter(counter_);
        return;
      }
    }

    // Insert new row.
    auto it = list_row_.insert(pos, GlyphCacheRow(y_pos, size));
    auto it_lru_row = lru_row_.insert(lru_row_.end(), it);
    map_row_.insert(
        std::pair<int32_t, GlyphCacheEntry::iterator_row>(size.y(), it));

    // Update a link.
    it->set_it_lru_row(it_lru_row);
  }

  void FlushRow(const GlyphCacheEntry::iterator_row row) {
    // Erase cached glyphs from look-up map.
    for (auto entry : row->get_cached_entries()) {
      map_entries_.erase(entry);
    }
#ifdef GLYPH_CACHE_STATS
    stats_row_flush_++;
#endif
  }

#ifdef GLYPH_CACHE_STATS
  void ResetStats() {
    // Initialize debug variables.
    stats_hit_ = 0;
    stats_lookup_ = 0;
    stats_row_flush_ = 0;
    stats_set_fail_ = 0;
  }
#endif

  void CopyImage(const mathfu::vec2i& pos, const T* const image,
                 const GlyphCacheEntry* entry) {
    for (int32_t y = 0; y < entry->get_size().y(); ++y) {
      for (int32_t x = 0; x < entry->get_size().x(); ++x) {
        buffer_.get()[pos.x() + x + (pos.y() + y) * size_.x()] =
            image[x + y * entry->get_size().x()];
      }
    }
  }

  // A time counter of the cache.
  // In each rendering cycle, the counter is incremented.
  // The counter is used if some cache entry can be evicted in current rendering
  // cycle.
  uint32_t counter_;

  // Size of the glyph cache. Rounded to power of 2.
  mathfu::vec2i size_;

  // Cache buffer;
  std::unique_ptr<T> buffer_;

  // Hash map to the cache entries
  // This map is the primary place to look up the cache entries.
  // Key: a combination of a codepoint in a font file and glyph size.
  // Note that the codepoint is an index in the
  // font file and not a Unicode value.
  std::unordered_map<uint64_t, GlyphCacheEntry> map_entries_;

  // list of rows in the cache.
  std::list<GlyphCacheRow> list_row_;

  // LRU entries of the row. Tracks iterator to list_row_.
  std::list<GlyphCacheEntry::iterator_row> lru_row_;

  // Map to row entries to have O(log N) access to a row entry.
  // Tracks iterator to list_row_.
  // Using multimap because multiple rows can have same row height.
  // Key: height of the row. With the map, an API can have quick access to a row
  // with a given height.
  std::multimap<int32_t, GlyphCacheEntry::iterator_row> map_row_;

#ifdef GLYPH_CACHE_STATS
  // Variables to track usage stats.
  int32_t stats_lookup_;
  int32_t stats_hit_;
  int32_t stats_row_flush_;
  int32_t stats_set_fail_;
#endif
};

}  // namespace fpl

#endif  // GLYPH_CACH_H
