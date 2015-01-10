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

#include <string>
#include <sstream>
#include <vector>
#include "bulk_spline_evaluator.h"
#include "dual_cubic.h"

using mathfu::Lerp;

namespace fpl {


// A spline is composed of a series of spline nodes (x, y, derivative) that are
// interpolated to form a smooth curve.
//
// This class represents a single spline node in 6-bytes. It quantizes the
// valid ranges of x, y, and slope into three 16-bit integers = 6 bytes.
//
// The x and y values are quantized to the valid range. The valid range is
// stored externally and passed in to each call. Please see comments on
// CompactSplineXGrain and CompactSplineYRung for more detail.
//
// The derivative is stored as the angle from the x-axis. This is so that we
// can equally represent derivatives <= 1 (that is, <= 45 degrees) and
// derivatives >= 1 (that is, >= 45 degrees) with a quantized number.
//
class CompactSplineNode {
 public:
  CompactSplineNode() : x_(0), y_(0), angle_(0) {}

  // Construct with values that have already been converted to quantized values.
  // This constructor is useful when deserializing pre-converted data.
  CompactSplineNode(const CompactSplineXGrain x, const CompactSplineYRung y,
                    const CompactSplineAngle angle)
      : x_(x), y_(y), angle_(angle) {
  }

  // Construct with real-world values. Must pass in the valid x and y ranges.
  CompactSplineNode(const float x, const float y, const float derivative,
                    const float x_granularity, const Range& y_range) {
    SetX(x, x_granularity);
    SetY(y, y_range);
    SetDerivative(derivative);
  }

  // Set with real-world values. The valid range of x and y must be passed in.
  // These values are passed in so that we don't have to store multiple copies
  // of them. Memory compactness is the purpose of this class.
  void SetX(const float x, const float x_granularity) {
    x_ = CompactX(x, x_granularity);
  }
  void SetY(const float y, const Range& y_range) { y_ = CompactY(y, y_range); }
  void SetDerivative(const float derivative) {
    angle_ = CompactDerivative(derivative);
  }

  // Get real world values. The valid range of x and y must be passed in.
  // The valid range must be the same as when x and y values were set.
  float X(const float x_granularity) const {
    return static_cast<float>(x_) * x_granularity;
  }
  float Y(const Range& y_range) const { return y_range.Lerp(YPercent()); }
  float Derivative() const { return tan(Angle()); }

  // Get the quantized values. Useful for serializing a series of nodes.
  CompactSplineXGrain x() const { return x_; }
  CompactSplineYRung y() const { return y_; }
  CompactSplineAngle angle() const { return angle_; }

  // Convert from real-world to quantized values.
  // Please see type definitions for documentation on the quantized format.
  static int QuantizeX(const float x, const float x_granularity) {
    return static_cast<int>(x / x_granularity + 0.5f);
  }

  static CompactSplineXGrain CompactX(const float x, const float x_granularity) {
    const int x_quantized = QuantizeX(x, x_granularity);
    assert(0 <= x_quantized && x_quantized <= kMaxX);
    return static_cast<CompactSplineXGrain>(x_quantized);
  }

  static CompactSplineYRung CompactY(const float y, const Range& y_range) {
    assert(y_range.Contains(y));
    const float y_percent = y_range.PercentClamped(y);
    const CompactSplineYRung compact_y =
        static_cast<CompactSplineYRung>(kMaxY * y_percent);
    return compact_y;
  }

  static CompactSplineAngle CompactDerivative(const float derivative) {
    const float angle_radians = atan(derivative);
    const CompactSplineAngle angle =
        static_cast<CompactSplineAngle>(angle_radians / kAngleScale);
    return angle;
  }

  static CompactSplineXGrain MaxX() { return kMaxX; }

 private:
  static const CompactSplineXGrain kMaxX =
      std::numeric_limits<CompactSplineXGrain>::max();
  static const CompactSplineYRung kMaxY =
      std::numeric_limits<CompactSplineXGrain>::max();
  static const CompactSplineAngle kMinAngle =
      std::numeric_limits<CompactSplineAngle>::min();
  static constexpr float kYScale = 1.0f / static_cast<float>(kMaxY);
  static constexpr float kAngleScale =
      static_cast<float>(-M_PI / static_cast<double>(kMinAngle));

