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

#ifndef FPL_COMPACT_SPLINE_H_
#define FPL_COMPACT_SPLINE_H_

#include "common.h"
#include "curve.h"

namespace fpl {

class CompactSplineNode;

// X-axis is quantized into units of 'x_granularity'. X values are represented
// by multiples of 'x_granularity'. One unit of CompactSplineXGrain represents
// one multiple of 'x_granularity'.
typedef uint16_t CompactSplineXGrain;

// Y values within 'y_range' can be represented. We quantize the 'y_range'
// into equally-sized rungs, and round to the closest rung.
typedef uint16_t CompactSplineYRung;

// Angles strictly between -90 and +90 can be represented. We record the angle
// instead of the slope for more uniform distribution.
typedef int16_t CompactSplineAngle;

// Index into the spline. Some high values have special meaning (see below).
typedef uint16_t CompactSplineIndex;
static const CompactSplineIndex kInvalidSplineIndex =
    static_cast<CompactSplineIndex>(-1);
static const CompactSplineIndex kBeforeSplineIndex =
    static_cast<CompactSplineIndex>(-2);
static const CompactSplineIndex kAfterSplineIndex =
    static_cast<CompactSplineIndex>(-3);

static bool OutsideSpline(CompactSplineIndex index) {
  return index >= kAfterSplineIndex;
}

enum CompactSplineAddMethod {
  kAddWithoutModification,  // Add node straight-up. No changes.
  kEnsureCubicWellBehaved,  // Insert an intermediate node, if required,
                            // to ensure cubic splines have uniform curvature.
};

// A spline interpolates a series of (x, y, derivative) nodes to create a
// smooth curve.
//
// This class holds a series of such nodes, and aids with the construction of
// that series by inserting extra nodes when extra smoothness is required.
//
// The data in this class is compacted as quantized values. It's not intended
// to be read directly. You should use the BulkSplineEvaluator to update
// and read values from the splines in a performant manner.
class CompactSpline {
 public:
  CompactSpline();
  CompactSpline(const Range& y_range, const float x_granularity,
                const int num_nodes = 0);

  // The copy constructors, copy operator, and destructor are all default
  // implementations, but must be specified explicitly because
  // CompactSplineNode is not specified in the header file.
  CompactSpline(const CompactSpline& rhs);
  ~CompactSpline();
  CompactSpline& operator=(const CompactSpline& rhs);

  // The range of values for x and y must be specified at spline creation time
  // and cannot be changed afterwards. Empties all nodes, if we have any.
  //
  // 'y_range' -- The upper and lower bounds for y-values in the nodes.
  //              The more narrow this is, the better the precision of the
  //              fixed point numbers. Note that you should add 10% padding
  //              here, since AddNode may insert a smoothing node that is
  //              slightly beyond the source y range.
  // 'x_granularity' -- The minimum increment of x-values. If you're working
  //                    with a spline changes at most 30 times per second,
  //                    and your x is in units of 1/1000th of a second, then
  //                    x_granularity = 33 is a good baseline. You'll probably
  //                    want granularity around 1/50th of that baseline value,
  //                    though, since AddNode may insert smoothing nodes at
  //                    intermediate x's. In our example here, you could set
  //                    x_granularity near 33 / 50. For ease of debugging,
  //                    an x_granularity of 0.5 or 1 is probably best.
  // 'num_nodes' -- A hint for how many nodes are expected to be added.
  //                Try to get this close or slightly too high, to avoid
  //                reallocation of internal memory.
  void Init(const Range& y_range, const float x_granularity,
            const int num_nodes = 0);

  // Add a node to the end of the spline. Depending on the method, an
  // intermediate node may also be inserted.
  //
  // 'x' -- Must be greater than the x-value of the last spline node. If not,
  //        this call is a nop.
  // 'y' -- Must be within the 'y_range' specified in Init().
  // 'derivative' -- No restrictions, but excessively large values may still
  //                 result in overshoot, even with an intermediate node.
  void AddNode(const float x, const float y, const float derivative,
               const CompactSplineAddMethod method = kEnsureCubicWellBehaved);

  // Remove all nodes from the spline.
  void Clear();

  // Return index of the first node before 'x'.
  // If 'x' is before the first node, return kBeforeSplineIndex.
  // If 'x' is past the last node, return kAfterSplineIndex.
  //
  // 'guess_index' -- Best guess at what the index for 'x' will be. Often the
  //                  caller will be traversing from low to high x, so a good
  //                  guess is the index after the current index.
  //                  If you have no idea, set to 0.
  CompactSplineIndex IndexForX(const float x,
                               const CompactSplineIndex guess_index) const;

  // First and last x, y, and derivatives in the spline.
  float StartX() const;
  float StartY() const;
  float StartDerivative() const;
  float EndX() const;
  float EndY() const;
  float EndDerivative() const;
  Range RangeX() const { return Range(StartX(), EndX()); }
  const Range& RangeY() const { return y_range_; }

  // The start and end x-values covered by the segment after 'index'.
  Range RangeX(const CompactSplineIndex index) const;

  // Initialization parameters for a cubic curve that starts at 'index' and
  // ends at 'index' + 1. Or a constant curve if 'index' is kBeforeSplineIndex
  // or kAfterSplineIndex.
  CubicInit CreateCubicInit(const CompactSplineIndex index) const;

  static float RecommendXGranularity(const float max_x);

 private:
  CompactSplineIndex LastNodeIndex() const;

  // Return true iff 'x' is between the the nodes at 'index' and 'index' + 1.
  bool IndexContainsX(const CompactSplineXGrain compact_x,
                      const CompactSplineIndex index) const;

  // Search the nodes to find the index of the first node before 'x'.
  CompactSplineIndex BinarySearchIndexForX(
      const CompactSplineXGrain compact_x) const;

  // Return e.x - s.x, converted from quantized to external units.
  float WidthX(const CompactSplineNode& s, const CompactSplineNode& e) const;

  // Create the initialization parameters for a cubic running from 's' to 'e'.
  CubicInit CreateCubicInit(const CompactSplineNode& s,
                            const CompactSplineNode& e) const;

  // Array of key points (x, y, derivative) that describe the curve.
  // The curve is interpolated smoothly between these key points.
  // Key points are stored in quantized form, and converted back to world
  // co-ordinates by using 'y_range_' and 'x_granularity_'.
  std::vector<CompactSplineNode> nodes_;

  // Extreme values for y. See comments on Init() for details.
  Range y_range_;

  // Minimum increment for x. See comments on Init() for details.
  float x_granularity_;
};

}  // namespace fpl

#endif  // FPL_COMPACT_SPLINE_H_
