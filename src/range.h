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
template<class T>
class RangeT {
 public:
  typedef std::vector<T> TVector;
  typedef std::vector<RangeT<T>> RangeVector;

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
  T DistanceFrom(const T x) const { return fabs(x - Clamp(x)); }

  // Swap start and end. When 'a' and 'b' don't overlap, if you invert the
  // return value of Range::Intersect(a, b), you'll get the gap between
  // 'a' and 'b'.
  RangeT Invert() const { return RangeT(end_, start_); }

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
  static void ValuesInRange(const RangeT& range, T epsilon, TVector* values) {
    size_t num_returned = 0;
    for (size_t i = 0; i < values->size(); ++i) {
      const T value = (*values)[i];
      const T clamped = range.Clamp(value);
      const T dist = fabs(value - clamped);

      // If the distance from the range is small, keep the clamped value.
      if (dist <= epsilon) {
        (*values)[num_returned++] = clamped;
      }
    }

    // Size can only get smaller, so realloc is never called.
    values->resize(num_returned);
  }

  // Intersect every element of 'a' with every element of 'b'. Append
  // intersections to 'intersections'. Note that 'intersections' is not reset at
  // the start of the call.
  static void IntersectRanges(const RangeVector& a, const RangeVector& b,
                              RangeVector* intersections,
                              RangeVector* gaps = nullptr) {
    for (size_t i = 0; i < a.size(); ++i) {
      for (size_t j = 0; j < b.size(); ++j) {

        const RangeT intersection = RangeT::Intersect(a[i], b[j]);
        if (intersection.Valid()) {
          intersections->push_back(intersection);

        } else if (gaps != nullptr) {
          // Return the gaps, too, if requested. Invert() invalid intersections
          // to get the gap between the ranges.
          gaps->push_back(intersection.Invert());
        }
      }
    }
  }

  // Return the index of the longest range in 'ranges'.
  static int IndexOfLongest(const RangeVector& ranges) {
    T longest_length = -1.0f;
    int longest_index = 0;
    for (int i = 0; i < static_cast<int>(ranges.size()); ++i) {
      const T length = ranges[i].Length();
      if (length > longest_length) {
        longest_length = length;
        longest_index = i;
      }
    }
    return longest_index;
  }

  // Return the index of the shortest range in 'ranges'.
  static int IndexOfShortest(const RangeVector& ranges) {
    T shortest_length = std::numeric_limits<T>::infinity();
    int shortest_index = 0;
    for (int i = 0; i < static_cast<int>(ranges.size()); ++i) {
      const T length = ranges[i].Length();
      if (length < shortest_length) {
        shortest_length = length;
        shortest_index = i;
      }
    }
    return shortest_index;
  }

 private:
  T start_; // Start of the range. Range is valid if start_ <= end_.
  T end_;   // End of the range. Range is inclusive of start_ and end_.
};

// Instantiate for various scalar.
typedef RangeT<float> RangeFloat;
typedef RangeT<double> RangeDouble;
typedef RangeT<int> RangeInt;
typedef RangeT<unsigned int> RangeUInt;

// Since the float specification will be most common, we give it a simple name.
typedef RangeFloat Range;


} // namespace fpl

#endif // FPL_RANGE_H_

