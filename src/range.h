// Copyright 2015 Google Inc. All rights reserved.
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

#ifndef FPL_RANGE_H_
#define FPL_RANGE_H_

#include <vector>
#include "mathfu/utilities.h"

namespace fpl {

/// Represent an interval on a number line.
template <class T>
class RangeT {
 public:
  template <size_t kMaxLen>
  struct TArray {
    size_t len;
    T arr[kMaxLen];
  };

  template <size_t kMaxLen>
  struct RangeArray {
    size_t len;
    RangeT<T> arr[kMaxLen];
  };

  // By default, initialize to an invalid range.
  RangeT() : start_(static_cast<T>(1)), end_(static_cast<T>(0)) {}
  RangeT(const T start, const T end) : start_(start), end_(end) {}

  // A range is valid if it contains at least one number.
  bool Valid() const { return start_ <= end_; }

  // Returns the mid-point of the range, rounded down for integers.
  // Behavior is undefined for invalid regions.
  T Middle() const { return (start_ + end_) / static_cast<T>(2); }

  // Returns the span of the range. Returns 0 when only one number in range.
  // Behavior is undefined for invalid regions.
  T Length() const { return end_ - start_; }

  // Returns 'x' if it is within the range. Otherwise, returns start_ or end_,
  // whichever is closer to 'x'.
  // Behavior is undefined for invalid regions.
  T Clamp(const T x) const { return mathfu::Clamp(x, start_, end_); }

  // Returns distances outside of the range. If inside the range, returns 0.
  // Behavior is undefined for invalid regions.
  T DistanceFrom(const T x) const { return fabs(x - Clamp(x)); }

  // Lerps between the start and the end.
  // 'percent' of 0 returns start. 'percent' of 1 returns end.
  // Behavior is undefined for invalid regions.
  T Lerp(const float percent) const {
    return mathfu::Lerp(start_, end_, percent);
  }

  // Returns percent 0~1, from start to end. *Not* clamped to 0~1.
  // 0 ==> start;  1 ==> end;  0.5 ==> Middle();  -1 ==> start - Length()
  float Percent(const T x) const { return (x - start_) / Length(); }

  // Returns percent 0~1, from start to end. Clamped to 0~1.
  // 0 ==> start or earlier;  1 ==> end or later;  0.5 ==> Middle()
  T PercentClamped(const T x) const {
    return mathfu::Clamp(Percent(x), 0.0f, 1.0f);
  }

  bool Contains(const T x) const { return start_ <= x && x <= end_; }

  // Swap start and end. When 'a' and 'b' don't overlap, if you invert the
  // return value of Range::Intersect(a, b), you'll get the gap between
  // 'a' and 'b'.
  RangeT Invert() const { return RangeT(end_, start_); }

  // Returns a range that is 'percent' longer. If 'percent' is < 1.0, then
  // returned range will actually be shorter.
  RangeT Lengthen(const float percent) const {
    const T extra = static_cast<T>(Length() * percent * 0.5f);
    return RangeT(start_ - extra, end_ + extra);
  }

  // Equality is strict. No epsilon checking here.
  bool operator==(const RangeT& rhs) const {
    return start_ == rhs.start_ && end_ == rhs.end_;
  }
  bool operator!=(const RangeT& rhs) const { return !operator==(rhs); }

  // Accessors.
  T start() const { return start_; }
  T end() const { return end_; }
  void set_start(const T start) { start_ = start; }
  void set_end(const T end) { end_ = end; }

  // Return the overlap of 'a' and 'b', or an invalid range if they do not
  // overlap at all.
  // When 'a' and 'b' don't overlap at all, calling Invert on the returned
  // range will give the gap between 'a' and 'b'.
  static RangeT Intersect(const RangeT& a, const RangeT& b) {
    // Possible cases:
    // 1.  |-a---|    |-b---|  ==>  return invalid
    // 2.  |-b---|    |-a---|  ==>  return invalid
    // 3.  |-a---------|       ==>  return b
    //        |-b---|
    // 4.  |-b---------|       ==>  return a
    //        |-a---|
    // 5.  |-a---|             ==>  return (b.start, a.end)
    //        |-b---|
    // 6.  |-b---|             ==>  return (a.start, b.end)
    //        |-a---|
    //
    // All satisfied by,
    //   intersection.start = max(a.start, b.start)
    //   intersection.end = min(a.end, b.end)
    // Note that ranges where start > end are considered invalid.
    return RangeT(std::max(a.start_, b.start_), std::min(a.end_, b.end_));
  }

