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

#ifndef IMPEL_ID_MAP_H_
#define IMPEL_ID_MAP_H_

#include <assert.h>
#include <vector>

#include "impel_common.h"

namespace impel {

// Maps a unique 'id' to a data element. Keeps the data elements contiguous
// in memory, even when 'ids' are deleted. Only reallocates memory when a new
// highwater number of elements is reached.
template<class ImpelData>
class IdMap {
  // Invalid array index.
  typedef uint16_t DataIndex;
  static const DataIndex kInvalidIndex = UINT16_MAX;

 public:
  // Accessors.
  ImpelData& Data(ImpelId id) { return data_[Index(id)]; }
  const ImpelData& Data(ImpelId id) const { return data_[Index(id)]; }
  ImpelData* Begin() { return &data_[0]; }
  const ImpelData* Begin() const { return &data_[0]; }
  const ImpelData* End() const { return &data_[data_.size()]; }
  int Count() const { return static_cast<int>(data_.size()); }

  // Allocate data and associate a unique id to it. Note that the id may have
  // been an id that was previously used and then Free'd.
  ImpelId Allocate() {
    // Allocate a spot at the end of data_.
    const DataIndex index = static_cast<DataIndex>(data_.size());
    data_.resize(index + 1);

    // Allocate an id. We try to recycle ids first to avoid growing
    // id_to_index_.
    ImpelId id = 0;
    if (ids_to_recycle_.empty()) {
      // Allocate a new id from the end of id_to_index_.
      id = id_to_index_.size();
      id_to_index_.resize(id + 1);

    } else {
      // Grab id from the recycle list.
      id = ids_to_recycle_.back();
      ids_to_recycle_.pop_back();
    }

    //　Map id to that spot.
    id_to_index_[id] = index;
    return id;
  }

  // Free the data associated with 'id' by compacting the data_ array on top
  // of it. Return 'id' to the list of eligable ids to allocate.
  void Free(ImpelId id) {
    // Plug hole in data_.
    const int index = Index(id);
    const int last_index = data_.size() - 1;
    if (index != last_index) {
      const int last_id = Id(last_index);
      assert(last_id != kImpelIdInvalid);

      // Move last item in data_ to the index being deleted.
      data_[index] = data_[last_index];

      // Remap id onto the index we just moved to.
      id_to_index_[last_id] = index;
    }

    // Remove the last item from data_. It's no longer being used.
    data_.pop_back();

    // Mark the current id invalid.
    id_to_index_[id] = kInvalidIndex;

    // Reuse this id so that the id_to_index_ map doesn't keep growing.
    ids_to_recycle_.push_back(id);
  }

 private:
  // Returns the index corresponding to an id. Fast.
  int Index(ImpelId id) const {
    assert(0 <= id && id < id_to_index_.size());
    const int index = id_to_index_[id];
    assert(index != kInvalidIndex);
    return index;
  }

  // Returns an id corresponding to an index. Slow. Should be called
  // infrequently.
  ImpelId Id(int index) const {
    // Get id for the last index in data.
    for (int id = 0; id < id_to_index_.size(); ++id) {
      if (id_to_index_[id] == index)
        return id;
    }
    return kImpelIdInvalid;
  }

  // Map ImpelIds into the data_ array. Each id gets a unique index into data_.
  // This map may have holes. That is id_to_index_[id] may be kInvalidIndex.
  // When the map has a hole, that 'id' will be in ids_to_recycle_.
  // Note that this is a vector (not a map) because it requires very quick
  // access.
  std::vector<DataIndex> id_to_index_;

  // An unordered collection of 'id's that can be reused. We try to reuse ids
  // so that the id_to_index_ map doesn't grow without bound.
  std::vector<ImpelId> ids_to_recycle_;

  // A packed array of (template defined) data. There are no holes in this data.
  // No holes allows for good memory cohesion and possible optimizations.
  std::vector<ImpelData> data_;
};

} // namespace impel

#endif // IMPEL_ID_MAP_H_