  float YPercent() const { return static_cast<float>(y_) * kYScale; }
  float Angle() const { return static_cast<float>(angle_) * kAngleScale; }

  // Position along x-axis. Multiplied by x-granularity to get actual domain.
  // 0 ==> start. kMaxX ==> end, we should never reach the end. If we do,
  // the x_granularity should be increased.
  CompactSplineXGrain x_;

  // Position within y_range. 0 ==> y_range.start. kMaxY ==> y_range.end.
  CompactSplineYRung y_;

  // Angle from x-axis. tan(angle) = rise / run = derivative.
  CompactSplineAngle angle_;
};


CompactSpline::CompactSpline() : x_granularity_(0.0f) {}

CompactSpline::CompactSpline(const Range& y_range, const float x_granularity,
                             const int num_nodes) {
  Init(y_range, x_granularity, num_nodes);
}

// Default implementation. Explicitly written here because 'nodes_' needs the
// implementation of CompactSplineNode.
CompactSpline::CompactSpline(const CompactSpline& rhs)
    : nodes_(rhs.nodes_),
      y_range_(rhs.y_range_),
      x_granularity_(rhs.x_granularity_) {
}

// Default implementation. Explicitly written here because 'nodes_' needs the
// implementation of CompactSplineNode.
CompactSpline::~CompactSpline() {}

// Default implementation. Explicitly written here because 'nodes_' needs the
// implementation of CompactSplineNode.
CompactSpline& CompactSpline::operator=(const CompactSpline& rhs) {
  *this = CompactSpline(rhs);
  return *this;
}

void CompactSpline::Clear() { nodes_.clear(); }

void CompactSpline::Init(const Range& y_range, const float x_granularity,
                         const int num_nodes) {
  y_range_ = y_range;
  x_granularity_ = x_granularity;
  nodes_.reserve(num_nodes);
  nodes_.resize(0);
}

void CompactSpline::AddNode(const float x, const float y,
                            const float derivative,
                            const CompactSplineAddMethod method) {
  const CompactSplineNode new_node(x, y, derivative, x_granularity_, y_range_);

  // Precondition: Nodes must come *after* the last node.
  // Due to rounding, it's possible that the we have the *same* x as the last
  // node. This is valid and we do not assert, but we do return immediately.
  assert(nodes_.size() == 0 || new_node.x() >= nodes_.back().x());
  const bool strictly_after_last_node = nodes_.size() == 0 ||
                                        new_node.x() > nodes_.back().x();
  if (!strictly_after_last_node)
    return;

  // Add a dual-cubic mid-node, if required, to keep cubic curves well behaved.
  if (method == kEnsureCubicWellBehaved && nodes_.size() != 0) {
    const CompactSplineNode& last_node = nodes_.back();
    const CubicInit init = CreateCubicInit(last_node, new_node);
    const CubicCurve curve(init);

    // A curve is well behaved if it has uniform curvature.
    if (!curve.UniformCurvature(Range(0.0f, WidthX(last_node, new_node)))) {
      // Find a suitable intermediate node using the math from the Dual Cubics
      // document.
      float mid_x;
      float mid_y;
      float mid_derivative;
      CalculateDualCubicMidNode(init, &mid_x, &mid_y, &mid_derivative);

      // Add the intermediate node, as long as it
      const CompactSplineNode mid_node(last_node.X(x_granularity_) + mid_x,
                                       mid_y, mid_derivative,
                                       x_granularity_, y_range_);
      const bool is_unique_x = mid_node.x() != last_node.x() &&
                               mid_node.x() != new_node.x();
      if (is_unique_x) {
        nodes_.push_back(mid_node);
      }
    }
  }

  // Add the new node.
  nodes_.push_back(new_node);
}

float CompactSpline::StartX() const { return nodes_.front().X(x_granularity_); }
float CompactSpline::StartY() const { return nodes_.front().Y(y_range_); }
float CompactSpline::StartDerivative() const {
  return nodes_.front().Derivative();
}

float CompactSpline::EndX() const { return nodes_.back().X(x_granularity_); }
float CompactSpline::EndY() const { return nodes_.back().Y(y_range_); }
float CompactSpline::EndDerivative() const {
  return nodes_.back().Derivative();
}

Range CompactSpline::RangeX(const CompactSplineIndex index) const {
  if (index == kBeforeSplineIndex)
    return Range(-std::numeric_limits<float>::infinity(), StartX());

  if (index == kAfterSplineIndex)
    return Range(EndX(), std::numeric_limits<float>::infinity());

  return Range(nodes_[index].X(x_granularity_),
               nodes_[index + 1].X(x_granularity_));
}

float CompactSpline::WidthX(const CompactSplineNode& s,
                            const CompactSplineNode& e) const {
  return (e.x() - s.x()) * x_granularity_;
}

CompactSplineIndex CompactSpline::IndexForX(
    const float x, const CompactSplineIndex guess_index) const {
  const int quantized_x = CompactSplineNode::QuantizeX(x, x_granularity_);

  // Check bounds first.
  // Return negative if before index 0.
  if (quantized_x < nodes_.front().x())
    return kBeforeSplineIndex;

  // Return index of the last index if beyond the last index.
  if (quantized_x >= nodes_.back().x())
    return kAfterSplineIndex;

  // Check the guess value first.
  const CompactSplineXGrain compact_x = static_cast<CompactSplineXGrain>(quantized_x);
  if (IndexContainsX(compact_x, guess_index))
    return guess_index;

  // Search for it, if the initial guess fails.
  const CompactSplineIndex index = BinarySearchIndexForX(compact_x);
  assert(IndexContainsX(compact_x, index));
  return index;
}

CompactSplineIndex CompactSpline::LastNodeIndex() const {
  return static_cast<CompactSplineIndex>(nodes_.size() - 1);
}

bool CompactSpline::IndexContainsX(const CompactSplineXGrain compact_x,
                                   const CompactSplineIndex index) const {
  return index < LastNodeIndex() &&
         nodes_[index].x() <= compact_x && compact_x <= nodes_[index + 1].x();
}

static inline bool CompareSplineNodeX(const CompactSplineXGrain compact_x,
                                      const CompactSplineNode& n) {
  return compact_x < n.x();
}

CompactSplineIndex CompactSpline::BinarySearchIndexForX(
    const CompactSplineXGrain compact_x) const {
  // Binary search nodes by x.
  // TODO OPT: avoid the pointer arithmetic (which is expensive on ARM since it
  // requires an integer division) by searching with indices instead of
  // iterators.
  //     int low = 0;
  //     int hi = max_hi;
  //     while (low + 1 < hi) {
  //       const int mid = (low + hi) / 2;
  //
  //       if (compact_x < nodes_[mid].x()) {
  //         hi = mid;
  //       } else {
  //         low = mid;
  //       }
  //     }
  const auto upper_it = std::upper_bound(nodes_.begin(), nodes_.end(),
                                         compact_x, CompareSplineNodeX);
  const int low = static_cast<int>(upper_it - nodes_.begin()) - 1;
  assert(0 <= low && low < LastNodeIndex());

  // We return the lower index: x is in the segment bt 'index' and 'index' + 1.
  return static_cast<CompactSplineIndex>(low);
}

CubicInit CompactSpline::CreateCubicInit(const CompactSplineIndex index) const {
  // Handle case where we are outside of the interpolatable range.
  if (OutsideSpline(index)) {
    const CompactSplineNode& n = index == kBeforeSplineIndex ? nodes_.front()
                                                             : nodes_.back();
    const float constant_y = n.Y(y_range_);
    return CubicInit(constant_y, 0.0f, constant_y, 0.0f, 1.0f);
  }

  // Interpolate between the nodes at 'index' and 'index' + 1.
  return CreateCubicInit(nodes_[index], nodes_[index + 1]);
}

CubicInit CompactSpline::CreateCubicInit(const CompactSplineNode& s,
                                         const CompactSplineNode& e) const {
  return CubicInit(s.Y(y_range_), s.Derivative(),
                   e.Y(y_range_), e.Derivative(), WidthX(s, e));
}

// static
float CompactSpline::RecommendXGranularity(const float max_x) {
  return max_x <= 0.0f ? 1.0f : max_x / CompactSplineNode::MaxX();
}


} // namespace fpl