  // Only keep entries in 'values' if they are in
  // (range.start - epsition, range.end + epsilon).
  // Any values that are kept are clamped to 'range'.
  //
  // This function is useful when floating point precision error might put a
  // value slightly outside 'range' even though mathematically it should be
  // inside 'range'. This often happens with values right on the border of the
  // valid range.
  static size_t ValuesInRange(const RangeT& range, T epsilon, size_t num_values,
                              T* values) {
    size_t num_returned = 0;
    for (size_t i = 0; i < num_values; ++i) {
      const T value = values[i];
      const T clamped = range.Clamp(value);
      const T dist = fabs(value - clamped);

      // If the distance from the range is small, keep the clamped value.
      if (dist <= epsilon) {
        values[num_returned++] = clamped;
      }
    }
    return num_returned;
  }

  template <size_t kMaxLen>
  static void ValuesInRange(const RangeT& range, T epsilon,
                            TArray<kMaxLen>* values) {
    values->len = ValuesInRange(range, epsilon, values->len, values->arr);
  }

  // Intersect every element of 'a' with every element of 'b'. Append
  // intersections to 'intersections'. Note that 'intersections' is not reset at
  // the start of the call.
  static size_t IntersectRanges(const RangeT* a, size_t len_a, const RangeT* b,
                                size_t len_b, RangeT* intersections,
                                RangeT* gaps = nullptr,
                                size_t* len_gaps = nullptr) {
    size_t num_intersections = 0;
    size_t num_gaps = 0;

    for (size_t i = 0; i < len_a; ++i) {
      for (size_t j = 0; j < len_b; ++j) {
        const RangeT intersection = RangeT::Intersect(a[i], b[j]);
        if (intersection.Valid()) {
          intersections[num_intersections++] = intersection;

        } else if (gaps != nullptr) {
          // Return the gaps, too, if requested. Invert() invalid intersections
          // to get the gap between the ranges.
          gaps[num_gaps++] = intersection.Invert();
        }
      }
    }

    // Set return values.
    if (len_gaps != nullptr) {
      *len_gaps = num_gaps;
    }
    return num_intersections;
  }

  template <size_t kMaxLen>
  static void IntersectRanges(const RangeArray<kMaxLen>& a,
                              const RangeArray<kMaxLen>& b,
                              RangeArray<kMaxLen * kMaxLen>* intersections,
                              RangeArray<kMaxLen * kMaxLen>* gaps = nullptr) {
    const bool use_gaps = gaps != nullptr;
    intersections->len = IntersectRanges(
        a.arr, a.len, b.arr, b.len, intersections->arr,
        use_gaps ? gaps->arr : nullptr, use_gaps ? &gaps->len : nullptr);
  }

  // Return the index of the longest range in 'ranges'.
  static size_t IndexOfLongest(const RangeT* ranges, size_t len) {
    T longest_length = -1.0f;
    size_t longest_index = 0;
    for (size_t i = 0; i < len; ++i) {
      const T length = ranges[i].Length();
      if (length > longest_length) {
        longest_length = length;
        longest_index = i;
      }
    }
    return longest_index;
  }

  template <size_t kMaxLen>
  static size_t IndexOfLongest(const RangeArray<kMaxLen>& ranges) {
    return IndexOfLongest(ranges.arr, ranges.len);
  }

  // Return the index of the shortest range in 'ranges'.
  static size_t IndexOfShortest(const RangeT* ranges, size_t len) {
    T shortest_length = std::numeric_limits<T>::infinity();
    size_t shortest_index = 0;
    for (size_t i = 0; i < len; ++i) {
      const T length = ranges[i].Length();
      if (length < shortest_length) {
        shortest_length = length;
        shortest_index = i;
      }
    }
    return shortest_index;
  }

  template <size_t kMaxLen>
  static size_t IndexOfShortest(const RangeArray<kMaxLen>& ranges) {
    return IndexOfShortest(ranges.arr, ranges.len);
  }

 private:
  T start_;  // Start of the range. Range is valid if start_ <= end_.
  T end_;    // End of the range. Range is inclusive of start_ and end_.
};

template <class T>
RangeT<T> CreateValidRange(const T a, const T b) {
  return RangeT<T>(std::min<T>(a, b), std::max<T>(a, b));
}

// Instantiate for various scalar.
typedef RangeT<float> RangeFloat;
typedef RangeT<double> RangeDouble;
typedef RangeT<int> RangeInt;
typedef RangeT<unsigned int> RangeUInt;

// Since the float specification will be most common, we give it a simple name.
typedef RangeFloat Range;

}  // namespace fpl

#endif  // FPL_RANGE_H_
