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
#include "bulk_spline_evaluator.h"
#include "common.h"

using fpl::QuadraticCurve;
using fpl::CubicCurve;
using fpl::CubicInit;
using fpl::Range;
using fpl::CompactSpline;
using fpl::CompactSplineIndex;
using fpl::BulkSplineEvaluator;
using mathfu::vec2;
using mathfu::vec2i;
using mathfu::vec3;

// Print the curves in a format that can be cut-and-paste into a spreadsheet.
// Working in a spreadsheet is nice because of the graphing features.
#define PRINT_SPLINES_AS_CSV 0

// Draw an ASCII graph of the curves. Helpful for a quick visualization, though
// not very high fidelity, obviously.
#define PRINT_SPLINES_AS_ASCII_GRAPHS 0

struct GraphData {
  std::vector<vec2> points;
  std::vector<vec3> derivatives;
};

static const int kNumCheckPoints = fpl::kDefaultGraphWidth;
static const vec2i kGraphSize(kNumCheckPoints, fpl::kDefaultGraphHeight);
static const float kFixedPointEpsilon = 0.01f;
static const float kDerivativePrecision = 0.01f;
static const float kSecondDerivativePrecision = 0.1f;
static const float kThirdDerivativePrecision = 1.0f;
static const float kXGranularityScale = 0.01f;

// Use a ridiculous index that will never hit when doing a search.
// We use this to test the binary search algorithm, not the cache.
static const CompactSplineIndex kRidiculousSplineIndex = 10000;

static const CubicInit kSimpleSplines[] = {
  //        start_y   start_derivative  end_y   end_derivative    width_x
  CubicInit( 1.0f,    -8.0f,            0.0f,   0.0f,             1.0f),
  CubicInit( 1.0f,    -8.0f,           -1.0f,   0.0f,             1.0f),
};
static const int kNumSimpleSplines =
    static_cast<int>(ARRAYSIZE(kSimpleSplines));

static const CubicInit CubicInitMirrorY(const CubicInit& init) {
  return CubicInit(-init.start_y, -init.start_derivative,
                   -init.end_y, -init.end_derivative, init.width_x);
}

static const CubicInit CubicInitScaleX(const CubicInit& init, float scale) {
  return CubicInit(init.start_y, init.start_derivative / scale,
                   init.end_y, init.end_derivative / scale,
                   init.width_x * scale);
}

static const CubicInit CubicInitFlipStartAndEnd(const CubicInit& init) {
  return CubicInit(init.end_y, init.end_derivative,
                   init.start_y, init.start_derivative, init.width_x);
}

static Range CubicInitYRange(const CubicInit& init, float buffer_percent) {
  return fpl::CreateValidRange(init.start_y,
                               init.end_y).Lengthen(buffer_percent);
}


static void InitializeSpline(const CubicInit& init, CompactSpline* spline) {
  const Range y_range = CubicInitYRange(init, 0.1f);
  spline->Init(y_range, init.width_x * kXGranularityScale, 3);
  spline->AddNode(0.0f, init.start_y, init.start_derivative);
  spline->AddNode(init.width_x, init.end_y, init.end_derivative);
}

static void ExecuteInterpolator(BulkSplineEvaluator& interpolator,
                                int num_points, GraphData* d) {
  const float y_precision = interpolator.SourceSpline(0)->RangeY().Length() *
                            kFixedPointEpsilon;

  const Range range_x = interpolator.SourceSpline(0)->RangeX();
  const float delta_x = range_x.Length() / (num_points - 1);

  for (int i = 0; i < num_points; ++i) {
    const CubicCurve& c = interpolator.Cubic(0);
    const float x = interpolator.CubicX(0);

    EXPECT_NEAR(c.Evaluate(x), interpolator.Y(0), y_precision);
    EXPECT_NEAR(c.Derivative(x), interpolator.Derivative(0),
                kDerivativePrecision);

    const vec2 point(interpolator.X(0), interpolator.Y(0));
    const vec3 derivatives(interpolator.Derivative(0), c.SecondDerivative(x),
                           c.ThirdDerivative(x));
    d->points.push_back(point);
    d->derivatives.push_back(derivatives);

    interpolator.AdvanceFrame(delta_x);
  }
}

