/*
 * Copyright 2026 Robomous
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Acceptance oracles for issue #402 — junction floor sidewalk band
// segmentation. The 2026-07 field triage measured this defect with a throwaway
// probe harness and set hard numeric criteria from it; per the triage record
// ("the item-1 probe oracles become tests, not GW steps") the measurement lives
// here instead, so the thresholds are enforced on every build.
//
// The four criteria, verbatim from the issue:
//   1. mid-band sidewalk-material misclassification < 1 % per band;
//   2. seam deviation < 0.5 * kSteinerStep from the ideal inner boundary;
//   3. zero-band collapse: 0 cases;
//   4. flush arm faces (no carriageway wedge at a sidewalked mouth).
//
// Everything is measured against `junction_sidewalk_bands()` — the same query
// the mesher constrains its triangulation to, so the oracle is the intent and
// the mesh is the realization.

#include "roadmaker/edit/operations.hpp"
#include "roadmaker/edit/markings.hpp"
#include "roadmaker/mesh/fill_params.hpp"
#include "roadmaker/mesh/junction_sidewalk_bands.hpp"
#include "roadmaker/mesh/mesh.hpp"
#include "roadmaker/mesh/mesh_builder.hpp"
#include "mesh/junction_surface.hpp"
#include "roadmaker/road/authoring.hpp"
#include "roadmaker/road/defaults.hpp"
#include "roadmaker/road/junction.hpp"
#include "roadmaker/road/network.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <numbers>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using roadmaker::ContactPoint;
using roadmaker::JunctionFloor;
using roadmaker::JunctionId;
using roadmaker::JunctionSidewalkBand;
using roadmaker::LaneProfile;
using roadmaker::LaneSpec;
using roadmaker::LaneType;
using roadmaker::NetworkMesh;
using roadmaker::RoadEnd;
using roadmaker::RoadId;
using roadmaker::RoadNetwork;
using roadmaker::SubMesh;
using roadmaker::Waypoint;

namespace {

// --- Fixtures ---------------------------------------------------------------
//
// Deliberately local rather than shared with test_junction_surface.cpp: the
// builders there are pinned to one geometry each (and five_way is hardwired to
// a profile with no sidewalk at all), because they double as the writers for
// the committed .xodr fixtures. The probe matrix needs the arm gap, the arm
// count and the profile to vary, which is a different job.

RoadId author(RoadNetwork& network,
              std::vector<Waypoint> waypoints,
              const char* odr_id,
              const LaneProfile& profile) {
  auto road = roadmaker::author_clothoid_road(network, waypoints, profile, "", odr_id);
  if (!road.has_value()) {
    throw std::runtime_error("author: " + road.error().message);
  }
  return *road;
}

JunctionId make_junction(RoadNetwork& network, const std::vector<RoadEnd>& ends) {
  auto command = roadmaker::edit::create_junction(network, ends);
  if (command == nullptr) {
    throw std::runtime_error("create_junction: null command");
  }
  auto applied = command->apply(network);
  if (!applied.has_value()) {
    throw std::runtime_error("create_junction: " + applied.error().message);
  }
  JunctionId junction;
  network.for_each_junction([&](JunctionId id, const roadmaker::Junction&) { junction = id; });
  return junction;
}

RoadEnd end_of(RoadId road) {
  return RoadEnd{.road = road, .contact = ContactPoint::End};
}

/// `arms` arms radiating from the origin at `angles`, each ending `gap` meters
/// out. `profiles[i]` is arm i's cross section, so a junction can mix
/// sidewalked and plain arms.
JunctionId build_radial(RoadNetwork& network,
                        const std::vector<double>& angles,
                        double gap,
                        const std::vector<LaneProfile>& profiles) {
  std::vector<RoadEnd> ends;
  for (std::size_t i = 0; i < angles.size(); ++i) {
    const double a = angles[i];
    const RoadId arm = author(network,
                              {Waypoint{40.0 * std::cos(a), 40.0 * std::sin(a)},
                               Waypoint{gap * std::cos(a), gap * std::sin(a)}},
                              std::to_string(i + 1).c_str(),
                              profiles[i % profiles.size()]);
    ends.push_back(end_of(arm));
  }
  return make_junction(network, ends);
}

std::vector<double> even_angles(int count) {
  std::vector<double> angles;
  angles.reserve(static_cast<std::size_t>(count));
  for (int i = 0; i < count; ++i) {
    angles.push_back((2.0 * std::numbers::pi * static_cast<double>(i)) /
                     static_cast<double>(count));
  }
  return angles;
}

/// A cross section whose sidewalk is NOT the outermost lane: one driving lane,
/// then the sidewalk, then a shoulder outboard of it. Nothing bundled puts a
/// shoulder outside a sidewalk, and that layout used to make the whole junction
/// invisible to the band splitter.
LaneProfile sidewalk_inside_shoulder() {
  LaneProfile profile;
  const LaneSpec driving{.type = LaneType::Driving, .width = roadmaker::defaults::kLocalLaneWidth};
  const LaneSpec walk{.type = LaneType::Sidewalk, .width = roadmaker::defaults::kSidewalkWidth};
  const LaneSpec shoulder{.type = LaneType::Shoulder, .width = roadmaker::defaults::kShoulderWidth};
  profile.left = {driving, walk, shoulder};
  profile.right = {driving, walk, shoulder};
  return profile;
}

// --- Geometry helpers -------------------------------------------------------

using Point = std::array<double, 2>;

double dist(const Point& a, const Point& b) {
  return std::hypot(a[0] - b[0], a[1] - b[1]);
}

Point lerp(const Point& a, const Point& b, double t) {
  return {a[0] + (t * (b[0] - a[0])), a[1] + (t * (b[1] - a[1]))};
}

Point mid_of(const JunctionSidewalkBand& band, std::size_t k) {
  return lerp(band.outer[k], band.inner[k], 0.5);
}

bool in_triangle(const Point& p, const Point& a, const Point& b, const Point& c) {
  const auto side = [](const Point& u, const Point& v, const Point& w) {
    return ((v[0] - u[0]) * (w[1] - u[1])) - ((v[1] - u[1]) * (w[0] - u[0]));
  };
  const double d1 = side(a, b, p);
  const double d2 = side(b, c, p);
  const double d3 = side(c, a, p);
  const bool neg = (d1 < 0.0) || (d2 < 0.0) || (d3 < 0.0);
  const bool pos = (d1 > 0.0) || (d2 > 0.0) || (d3 > 0.0);
  return !(neg && pos);
}

bool covers(const SubMesh& mesh, const Point& p) {
  for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
    const auto at = [&mesh](std::uint32_t v) {
      return Point{mesh.positions[v * 3], mesh.positions[(v * 3) + 1]};
    };
    if (in_triangle(p, at(mesh.indices[t]), at(mesh.indices[t + 1]), at(mesh.indices[t + 2]))) {
      return true;
    }
  }
  return false;
}

std::vector<const SubMesh*> sidewalk_bands_of(const JunctionFloor& floor) {
  std::vector<const SubMesh*> bands;
  for (const SubMesh& detail : floor.details) {
    if (detail.material == LaneType::Sidewalk && !detail.indices.empty()) {
      bands.push_back(&detail);
    }
  }
  return bands;
}

enum class Cover { Outside, Carriageway, Sidewalk };

/// What material the finished junction floor actually shows at (x, y).
/// `Outside` means no floor triangle covers the point at all — the ideal band
/// reached past this junction's floor, which the mesher clips away and the
/// oracle therefore skips.
Cover cover_at(const JunctionFloor& floor, const Point& p) {
  for (const SubMesh* band : sidewalk_bands_of(floor)) {
    if (covers(*band, p)) {
      return Cover::Sidewalk;
    }
  }
  return covers(floor.mesh, p) ? Cover::Carriageway : Cover::Outside;
}

/// Distance from `p` to the polyline `line`.
double distance_to_polyline(const Point& p, const std::vector<Point>& line) {
  double best = std::numeric_limits<double>::max();
  for (std::size_t i = 0; i + 1 < line.size(); ++i) {
    const Point& a = line[i];
    const Point& b = line[i + 1];
    const double ex = b[0] - a[0];
    const double ey = b[1] - a[1];
    const double len2 = (ex * ex) + (ey * ey);
    double t = 0.0;
    if (len2 > 1e-18) {
      t = std::clamp((((p[0] - a[0]) * ex) + ((p[1] - a[1]) * ey)) / len2, 0.0, 1.0);
    }
    best = std::min(best, dist(p, lerp(a, b, t)));
  }
  return best;
}

// --- The probe ---------------------------------------------------------------

struct BandMeasurement {
  std::size_t samples = 0;       ///< mid-band samples that landed on the floor
  std::size_t misclassified = 0; ///< ...and did not show sidewalk material
  double miss_ratio = 0.0;
};

/// Walks one ideal band's MID-LINE and asks the finished mesh what material it
/// shows there. Samples that fall outside the floor are not counted either way.
BandMeasurement measure_band(const JunctionFloor& floor, const JunctionSidewalkBand& band) {
  constexpr double kSampleStep = 0.2;
  BandMeasurement out;
  const std::size_t last = band.outer.size() - 1;
  for (std::size_t k = 0; k + 1 < band.outer.size(); ++k) {
    const Point from = mid_of(band, k);
    const Point to = mid_of(band, k + 1);
    const int steps = std::max(static_cast<int>(std::ceil(dist(from, to) / kSampleStep)), 1);
    for (int s = 0; s <= steps; ++s) {
      // The mid-line's two extreme ends sit exactly ON an arm face, which is
      // the floor's own boundary — no triangle owns a boundary point, so the
      // material there is not a defined quantity. Every other sample counts,
      // including the one 0.2 m in, so a band that fails to reach its mouth is
      // still caught (and criterion 4 probes the mouths directly).
      if ((k == 0 && s == 0) || (k + 1 == last && s == steps)) {
        continue;
      }
      const Point p = lerp(from, to, static_cast<double>(s) / steps);
      const Cover cover = cover_at(floor, p);
      if (cover == Cover::Outside) {
        continue;
      }
      ++out.samples;
      if (cover != Cover::Sidewalk) {
        ++out.misclassified;
      }
    }
  }
  out.miss_ratio = out.samples > 0
                       ? static_cast<double>(out.misclassified) / static_cast<double>(out.samples)
                       : 0.0;
  return out;
}

/// Largest distance from a realized SEAM vertex to the ideal inner boundary.
/// A seam vertex is one the carriageway and a band share bit-for-bit —
/// emit_floor_region copies the floor's exact positions into both regions, so
/// a shared position IS a point on the realized material boundary.
double seam_deviation(const JunctionFloor& floor, const std::vector<JunctionSidewalkBand>& ideal) {
  std::vector<Point> core;
  for (std::size_t v = 0; v + 2 < floor.mesh.positions.size(); v += 3) {
    core.push_back({floor.mesh.positions[v], floor.mesh.positions[v + 1]});
  }
  double worst = 0.0;
  for (const SubMesh* band : sidewalk_bands_of(floor)) {
    for (std::size_t v = 0; v + 2 < band->positions.size(); v += 3) {
      const Point p{band->positions[v], band->positions[v + 1]};
      const bool shared = std::any_of(
          core.begin(), core.end(), [&p](const Point& q) { return q[0] == p[0] && q[1] == p[1]; });
      if (!shared) {
        continue;
      }
      double best = std::numeric_limits<double>::max();
      for (const JunctionSidewalkBand& b : ideal) {
        // The seam against the carriageway is normally the band's INNER
        // boundary, the criterion's subject. A cross section that parks a
        // shoulder outboard of its sidewalk has a second one: the band's OUTER
        // boundary then divides sidewalk from shoulder rather than sidewalk
        // from open air, and it is a realized material boundary exactly like
        // the inner one. Measure each shared vertex against whichever ideal
        // boundary it belongs to.
        best = std::min({best, distance_to_polyline(p, b.inner), distance_to_polyline(p, b.outer)});
      }
      worst = std::max(worst, best);
    }
  }
  return worst;
}

/// One row of the probe matrix.
struct Case {
  std::string name;
  std::vector<double> angles;
  double gap;
  std::vector<LaneProfile> profiles;
};

std::vector<Case> matrix() {
  const LaneProfile walked = LaneProfile::local_road(); // sidewalk both sides
  const LaneProfile plain = LaneProfile::two_lane_rural();
  const LaneProfile shouldered = sidewalk_inside_shoulder();
  const double pi = std::numbers::pi;
  return {
      {"tee_tight", {pi, 0.0, -pi / 2.0}, 6.0, {walked}},
      {"four_way_tight", even_angles(4), 6.0, {walked}},
      {"four_way_roomy", even_angles(4), 20.0, {walked}},
      {"five_way_tight", even_angles(5), 7.0, {walked}},
      {"y_three_way", {pi / 2.0, (7.0 * pi) / 6.0, (11.0 * pi) / 6.0}, 10.0, {walked}},
      {"mixed_sidewalk_and_plain", even_angles(4), 10.0, {walked, walked, walked, plain}},
      {"single_sided", even_angles(4), 10.0, {walked, plain, plain, plain}},
      {"sidewalk_inside_shoulder", even_angles(4), 10.0, {shouldered}},
  };
}

/// The junction floor of a built case, plus the bands it should have.
struct Built {
  RoadNetwork network;
  JunctionId junction;
  NetworkMesh mesh;
};

Built build(const Case& c) {
  Built built;
  built.junction = build_radial(built.network, c.angles, c.gap, c.profiles);
  built.mesh = roadmaker::build_network_mesh(built.network);
  return built;
}

class SidewalkBandMatrix : public testing::TestWithParam<Case> {};

INSTANTIATE_TEST_SUITE_P(Probe,
                         SidewalkBandMatrix,
                         testing::ValuesIn(matrix()),
                         // `info` would shadow a parameter inside the macro's
                         // own expansion — GCC errors on it, clang does not.
                         [](const testing::TestParamInfo<Case>& probe) {
                           return probe.param.name;
                         });

// Criterion 3: every corner with at least one sidewalked adjacent arm emits a
// band. The tight five-way and the sidewalk-inside-shoulder profile used to
// emit NONE junction-wide — a fully sidewalked junction that rendered no
// sidewalk at all.
TEST_P(SidewalkBandMatrix, EverySidewalkedCornerEmitsABand) {
  const Built built = build(GetParam());
  ASSERT_FALSE(built.mesh.junction_floors.empty());
  const std::vector<JunctionSidewalkBand> ideal =
      roadmaker::junction_sidewalk_bands(built.network, built.junction);
  ASSERT_FALSE(ideal.empty()) << "no corner reported a sidewalk band";

  const std::vector<const SubMesh*> realized = sidewalk_bands_of(built.mesh.junction_floors[0]);
  EXPECT_EQ(realized.size(), ideal.size()) << "expected one band per sidewalked corner, got "
                                           << realized.size() << " of " << ideal.size();
  for (const SubMesh* band : realized) {
    EXPECT_EQ(band->material, LaneType::Sidewalk);
    EXPECT_EQ(band->indices.size() % 3, 0U);
    EXPECT_EQ(band->positions.size(), band->normals.size());
  }
}

// Criterion 1: the middle of a band shows sidewalk material. Centroid
// classification against an unconstrained triangulation measured 4 % on the
// roomiest case in the matrix and 27-33 % on the tight ones.
TEST_P(SidewalkBandMatrix, MidBandMisclassificationStaysBelowOnePercent) {
  const Built built = build(GetParam());
  ASSERT_FALSE(built.mesh.junction_floors.empty());
  const JunctionFloor& floor = built.mesh.junction_floors[0];
  const std::vector<JunctionSidewalkBand> ideal =
      roadmaker::junction_sidewalk_bands(built.network, built.junction);
  ASSERT_FALSE(ideal.empty());

  for (std::size_t i = 0; i < ideal.size(); ++i) {
    const BandMeasurement m = measure_band(floor, ideal[i]);
    if (m.samples == 0) {
      continue; // the band lies entirely outside this floor
    }
    EXPECT_LT(m.miss_ratio, 0.01) << "band " << i << ": " << m.misclassified << " of " << m.samples
                                  << " mid-band samples show carriageway material ("
                                  << (100.0 * m.miss_ratio) << " %)";
  }
}

// Criterion 2: the realized seam sits on the ideal one. Constrained edges pin
// it there up to corner-curve tessellation; an unconstrained classification is
// bounded below by the triangle size, which is kSteinerStep.
TEST_P(SidewalkBandMatrix, SeamFollowsTheIdealInnerBoundary) {
  const Built built = build(GetParam());
  ASSERT_FALSE(built.mesh.junction_floors.empty());
  const std::vector<JunctionSidewalkBand> ideal =
      roadmaker::junction_sidewalk_bands(built.network, built.junction);
  ASSERT_FALSE(ideal.empty());

  const double deviation = seam_deviation(built.mesh.junction_floors[0], ideal);
  EXPECT_LT(deviation, 0.5 * roadmaker::fill_params::kSteinerStep)
      << "worst seam vertex sits " << deviation << " m off the ideal inner boundary";
}

// Criterion 4: the band reaches the arm mouth, so the sidewalk pavement of the
// road and the band of the junction meet flush — no carriageway-material wedge
// between them.
TEST_P(SidewalkBandMatrix, BandsAreFlushWithTheArmFaces) {
  const Built built = build(GetParam());
  ASSERT_FALSE(built.mesh.junction_floors.empty());
  const JunctionFloor& floor = built.mesh.junction_floors[0];
  const std::vector<JunctionSidewalkBand> ideal =
      roadmaker::junction_sidewalk_bands(built.network, built.junction);
  ASSERT_FALSE(ideal.empty());

  constexpr double kJustInside = 0.3;
  for (std::size_t i = 0; i < ideal.size(); ++i) {
    const JunctionSidewalkBand& band = ideal[i];
    const std::size_t last = band.outer.size() - 1;
    const std::array<std::pair<Point, Point>, 2> mouths{
        std::pair{mid_of(band, 0), mid_of(band, 1)},
        std::pair{mid_of(band, last), mid_of(band, last - 1)}};
    for (const auto& [face, inward] : mouths) {
      const double run = dist(face, inward);
      if (run < kJustInside) {
        continue; // too short a leg to sample without leaving the band
      }
      const Point probe = lerp(face, inward, kJustInside / run);
      const Cover cover = cover_at(floor, probe);
      if (cover == Cover::Outside) {
        continue;
      }
      EXPECT_EQ(cover, Cover::Sidewalk)
          << "band " << i << " leaves a carriageway wedge at its mouth (" << probe[0] << ", "
          << probe[1] << ")";
    }
  }
}

// PrawkoRoadStudio S2: raised mode lifts the *existing constrained floor
// regions*, and builds a real vertical curb only at the shared edge with
// carriageway. Legacy mode remains flat and emits no curb walls.
TEST_P(SidewalkBandMatrix, RaisedCornerHasElevatedTopAndTrueCurbWall) {
  const Built baseline = build(GetParam());
  roadmaker::MeshOptions options;
  options.raised_sidewalks = true;
  const NetworkMesh raised = roadmaker::build_network_mesh(baseline.network, options);
  ASSERT_FALSE(raised.junction_floors.empty());
  const JunctionFloor& flat_floor = baseline.mesh.junction_floors.front();
  const JunctionFloor& raised_floor = raised.junction_floors.front();

  // The carriageway must not be moved when only sidewalks are raised.
  EXPECT_EQ(raised_floor.mesh.positions, flat_floor.mesh.positions);
  EXPECT_EQ(raised_floor.mesh.indices, flat_floor.mesh.indices);

  std::size_t sidewalks = 0;
  std::size_t curbs = 0;
  for (const SubMesh& detail : raised_floor.details) {
    if (detail.material == LaneType::Sidewalk) {
      ++sidewalks;
      ASSERT_FALSE(detail.indices.empty());
      for (std::size_t v = 2; v < detail.positions.size(); v += 3) {
        EXPECT_NEAR(detail.positions[v], roadmaker::defaults::kCurbHeight, 1e-6);
      }
    } else if (detail.material == LaneType::Curb) {
      ++curbs;
      ASSERT_FALSE(detail.indices.empty());
      const bool bevel = detail.name.find("bevel") != std::string::npos;
      EXPECT_EQ(detail.indices.size() % (bevel ? 3U : 6U), 0U);
      ASSERT_EQ(detail.positions.size(), detail.normals.size());
      ASSERT_EQ(detail.uvs.size() * 3, detail.positions.size() * 2);
      if (bevel) {
        using Point3 = std::array<double, 3>;
        std::map<Point3, Point3> normals_at_point;
        std::size_t matching_seams = 0;
        for (std::size_t v = 0; v < detail.positions.size() / 3; ++v) {
          const Point3 p{detail.positions[3 * v], detail.positions[3 * v + 1],
                         detail.positions[3 * v + 2]};
          const Point3 n{detail.normals[3 * v], detail.normals[3 * v + 1],
                        detail.normals[3 * v + 2]};
          EXPECT_NEAR(std::hypot(n[0], n[1], n[2]), 1.0, 1e-8);
          auto [it, inserted] = normals_at_point.emplace(p, n);
          if (!inserted) {
            ++matching_seams;
            for (int axis = 0; axis < 3; ++axis) {
              EXPECT_NEAR(n[axis], it->second[axis], 1e-8)
                  << "faceted normal seam inside a continuous curb chamfer";
            }
          }
        }
        if (detail.indices.size() >= 6) EXPECT_GT(matching_seams, 0U);
      }
      double min_height = 1e9;
      double max_height = -1e9;
      for (std::size_t v = 0; v + 2 < detail.positions.size(); v += 3) {
        min_height = std::min(min_height, detail.positions[v + 2]);
        max_height = std::max(max_height, detail.positions[v + 2]);
        if (bevel) {
          EXPECT_GT(detail.normals[v + 2], 0.0); // chamfer faces up and out
        } else {
          EXPECT_NEAR(detail.normals[v + 2], 0.0, 1e-8); // vertical face
        }
      }
      if (!bevel) {
        EXPECT_NEAR(min_height, 0.0, 1e-6);
        EXPECT_LE(max_height, roadmaker::defaults::kCurbHeight + 1e-6);
      }
      EXPECT_EQ(detail.surface, "concrete");
    }
  }
  EXPECT_GT(sidewalks, 0U);
  EXPECT_GT(curbs, 0U);
  // No extra 3-D wall or raised top in the old OpenDRIVE-compatible mode.
  for (const SubMesh& detail : flat_floor.details) {
    EXPECT_NE(detail.material, LaneType::Curb);
    if (detail.material == LaneType::Sidewalk) {
      for (std::size_t v = 2; v < detail.positions.size(); v += 3) {
        EXPECT_NEAR(detail.positions[v], 0.0, 1e-6);
      }
    }
  }

  // If there is an actual raised junction band, don't ramp the road-side
  // pavement back to ground level at its junction mouth.
  for (const auto& road : raised.roads) {
    for (const auto& patch : road.lanes) {
      if (patch.material != LaneType::Sidewalk) {
        continue;
      }
      for (const std::uint32_t index : patch.indices) {
        EXPECT_GE(road.positions[(3 * index) + 2],
                  roadmaker::defaults::kCurbHeight - 1e-6);
      }
    }
  }

  // At every real sidewalk arm mouth, BOTH the outside edge and the
  // carriageway-side edge must coincide with the raised junction sidewalk.
  // Without the matching bevel taper only the outside edge joined: the
  // inner edge was displaced by the road's 5 cm chamfer.
  const auto road_top_at = [&](const Point& p) {
    for (const auto& road : raised.roads) {
      for (const auto& patch : road.lanes) {
        if (patch.material != LaneType::Sidewalk) {
          continue;
        }
        for (const std::uint32_t v : patch.indices) {
          const double x = road.positions[3 * v];
          const double y = road.positions[(3 * v) + 1];
          const double z = road.positions[(3 * v) + 2];
          if (std::hypot(x - p[0], y - p[1]) < 1e-4 &&
              std::abs(z - roadmaker::defaults::kCurbHeight) < 1e-6) {
            return true;
          }
        }
      }
    }
    return false;
  };
  std::size_t mouths_checked = 0;
  for (const JunctionSidewalkBand& band :
       roadmaker::junction_sidewalk_bands(baseline.network, baseline.junction)) {
    for (const std::size_t j : {std::size_t{0}, band.outer.size() - 1}) {
      if (road_top_at(band.outer[j])) {
        ++mouths_checked;
        EXPECT_TRUE(road_top_at(band.inner[j]))
            << "raised top misses the inner curb edge at an arm mouth";
      }
    }
  }
  EXPECT_GT(mouths_checked, 0U);
}

// S2.2c: the bevel has its own triangles, but must share the true 3-D
// vertices of the shortened vertical wall and inset sidewalk top. A painted
// strip on an unchanged top would overlap the original triangles (z-fighting).
TEST_P(SidewalkBandMatrix, CornerChamferHasNoCracksOverlapsOrInvertedTopTriangles) {
  const Built baseline = build(GetParam());
  roadmaker::MeshOptions options;
  options.raised_sidewalks = true;
  const auto raised = roadmaker::build_network_mesh(baseline.network, options);
  ASSERT_FALSE(raised.junction_floors.empty());
  ASSERT_FALSE(baseline.mesh.junction_floors.empty());
  const auto& flat = baseline.mesh.junction_floors.front();
  const auto& corner = raised.junction_floors.front();
  std::vector<const SubMesh*> flat_tops, raised_tops, bevels, walls;
  for (const auto& part : flat.details) {
    if (part.material == LaneType::Sidewalk) {
      flat_tops.push_back(&part);
    }
  }
  for (const auto& part : corner.details) {
    if (part.material == LaneType::Sidewalk) {
      raised_tops.push_back(&part);
    } else if (part.material == LaneType::Curb) {
      if (part.name.find("bevel") != std::string::npos) {
        bevels.push_back(&part);
      } else {
        walls.push_back(&part);
      }
    }
  }
  ASSERT_EQ(flat_tops.size(), raised_tops.size());
  ASSERT_FALSE(raised_tops.empty());
  ASSERT_FALSE(walls.empty());
  ASSERT_FALSE(bevels.empty());

  for (std::size_t i = 0; i < flat_tops.size(); ++i) {
    const SubMesh& a = *flat_tops[i];
    const SubMesh& b = *raised_tops[i];
    ASSERT_EQ(a.indices, b.indices);
    ASSERT_EQ(a.positions.size(), b.positions.size());
    for (std::size_t p = 0; p < a.positions.size(); p += 3) {
      const double shift = std::hypot(b.positions[p] - a.positions[p],
                                      b.positions[p + 1] - a.positions[p + 1]);
      EXPECT_LE(shift, 0.05001); // maximum 5 cm physical inset
    }
    const auto area = [&](const SubMesh& mesh, std::size_t t) {
      const auto a0 = static_cast<std::size_t>(mesh.indices[t]);
      const auto b0 = static_cast<std::size_t>(mesh.indices[t + 1]);
      const auto c0 = static_cast<std::size_t>(mesh.indices[t + 2]);
      const auto dx = [&](std::size_t from, std::size_t to, int axis) {
        return mesh.positions[3 * to + axis] - mesh.positions[3 * from + axis];
      };
      return (dx(a0, b0, 0) * dx(a0, c0, 1)) -
             (dx(a0, b0, 1) * dx(a0, c0, 0));
    };
    for (std::size_t t = 0; t < a.indices.size(); t += 3) {
      const double old_area = area(a, t);
      const double new_area = area(b, t);
      if (std::abs(old_area) > 1e-11) {
        EXPECT_GT(old_area * new_area, 0.0) << "top triangle flipped";
        EXPECT_GE(std::abs(new_area), 0.19 * std::abs(old_area))
            << "top triangle compressed below safe fraction";
      }
    }
  }

  const auto matches_any = [](const std::array<double, 3>& p,
                              const std::vector<const SubMesh*>& meshes) {
    for (const SubMesh* mesh : meshes) {
      for (std::size_t v = 0; v + 2 < mesh->positions.size(); v += 3) {
        if (std::hypot(p[0] - mesh->positions[v],
                       p[1] - mesh->positions[v + 1],
                       p[2] - mesh->positions[v + 2]) < 1e-6) {
          return true;
        }
      }
    }
    return false;
  };
  std::size_t upper = 0, lower = 0;
  for (const SubMesh* bevel : bevels) {
    ASSERT_EQ(bevel->positions.size(), bevel->normals.size());
    ASSERT_EQ(bevel->positions.size() * 2, bevel->uvs.size() * 3);
    for (std::size_t t = 0; t < bevel->indices.size(); t += 3) {
      const auto v0 = static_cast<std::size_t>(bevel->indices[t]);
      const auto v1 = static_cast<std::size_t>(bevel->indices[t + 1]);
      const auto v2 = static_cast<std::size_t>(bevel->indices[t + 2]);
      const auto p = [&](std::size_t v, int axis) {
        return bevel->positions[3 * v + axis];
      };
      const double ax = p(v1, 0) - p(v0, 0);
      const double ay = p(v1, 1) - p(v0, 1);
      const double az = p(v1, 2) - p(v0, 2);
      const double bx = p(v2, 0) - p(v0, 0);
      const double by = p(v2, 1) - p(v0, 1);
      const double bz = p(v2, 2) - p(v0, 2);
      const auto nx = (ay * bz) - (az * by);
      const auto ny = (az * bx) - (ax * bz);
      const auto nz = (ax * by) - (ay * bx);
      EXPECT_GT(std::hypot(nx, ny, nz), 1e-9);
      EXPECT_GT(nz, 0.0);
    }
    for (std::size_t v = 0; v + 2 < bevel->positions.size(); v += 3) {
      const std::array<double, 3> point{
          bevel->positions[v], bevel->positions[v + 1], bevel->positions[v + 2]};
      if (bevel->uvs[(2 * (v / 3)) + 1] > 1e-8) {
        ++upper;
        EXPECT_TRUE(matches_any(point, raised_tops))
            << "chamfer upper edge does not meet the inset sidewalk";
      } else {
        ++lower;
        EXPECT_TRUE(matches_any(point, walls))
            << "chamfer lower edge does not meet its vertical wall";
      }
    }
  }
  EXPECT_GT(upper, 0U);
  EXPECT_GT(lower, 0U);
}

// S2.2b: a zebra placed 1 m before a junction used to be ignored because
// the road-only ramp could not climb back to 15 cm before the arm face. The
// junction band must now take over the ascent without an open vertical seam.
TEST(JunctionSidewalkBands, NearJunctionCrosswalksJoinCornerRampsAndRemeshTogether) {
  RoadNetwork network;
  const auto walk = LaneProfile::local_road();
  const JunctionId junction = build_radial(
      network, even_angles(4), 20.0, {walk});
  roadmaker::MeshOptions options;
  options.raised_sidewalks = true;
  NetworkMesh incremental = roadmaker::build_network_mesh(network, options);
  ASSERT_EQ(incremental.junction_floors.size(), 1U);

  roadmaker::edit::CrosswalkParams params;
  params.setback_m = 1.0;
  params.depth_m = 3.0;
  const auto crossings = roadmaker::edit::junction_crosswalks(network, junction, params);
  ASSERT_EQ(crossings.size(), 4U);
  std::vector<RoadId> changed;
  for (const auto& [road, obj] : crossings) {
    auto edit = roadmaker::edit::add_object(network, road, obj);
    ASSERT_NE(edit, nullptr);
    ASSERT_TRUE(edit->apply(network).has_value());
    changed.push_back(road);
  }
  roadmaker::remesh_objects(network, incremental, changed, options);
  const NetworkMesh fresh = roadmaker::build_network_mesh(network, options);
  ASSERT_EQ(fresh.junction_floors.size(), 1U);
  ASSERT_EQ(incremental.junction_floors.size(), 1U);
  ASSERT_EQ(incremental.roads.size(), fresh.roads.size());
  for (std::size_t i = 0; i < fresh.roads.size(); ++i) {
    EXPECT_EQ(incremental.roads[i].positions, fresh.roads[i].positions);
  }
  const auto& corner = fresh.junction_floors.front();
  const auto& refreshed = incremental.junction_floors.front();
  ASSERT_EQ(corner.details.size(), refreshed.details.size());
  for (std::size_t i = 0; i < corner.details.size(); ++i) {
    EXPECT_EQ(corner.details[i].positions, refreshed.details[i].positions);
    EXPECT_EQ(corner.details[i].indices, refreshed.details[i].indices);
  }

  std::size_t top_count = 0;
  double min_top = 1.0, max_top = -1.0;
  for (const auto& detail : corner.details) {
    if (detail.material != LaneType::Sidewalk) {
      continue;
    }
    ++top_count;
    for (std::size_t i = 2; i < detail.positions.size(); i += 3) {
      min_top = std::min(min_top, detail.positions[i]);
      max_top = std::max(max_top, detail.positions[i]);
    }
  }
  EXPECT_GT(top_count, 0U);
  EXPECT_LE(min_top, 0.015); // junction arm face is now at crossing level
  EXPECT_GE(max_top, 0.10);  // rises towards the middle of the corner
  std::size_t bevel_vertices = 0;
  for (const auto& detail : corner.details) {
    if (detail.name.find("bevel") == std::string::npos) {
      continue;
    }
    for (std::size_t v = 2; v < detail.normals.size(); v += 3) {
      ++bevel_vertices;
      EXPECT_GT(detail.normals[v], 0.0);
    }
  }
  EXPECT_GT(bevel_vertices, 0U)
      << "crosswalk ramp removed the corner chamfer away from the flush arm face";

  // Exact matching XY vertices at a street/junction seam must have the same
  // Z on both independently emitted sidewalk surfaces (no vertical cracks).
  std::size_t matched = 0;
  for (const auto& road : fresh.roads) {
    for (const auto& patch : road.lanes) {
      if (patch.material != LaneType::Sidewalk) {
        continue;
      }
      for (const auto rv : patch.indices) {
        const double rx = road.positions[3 * rv];
        const double ry = road.positions[3 * rv + 1];
        const double rz = road.positions[3 * rv + 2];
        for (const auto& detail : corner.details) {
          if (detail.material != LaneType::Sidewalk) {
            continue;
          }
          for (std::size_t v = 0; v + 2 < detail.positions.size(); v += 3) {
            if (std::hypot(rx - detail.positions[v], ry - detail.positions[v + 1]) < 1e-5) {
              ++matched;
              EXPECT_NEAR(rz, detail.positions[v + 2], 1e-5);
              EXPECT_LT(rz, 0.015);
            }
          }
        }
      }
    }
  }
  EXPECT_GT(matched, 0U);

  // Deleting the crossings must update the corner as well as the road, with
  // no residual depression in incremental 3-D geometry.
  std::vector<roadmaker::ObjectId> to_erase;
  network.for_each_object([&](roadmaker::ObjectId id, const roadmaker::Object& obj) {
    if (obj.type == roadmaker::ObjectType::Crosswalk) {
      to_erase.push_back(id);
    }
  });
  for (const auto id : to_erase) {
    ASSERT_TRUE(network.erase_object(id));
  }
  roadmaker::remesh_objects(network, incremental, changed, options);
  const NetworkMesh restored = roadmaker::build_network_mesh(network, options);
  ASSERT_EQ(incremental.junction_floors.size(), restored.junction_floors.size());
  ASSERT_EQ(incremental.junction_floors.front().details.size(),
            restored.junction_floors.front().details.size());
  for (std::size_t i = 0; i < restored.junction_floors.front().details.size(); ++i) {
    EXPECT_EQ(incremental.junction_floors.front().details[i].positions,
              restored.junction_floors.front().details[i].positions);
  }
}


// S2.2f: the unchanged legacy junction CDT is kept, but the rendered
// raised-sidewalk seam gains safe curvature samples on both adjacent faces.
// Compare both modes for X/T/Y, including the non-perpendicular Y.
TEST(JunctionSidewalkBands, VisualCurbArcLODAddsSharedVerticesWithoutSlivers) {
  const std::vector<std::vector<double>> configurations{
      {0.0, std::numbers::pi / 2, std::numbers::pi, 3 * std::numbers::pi / 2},
      {0.0, std::numbers::pi / 2, 3 * std::numbers::pi / 2},
      {0.0, 2 * std::numbers::pi / 3, 4 * std::numbers::pi / 3}};
  for (const auto& angles : configurations) {
    RoadNetwork network;
    const JunctionId id = build_radial(network, angles, 20.0, {LaneProfile::local_road()});
    const auto* junction = network.junction(id);
    ASSERT_NE(junction, nullptr);
    const auto original = roadmaker::build_junction_floor_split(network, *junction, {}, false);
    const auto stable_raised = roadmaker::build_junction_floor_split(network, *junction, {}, true);
    const auto refined = roadmaker::build_junction_floor_split(network, *junction, {}, true, true);
    const auto again = roadmaker::build_junction_floor_split(network, *junction, {}, true, true);
    ASSERT_EQ(original.sidewalk_bands.size(), stable_raised.sidewalk_bands.size());
    for (std::size_t k=0;k<original.sidewalk_bands.size();++k) {
      EXPECT_EQ(original.sidewalk_bands[k].indices, stable_raised.sidewalk_bands[k].indices);
    }
    ASSERT_EQ(refined.sidewalk_bands.size(), original.sidewalk_bands.size());
    ASSERT_EQ(refined.sidewalk_bands.size(), again.sidewalk_bands.size());
    ASSERT_EQ(refined.carriageway.indices, again.carriageway.indices);
    ASSERT_EQ(refined.carriageway.positions, again.carriageway.positions);
    std::size_t old_tris = original.carriageway.indices.size() / 3;
    std::size_t new_tris = refined.carriageway.indices.size() / 3;
    for (std::size_t k = 0; k < original.sidewalk_bands.size(); ++k) {
      old_tris += original.sidewalk_bands[k].indices.size() / 3;
      new_tris += refined.sidewalk_bands[k].indices.size() / 3;
      EXPECT_EQ(refined.sidewalk_bands[k].positions, again.sidewalk_bands[k].positions);
      EXPECT_EQ(refined.sidewalk_bands[k].indices, again.sidewalk_bands[k].indices);
    }
    // Independent 5-degree sliver check on the actual rendered triangulation,
    // not the untouched CDT mesh. It protects dense, acute T/Y junctions.
    const auto slivers = [](const SubMesh& mesh) -> std::size_t {
      std::size_t bad = 0;
      const auto at = [&](std::uint32_t v) {
        return std::array<double,2>{mesh.positions[3*v],mesh.positions[3*v+1]};
      };
      const auto angle = [](const auto& a, const auto& b, const auto& c) {
        const double ux=b[0]-a[0], uy=b[1]-a[1];
        const double vx=c[0]-a[0], vy=c[1]-a[1];
        return std::atan2(std::abs(ux*vy-uy*vx), ux*vx+uy*vy);
      };
      for (std::size_t i=0;i+2<mesh.indices.size();i+=3) {
        const auto a=at(mesh.indices[i]),b=at(mesh.indices[i+1]),
                   c=at(mesh.indices[i+2]);
        if (std::min({angle(a,b,c),angle(b,c,a),angle(c,a,b)}) <
            0.08726646259971647) ++bad;
      }
      return bad;
    };
    std::size_t old_slivers=slivers(original.carriageway);
    std::size_t new_slivers=slivers(refined.carriageway);
    for (std::size_t k=0;k<original.sidewalk_bands.size();++k) {
      old_slivers+=slivers(original.sidewalk_bands[k]);
      new_slivers+=slivers(refined.sidewalk_bands[k]);
    }
    std::printf("[ArcLOD] arms=%zu old_tris=%zu new_tris=%zu delta=%zu slivers=%zu/%zu\n",
                angles.size(), old_tris, new_tris, new_tris - old_tris,
                old_slivers, new_slivers);
    EXPECT_LE(new_slivers, old_slivers)
        << "Refining curved seams must not introduce any new sliver triangle";
    EXPECT_GT(new_tris, old_tris) << "The render mesh did not actually refine curved seams";
    // The wall's lower vertices must be EXACT road vertices — a visual
    // refinement may not introduce a T-seam or expose a gap under the curb.
    std::set<std::array<double,3>> driving_vertices;
    for (std::size_t v=0;v+2<refined.carriageway.positions.size();v+=3) {
      driving_vertices.insert({refined.carriageway.positions[v],
                               refined.carriageway.positions[v+1],
                               refined.carriageway.positions[v+2]});
    }
    std::size_t curb_bottoms=0;
    for (const auto& wall : refined.curb_faces) {
      if (wall.name.find("corner curb") == std::string::npos) continue;
      for (std::size_t v=0;v+11<wall.positions.size();v+=12) {
        for (std::size_t j=0;j<2;++j) {
          const std::array<double,3> p{
              wall.positions[v+3*j],wall.positions[v+3*j+1],
              wall.positions[v+3*j+2]};
          ++curb_bottoms;
          EXPECT_TRUE(driving_vertices.contains(p))
              << "Wall does not meet the driving triangulation at its base";
        }
      }
    }
    EXPECT_GT(curb_bottoms, 0U);
    for (const auto* mesh : [&]() {
           std::vector<const SubMesh*> result{&refined.carriageway};
           for (const auto& side : refined.sidewalk_bands) result.push_back(&side);
           for (const auto& face : refined.curb_faces) result.push_back(&face);
           return result;
         }()) {
      for (std::size_t i = 0; i + 2 < mesh->indices.size(); i += 3) {
        const auto a = mesh->indices[i], b = mesh->indices[i+1], c=mesh->indices[i+2];
        const auto xyz = [&](std::uint32_t v) {
          return std::array<double,3>{mesh->positions[3*v], mesh->positions[3*v+1],
                                      mesh->positions[3*v+2]};
        };
        const auto p=xyz(a), q=xyz(b), r=xyz(c);
        const std::array<double,3> u{q[0]-p[0],q[1]-p[1],q[2]-p[2]};
        const std::array<double,3> v{r[0]-p[0],r[1]-p[1],r[2]-p[2]};
        const std::array<double,3> n{
            u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
        EXPECT_GT(std::hypot(n[0],n[1],n[2]), 1e-9) << mesh->name;
      }
    }
  }
}

} // namespace