static void PrintGraphDataAsCsv(const GraphData& d) {
  (void)d;
#if PRINT_SPLINES_AS_CSV
  for (size_t i = 0; i < d.points.size(); ++i) {
    printf("%f, %f, %f, %f, %f\n",
           d.points[i].x(), d.points[i].y(), d.derivatives[i].x(),
           d.derivatives[i].y(), d.derivatives[i].z());
  }
#endif // PRINT_SPLINES_AS_CSV
}

static void PrintSplineAsAsciiGraph(const GraphData& d) {
  (void)d;
#if PRINT_SPLINES_AS_ASCII_GRAPHS
  printf("\n%s\n\n",
         fpl::Graph2DPoints(&d.points[0], d.points.size(), kGraphSize).c_str());
#endif // PRINT_SPLINES_AS_ASCII_GRAPHS
}

static void GatherGraphData(const CubicInit& init, GraphData* d) {
  CompactSpline spline;
  InitializeSpline(init, &spline);

  BulkSplineEvaluator interpolator;
  interpolator.SetNumIndices(1);
  interpolator.SetSpline(0, spline);

  ExecuteInterpolator(interpolator, kNumCheckPoints, d);

  PrintGraphDataAsCsv(*d);
  PrintSplineAsAsciiGraph(*d);
}

class SplineTests : public ::testing::Test {
protected:
  virtual void SetUp() {
    short_spline_.Init(Range(0.0f, 1.0f), 0.01f, 4);
    short_spline_.AddNode(0.0f, 0.1f, 0.0f, fpl::kAddWithoutModification);
    short_spline_.AddNode(1.0f, 0.4f, 0.0f, fpl::kAddWithoutModification);
    short_spline_.AddNode(4.0f, 0.2f, 0.0f, fpl::kAddWithoutModification);
    short_spline_.AddNode(40.0f, 0.2f, 0.0f, fpl::kAddWithoutModification);
    short_spline_.AddNode(100.0f, 1.0f, 0.0f, fpl::kAddWithoutModification);
  }
  virtual void TearDown() {}

  CompactSpline short_spline_;
};

// Ensure the index lookup is accurate for x's before the range.
TEST_F(SplineTests, IndexForXBefore) {
  EXPECT_EQ(fpl::kBeforeSplineIndex,
            short_spline_.IndexForX(-1.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(SplineTests, IndexForXJustBefore) {
  EXPECT_EQ(0, short_spline_.IndexForX(-0.0001f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely before the range.
TEST_F(SplineTests, IndexForXBiggerThanGranularityAtStart) {
  EXPECT_EQ(0, short_spline_.IndexForX(-0.011f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's after the range.
TEST_F(SplineTests, IndexForXAfter) {
  EXPECT_EQ(fpl::kAfterSplineIndex,
            short_spline_.IndexForX(101.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x's barely after the range.
TEST_F(SplineTests, IndexForXJustAfter) {
  EXPECT_EQ(fpl::kAfterSplineIndex,
            short_spline_.IndexForX(100.0001f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x right at start.
TEST_F(SplineTests, IndexForXStart) {
  EXPECT_EQ(0, short_spline_.IndexForX(0.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x right at end.
TEST_F(SplineTests, IndexForXEnd) {
  EXPECT_EQ(fpl::kAfterSplineIndex,
            short_spline_.IndexForX(100.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(SplineTests, IndexForXAlmostEnd) {
  EXPECT_EQ(fpl::kAfterSplineIndex,
            short_spline_.IndexForX(99.9999f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x just inside end.
TEST_F(SplineTests, IndexForXBiggerThanGranularityAtEnd) {
  EXPECT_EQ(3, short_spline_.IndexForX(99.99f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, right on the node.
TEST_F(SplineTests, IndexForXMidOnNode) {
  EXPECT_EQ(1, short_spline_.IndexForX(1.0f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(SplineTests, IndexForXMidAfterNode) {
  EXPECT_EQ(1, short_spline_.IndexForX(1.1f, kRidiculousSplineIndex));
}

// Ensure the index lookup is accurate for x in middle, in middle of segment.
TEST_F(SplineTests, IndexForXMidSecondLast) {
  EXPECT_EQ(2, short_spline_.IndexForX(4.1f, kRidiculousSplineIndex));
}

// Ensure the splines don't overshoot their mark.
TEST_F(SplineTests, Overshoot) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];

    GraphData d;
    GatherGraphData(init, &d);

    const Range x_range(-kXGranularityScale,
                        init.width_x * (1.0f + kXGranularityScale));
    const Range y_range = CubicInitYRange(init, 0.001f);
    for (size_t i = 0; i < d.points.size(); ++i) {
      EXPECT_TRUE(x_range.Contains(d.points[i].x()));
      EXPECT_TRUE(y_range.Contains(d.points[i].y()));
    }
  }
}

// Ensure that the curves are mirrored in y when node y's are mirrored.
TEST_F(SplineTests, MirrorY) {
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit mirrored_init = CubicInitMirrorY(init);
    const float y_precision = fabs(init.start_y - init.end_y) *
                              kFixedPointEpsilon;

    GraphData d, mirrored_d;
    GatherGraphData(init, &d);
    GatherGraphData(mirrored_init, &mirrored_d);

    EXPECT_EQ(d.points.size(), mirrored_d.points.size());
    const int num_points = static_cast<int>(d.points.size());
    for (int i = 0; i < num_points; ++i) {
      EXPECT_EQ(d.points[i].x(), mirrored_d.points[i].x());
      EXPECT_NEAR(d.points[i].y(), -mirrored_d.points[i].y(), y_precision);
      EXPECT_NEAR(d.derivatives[i].x(), -mirrored_d.derivatives[i].x(),
                  kDerivativePrecision);
      EXPECT_NEAR(d.derivatives[i].y(), -mirrored_d.derivatives[i].y(),
                  kSecondDerivativePrecision);
      EXPECT_NEAR(d.derivatives[i].z(), -mirrored_d.derivatives[i].z(),
                  kThirdDerivativePrecision);
    }
  }
}

// Ensure that the curves are scaled in x when node's x is scaled.
TEST_F(SplineTests, ScaleX) {
  static const float kScale = 100.0f;
  for (int i = 0; i < kNumSimpleSplines; ++i) {
    const CubicInit& init = kSimpleSplines[i];
    const CubicInit scaled_init = CubicInitScaleX(init, kScale);
    const float x_precision = init.width_x * kFixedPointEpsilon;
    const float y_precision = fabs(init.start_y - init.end_y) *
                              kFixedPointEpsilon;

    GraphData d, scaled_d;
    GatherGraphData(init, &d);
    GatherGraphData(scaled_init, &scaled_d);

    EXPECT_EQ(d.points.size(), scaled_d.points.size());
    const int num_points = static_cast<int>(d.points.size());
    for (int i = 0; i < num_points; ++i) {
      EXPECT_NEAR(d.points[i].x(), scaled_d.points[i].x() / kScale,
                  x_precision);
      EXPECT_NEAR(d.points[i].y(), scaled_d.points[i].y(), y_precision);
      EXPECT_NEAR(d.derivatives[i].x(), scaled_d.derivatives[i].x() *
                  kScale, kDerivativePrecision);
      EXPECT_NEAR(d.derivatives[i].y(), scaled_d.derivatives[i].y() *
                  kScale * kScale, kSecondDerivativePrecision);
      EXPECT_NEAR(d.derivatives[i].z(), scaled_d.derivatives[i].z() *
                  kScale * kScale * kScale, kThirdDerivativePrecision);
    }
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

