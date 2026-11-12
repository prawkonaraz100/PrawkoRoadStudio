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

#include "roadmaker/mesh/mesh_builder.hpp"

#include "roadmaker/assets/prop_library.hpp"
#include "roadmaker/assets/sign_catalog.hpp"
#include "roadmaker/edit/connection.hpp"
#include "roadmaker/geometry/poly3.hpp"
#include "roadmaker/mesh/junction_stoplines.hpp"
#include "roadmaker/mesh/junction_sidewalk_bands.hpp"
#include "roadmaker/road/defaults.hpp"
#include "roadmaker/road/repeat_expansion.hpp"
#include "roadmaker/tol.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "bridge_solids.hpp"
#include "fill_backend.hpp"
#include "junction_surface.hpp"
#include "mesh_detail.hpp"
#include "object_placement.hpp"
#include "surface_fill.hpp"
#include "terrain_mesh.hpp"

namespace roadmaker {

namespace {

using mesh_detail::boundary_offsets;
using mesh_detail::lateral_point;
using mesh_detail::make_frame;
using mesh_detail::mandatory_stations;
using mesh_detail::StationFrame;
using mesh_detail::surface_normal;

// Marking pattern for RoadMarkType::Broken — the registry's broken lane line
// (docs/domain/realism_defaults.md §1.3).
constexpr double kDashLength = defaults::kDashLength;
constexpr double kDashCycle = defaults::kDashLength + defaults::kDashGap;

// Lift markings slightly above the surface to avoid z-fighting.
constexpr double kMarkingLift = 0.002;

/// The outer boundary offset of a lane (its t at the edge away from the
/// reference line) — where its marking is painted.
double outer_boundary_offset(const RoadNetwork& network,
                             const Road& road,
                             const LaneSection& section,
                             const Lane& lane,
                             double s) {
  const std::vector<double> offsets = boundary_offsets(network, road, section, s);
  const std::size_t center_index = static_cast<std::size_t>(std::ranges::count_if(
      section.lanes, [&](LaneId id) { return network.lane(id)->odr_id > 0; }));
  if (lane.odr_id == 0) {
    return offsets[center_index];
  }
  // p-th lane out from the center on its side.
  std::size_t p = 0;
  for (const LaneId lane_id : section.lanes) {
    const Lane& other = *network.lane(lane_id);
    if ((lane.odr_id > 0) == (other.odr_id > 0) && other.odr_id != 0 &&
        std::abs(other.odr_id) <= std::abs(lane.odr_id)) {
      ++p;
    }
  }
  return lane.odr_id > 0 ? offsets[center_index - p] : offsets[center_index + p];
}

void emit_marking_strip(const Road& road,
                        double s_begin,
                        double s_end,
                        double t,
                        double width,
                        std::vector<double> section_stations,
                        SubMesh& out) {
  // Clamp the strip's sampling to [s_begin, s_end].
  std::erase_if(section_stations,
                [&](double s) { return s < s_begin - tol::kLength || s > s_end + tol::kLength; });
  if (section_stations.empty() || section_stations.front() > s_begin + tol::kLength) {
    section_stations.insert(section_stations.begin(), s_begin);
  }
  if (section_stations.back() < s_end - tol::kLength) {
    section_stations.push_back(s_end);
  }
  const double half = width / 2.0;
  const std::uint32_t base = static_cast<std::uint32_t>(out.positions.size() / 3);
  for (const double s : section_stations) {
    const StationFrame frame = make_frame(road, s);
    for (const double offset : {t + half, t - half}) {
      const auto p = lateral_point(frame, offset);
      out.positions.insert(out.positions.end(), {p[0], p[1], p[2] + kMarkingLift});
      const auto n = surface_normal(frame);
      out.normals.insert(out.normals.end(), n.begin(), n.end());
    }
  }
  for (std::uint32_t row = 0; row + 1 < section_stations.size(); ++row) {
    const std::uint32_t a = base + (row * 2); // left edge of strip
    const std::uint32_t b = a + 1;            // right edge
    const std::uint32_t c = a + 3;            // next right
    const std::uint32_t d = a + 2;            // next left
    out.indices.insert(out.indices.end(), {a, b, c, a, c, d});
  }
}

/// The painted stripes a road mark decomposes into, resolved for meshing
/// (docs/design/m3a/02 §1/§4). Explicit <line> geometry wins; otherwise the
/// multi-line types synthesize two symmetric stripes so solid_solid renders
/// as true dual geometry, and single types stay one stripe (M2 behaviour).
std::vector<RoadMarkLine> resolve_stripes(const RoadMark& mark) {
  if (!mark.lines.empty()) {
    return mark.lines;
  }
  const double w = mark.width;
  const RoadMarkLine solid{.width = w};
  const RoadMarkLine broken{.width = w, .length = kDashLength, .space = kDashCycle - kDashLength};
  // Symmetric split for the *_family: stripe centers sit so the clear space
  // between the two stripes is the registry's double-line separation (§1.3).
  const double half = (w + defaults::kDoubleLineSeparation) / 2.0;
  const auto at = [](RoadMarkLine stripe, double t) {
    stripe.t_offset = t;
    return stripe;
  };
  switch (mark.type) {
  case RoadMarkType::Broken:
    return {broken};
  case RoadMarkType::SolidSolid:
    return {at(solid, +half), at(solid, -half)};
  case RoadMarkType::SolidBroken:
    return {at(solid, +half), at(broken, -half)};
  case RoadMarkType::BrokenSolid:
    return {at(broken, +half), at(solid, -half)};
  case RoadMarkType::BrokenBroken:
    return {at(broken, +half), at(broken, -half)};
  case RoadMarkType::Solid:
  case RoadMarkType::Other:
  case RoadMarkType::None:
    break;
  }
  return {solid};
}

void build_markings(const RoadNetwork& network,
                    const Road& road,
                    const RoadId road_id,
                    const std::vector<double>& stations,
                    RoadMesh& mesh) {
  for (std::size_t si = 0; si < road.sections.size(); ++si) {
    const LaneSection& section = *network.lane_section(road.sections[si]);
    const double section_end = si + 1 < road.sections.size()
                                   ? network.lane_section(road.sections[si + 1])->s0
                                   : road.plan_view.length();

    std::vector<double> section_stations;
    for (const double s : stations) {
      if (s >= section.s0 - tol::kLength && s <= section_end + tol::kLength) {
        section_stations.push_back(s);
      }
    }

    for (const LaneId lane_id : section.lanes) {
      const Lane& lane = *network.lane(lane_id);
      for (std::size_t mi = 0; mi < lane.road_marks.size(); ++mi) {
        const RoadMark& mark = lane.road_marks[mi];
        if (mark.type == RoadMarkType::None || mark.width <= 0.0) {
          continue;
        }
        const double mark_begin = section.s0 + mark.s_offset;
        const double mark_end = mi + 1 < lane.road_marks.size()
                                    ? section.s0 + lane.road_marks[mi + 1].s_offset
                                    : section_end;
        if (mark_end <= mark_begin) {
          continue;
        }

        SubMesh strip;
        strip.material = lane.type;
        strip.mark_color = mark.color;
        strip.name = fmt::format("road {} lane {} marking", road.odr_id, lane.odr_id);

        // One quad run per painted stripe. Multi-line marks emit two strips at
        // their lateral offsets; dashed stripes (space > 0) tessellate as
        // segment runs of `length` on / (length + space) cycle.
        for (const RoadMarkLine& stripe : resolve_stripes(mark)) {
          const double begin = std::min(mark_begin + stripe.s_offset, mark_end);
          if (stripe.width <= 0.0) {
            continue;
          }
          if (stripe.space > tol::kLength && stripe.length > tol::kLength) {
            const double cycle = stripe.length + stripe.space;
            for (double dash = begin; dash < mark_end; dash += cycle) {
              const double dash_end = std::min(dash + stripe.length, mark_end);
              const double t =
                  outer_boundary_offset(network, road, section, lane, dash) + stripe.t_offset;
              emit_marking_strip(road, dash, dash_end, t, stripe.width, section_stations, strip);
            }
          } else {
            const double t =
                outer_boundary_offset(network, road, section, lane, begin) + stripe.t_offset;
            emit_marking_strip(road, begin, mark_end, t, stripe.width, section_stations, strip);
          }
        }
        if (!strip.indices.empty()) {
          mesh.markings.push_back(std::move(strip));
        }
      }
    }
    (void)road_id;
  }
}

// --- object markings (crosswalks, stop lines, lane arrows) — §13.8/§13.14 ---

/// A flat local frame at an object's origin for painted-marking geometry: the
/// world centre (lifted off the surface) plus planar u (forward = road tangent
/// rotated by the object heading) and v (leftward) axes.
struct ObjectFrame {
  std::array<double, 3> center;
  std::array<double, 3> normal;
  double ux, uy; // forward axis in world XY
  double vx, vy; // leftward axis in world XY
};

/// The paint frame at a road-relative pose. Split out of object_frame so a
/// DERIVED marking (one with no Object behind it, e.g. a junction stop line)
/// gets a frame from exactly the same math a placed object would.
ObjectFrame pose_frame(const Road& road, double s, double t, double hdg) {
  const StationFrame f = make_frame(road, s);
  std::array<double, 3> center = lateral_point(f, t);
  center[2] += kMarkingLift;
  const double heading = std::atan2(f.sin_h, f.cos_h) + hdg;
  return ObjectFrame{.center = center,
                     .normal = surface_normal(f),
                     .ux = std::cos(heading),
                     .uy = std::sin(heading),
                     .vx = -std::sin(heading),
                     .vy = std::cos(heading)};
}

ObjectFrame object_frame(const Road& road, const Object& object) {
  return pose_frame(road, object.s, object.t, object.hdg);
}

/// Appends a convex polygon (local u/v points, CCW) as a triangle fan.
void emit_object_polygon(const ObjectFrame& frame,
                         std::span<const std::array<double, 2>> points,
                         SubMesh& out) {
  if (points.size() < 3) {
    return;
  }
  const std::uint32_t base = static_cast<std::uint32_t>(out.positions.size() / 3);
  for (const std::array<double, 2>& p : points) {
    out.positions.insert(out.positions.end(),
                         {frame.center[0] + (p[0] * frame.ux) + (p[1] * frame.vx),
                          frame.center[1] + (p[0] * frame.uy) + (p[1] * frame.vy),
                          frame.center[2]});
    out.normals.insert(out.normals.end(), frame.normal.begin(), frame.normal.end());
  }
  for (std::uint32_t i = 1; i + 1 < static_cast<std::uint32_t>(points.size()); ++i) {
    out.indices.insert(out.indices.end(), {base, base + i, base + i + 1});
  }
}

/// A filled rectangle centred at the object, `length` along u, `width` along v.
void emit_object_quad(const ObjectFrame& frame, double length, double width, SubMesh& out) {
  const double hu = length / 2.0;
  const double hv = width / 2.0;
  const std::array<std::array<double, 2>, 4> quad{{{-hu, -hv}, {hu, -hv}, {hu, hv}, {-hu, hv}}};
  emit_object_polygon(frame, quad, out);
}

// Zebra stripe geometry (§13.14.3): paint band + gap repeated along u.
constexpr double kZebraStripe = 0.5;
constexpr double kZebraGap = 0.5;

/// Alternating painted bars across the crossing: `length` along u (crossing
/// direction), `width` along v. Deterministic bar layout from -length/2. Used
/// as the fallback for foreign crosswalks that carry no CrosswalkData.
void emit_zebra(const ObjectFrame& frame, double length, double width, SubMesh& out) {
  const double hv = width / 2.0;
  const double cycle = kZebraStripe + kZebraGap;
  for (double u = -length / 2.0; u < length / 2.0 - tol::kLength; u += cycle) {
    const double u_end = std::min(u + kZebraStripe, length / 2.0);
    const std::array<std::array<double, 2>, 4> bar{{{u, -hv}, {u_end, -hv}, {u_end, hv}, {u, hv}}};
    emit_object_polygon(frame, bar, out);
  }
}

/// A parametric crosswalk from its CrosswalkData (p3-s2). `length` is the
/// crossing span along u, `width` the walking depth along v. dash_length<=0
/// paints one solid quad; otherwise bars of `dash_length` repeat every
/// dash_length+dash_gap. With border_width>0, two solid border quads frame the
/// two road-parallel edges (the u-extremes). Matches the outline/markings the
/// generator authors (edit::author_crosswalk).
void emit_crosswalk(const ObjectFrame& frame,
                    double length,
                    double width,
                    const CrosswalkData& data,
                    SubMesh& out) {
  const double hv = width / 2.0;
  const double hu = length / 2.0;
  if (data.dash_length <= tol::kLength) {
    emit_object_quad(frame, length, width, out); // solid crossing
  } else {
    const double cycle = data.dash_length + data.dash_gap;
    for (double u = -hu; u < hu - tol::kLength; u += cycle) {
      const double u_end = std::min(u + data.dash_length, hu);
      const std::array<std::array<double, 2>, 4> bar{
          {{u, -hv}, {u_end, -hv}, {u_end, hv}, {u, hv}}};
      emit_object_polygon(frame, bar, out);
    }
  }
  if (data.border_width > tol::kLength) {
    const double bw = std::min(data.border_width, length); // clamp to the span
    for (const double u_center : {-hu + (bw / 2.0), hu - (bw / 2.0)}) {
      const std::array<std::array<double, 2>, 4> border{{{u_center - (bw / 2.0), -hv},
                                                         {u_center + (bw / 2.0), -hv},
                                                         {u_center + (bw / 2.0), hv},
                                                         {u_center - (bw / 2.0), hv}}};
      emit_object_polygon(frame, border, out);
    }
  }
}

/// Parametric arrow glyph (generated, no asset — GS-1 checklist row 5): a
/// shaft rectangle plus a triangular head, `length` along u (travel), sized to
/// `width`. The head tip shifts laterally for the left/right turn variants.
void emit_arrow(
    const ObjectFrame& frame, double length, double width, std::string_view subtype, SubMesh& out) {
  const double neck = length * 0.2;       // where the head starts (from centre)
  const double shaft_half = width * 0.15; // shaft half-width
  const double head_half = width * 0.5;   // head base half-width
  // Shaft: tail (-length/2) to neck.
  const std::array<std::array<double, 2>, 4> shaft{{{-length / 2.0, -shaft_half},
                                                    {neck, -shaft_half},
                                                    {neck, shaft_half},
                                                    {-length / 2.0, shaft_half}}};
  emit_object_polygon(frame, shaft, out);
  // Head: base at neck spanning ±head_half, tip forward (+u) with a lateral
  // bias for turn arrows.
  double tip_v = 0.0;
  if (subtype == "arrowLeft") {
    tip_v = head_half;
  } else if (subtype == "arrowRight") {
    tip_v = -head_half;
  }
  const std::array<std::array<double, 2>, 3> head{
      {{neck, -head_half}, {neck, head_half}, {length / 2.0, tip_v}}};
  emit_object_polygon(frame, head, out);
}

/// A free-form marking curve from its MarkingCurveData (p3-s4): a band of the
/// stored width painted along the (s,t) centreline, walked by arc length so a
/// dash pattern (dash_length>0) breaks into runs. Each sample maps through the
/// road reference line to a world point; the band edges are the centreline
/// offset ±width/2 along the XY-perpendicular of the local tangent. Solid when
/// dash_length<=0. Meshed from the userData, mirroring emit_crosswalk (the
/// outline/markings are only the foreign-tool projection).
void emit_marking_curve(const Road& road, const MarkingCurveData& data, SubMesh& out) {
  const std::vector<std::array<double, 2>>& samples = data.samples;
  if (samples.size() < 2) {
    return;
  }
  const double half = std::max(data.width, tol::kLength) / 2.0;

  struct Node {
    std::array<double, 3> p;    // world point (surface-lifted)
    std::array<double, 2> left; // unit XY perpendicular, pointing left
    std::array<double, 3> n;    // surface normal
  };

  std::vector<Node> nodes(samples.size());
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const StationFrame f = make_frame(road, samples[i][0]);
    std::array<double, 3> p = lateral_point(f, samples[i][1]);
    p[2] += kMarkingLift;
    nodes[i].p = p;
    nodes[i].n = surface_normal(f);
  }
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    double dx = 0.0;
    double dy = 0.0;
    if (i > 0) {
      dx += nodes[i].p[0] - nodes[i - 1].p[0];
      dy += nodes[i].p[1] - nodes[i - 1].p[1];
    }
    if (i + 1 < nodes.size()) {
      dx += nodes[i + 1].p[0] - nodes[i].p[0];
      dy += nodes[i + 1].p[1] - nodes[i].p[1];
    }
    const double len = std::hypot(dx, dy);
    nodes[i].left = len > tol::kLength ? std::array<double, 2>{-dy / len, dx / len}
                                       : std::array<double, 2>{0.0, 0.0};
  }

  // Push a band quad between the interpolated cross-sections at local arc
  // lengths la..lb of segment [A,B]; left/normal are lerped and re-normalized.
  const auto emit_span = [&](const Node& a, const Node& b, double seglen, double la, double lb) {
    const auto cross = [&](double frac) {
      std::array<double, 3> centre{a.p[0] + ((b.p[0] - a.p[0]) * frac),
                                   a.p[1] + ((b.p[1] - a.p[1]) * frac),
                                   a.p[2] + ((b.p[2] - a.p[2]) * frac)};
      double lx = a.left[0] + ((b.left[0] - a.left[0]) * frac);
      double ly = a.left[1] + ((b.left[1] - a.left[1]) * frac);
      const double llen = std::hypot(lx, ly);
      if (llen > tol::kLength) {
        lx /= llen;
        ly /= llen;
      }
      const std::array<double, 3> l{centre[0] - (half * lx), centre[1] - (half * ly), centre[2]};
      const std::array<double, 3> r{centre[0] + (half * lx), centre[1] + (half * ly), centre[2]};
      return std::pair{l, r};
    };
    const auto [la_l, la_r] = cross(la / seglen);
    const auto [lb_l, lb_r] = cross(lb / seglen);
    const std::array<double, 3> n{
        (a.n[0] + b.n[0]) / 2.0, (a.n[1] + b.n[1]) / 2.0, (a.n[2] + b.n[2]) / 2.0};
    const std::uint32_t base = static_cast<std::uint32_t>(out.positions.size() / 3);
    for (const std::array<double, 3>& v : {la_r, lb_r, lb_l, la_l}) {
      out.positions.insert(out.positions.end(), v.begin(), v.end());
      out.normals.insert(out.normals.end(), n.begin(), n.end());
    }
    out.indices.insert(out.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
  };

  const bool solid = data.dash_length <= tol::kLength;
  const double cycle = data.dash_length + data.dash_gap;
  double dist = 0.0;
  for (std::size_t i = 0; i + 1 < nodes.size(); ++i) {
    const Node& a = nodes[i];
    const Node& b = nodes[i + 1];
    const double seglen = std::hypot(b.p[0] - a.p[0], b.p[1] - a.p[1]);
    if (seglen <= tol::kLength) {
      continue;
    }
    if (solid) {
      emit_span(a, b, seglen, 0.0, seglen);
    } else if (cycle > tol::kLength) {
      const long kstart = static_cast<long>(std::floor(dist / cycle));
      for (long k = kstart; static_cast<double>(k) * cycle < dist + seglen; ++k) {
        const double pa = std::max(dist, static_cast<double>(k) * cycle);
        const double pb =
            std::min(dist + seglen, (static_cast<double>(k) * cycle) + data.dash_length);
        if (pb > pa) {
          emit_span(a, b, seglen, pa - dist, pb - dist);
        }
      }
    }
    dist += seglen;
  }
}

/// Tessellates a closed cornerLocal outline (u,v) with the in-tree CDT and
/// emits the triangles in the object's frame — the concave-glyph path for
/// authored arrow stencils (p3-s4). No-op when CDT refuses the polygon.
void emit_tessellated_outline(const ObjectFrame& frame,
                              const ObjectOutline& outline,
                              SubMesh& out) {
  std::vector<std::array<double, 2>> polygon;
  polygon.reserve(outline.corners.size());
  for (const OutlineCorner& corner : outline.corners) {
    polygon.push_back({corner.a, corner.b}); // (u, v) local
  }
  const std::optional<fill_backend::CompactMesh> mesh = fill_backend::tessellate_polygon(polygon);
  if (!mesh.has_value()) {
    return;
  }
  const std::uint32_t base = static_cast<std::uint32_t>(out.positions.size() / 3);
  for (const fill_backend::Vec3& v : mesh->vertices) {
    out.positions.insert(out.positions.end(),
                         {frame.center[0] + (v.x * frame.ux) + (v.y * frame.vx),
                          frame.center[1] + (v.x * frame.uy) + (v.y * frame.vy),
                          frame.center[2]});
    out.normals.insert(out.normals.end(), frame.normal.begin(), frame.normal.end());
  }
  for (const std::array<std::uint32_t, 3>& tri : mesh->triangles) {
    out.indices.insert(out.indices.end(), {base + tri[0], base + tri[1], base + tri[2]});
  }
}

/// True when `object` carries an authored stencil outline (one closed cornerLocal
/// loop) the mesher should tessellate instead of drawing the parametric glyph.
bool has_local_glyph_outline(const Object& object) {
  return !object.outlines.empty() && !object.outlines.front().road_coords &&
         object.outlines.front().closed.value_or(false) &&
         object.outlines.front().corners.size() >= 3;
}

/// Object-based road markings a road owns (crosswalks, stop lines, arrows —
/// docs/design/m3a/02 §3). Emitted as paint submeshes; unresolved object
/// types are left to the render-side placeholder box (phase 4).
void build_object_markings(const RoadNetwork& network,
                           const Road& road,
                           RoadId road_id,
                           RoadMesh& mesh) {
  for (const ObjectId object_id : objects_of(network, road_id)) {
    const Object& object = *network.object(object_id);
    const ObjectFrame frame = object_frame(road, object);
    SubMesh marking;
    marking.material = LaneType::None;

    if (object.marking_curve.has_value()) {
      // Free-form curve: render exactly from MarkingCurveData (dispatched before
      // the crosswalk branch — a striped curve is typed as a crosswalk).
      marking.name = fmt::format("road {} object {} marking curve", road.odr_id, object.odr_id);
      emit_marking_curve(road, *object.marking_curve, marking);
      marking.surface = object.marking_curve->material;
    } else if (object.type == ObjectType::Crosswalk) {
      const double length = object.length.value_or(4.0);
      const double width = object.width.value_or(2.0);
      marking.name = fmt::format("road {} object {} crosswalk", road.odr_id, object.odr_id);
      if (object.crosswalk.has_value()) {
        // Parametric asset instance: render exactly from CrosswalkData and
        // carry its material code so the viewport can tint the paint.
        emit_crosswalk(frame, length, width, *object.crosswalk, marking);
        marking.surface = object.crosswalk->material;
      } else {
        emit_zebra(frame, length, width, marking); // foreign crosswalk fallback
      }
    } else if (object.type_str == "roadMark" && object.subtype == "signalLines") {
      const double length = object.length.value_or(0.3);
      const double width = object.width.value_or(3.5);
      marking.name = fmt::format("road {} object {} stop line", road.odr_id, object.odr_id);
      emit_object_quad(frame, length, width, marking);
    } else if (object.type_str == "roadMark" && object.subtype.starts_with("arrow")) {
      marking.name = fmt::format("road {} object {} arrow", road.odr_id, object.odr_id);
      if (has_local_glyph_outline(object)) {
        // Authored stencil: tessellate the object's own concave glyph outline.
        emit_tessellated_outline(frame, object.outlines.front(), marking);
      } else {
        const double length = object.length.value_or(4.0);
        const double width = object.width.value_or(1.75);
        emit_arrow(frame, length, width, object.subtype, marking); // legacy/merge fallback
      }
      if (object.stencil.has_value()) {
        marking.surface = object.stencil->material; // tint from the asset material
      }
    } else {
      continue; // not a painted marking — render-side placeholder (phase 4)
    }
    if (!marking.indices.empty()) {
      mesh.markings.push_back(std::move(marking));
    }
  }
}

/// Junction stop lines painted on THIS road (p4-s3, #318). Unlike every other
/// marking these have no Object behind them: they are derived from the junction
/// at each end of the road (plus any authored StopLine record) and materialized
/// into the .xodr only on write. junction_stoplines() is the single geometry
/// source, so what the viewport paints and what the writer exports cannot drift.
///
/// The quad is emitted through the same emit_object_quad the placed-object
/// branch uses, with a synthetic frame at (s_center, t_center, hdg = 0) and
/// length/width taken from the solve — pixel-identical to the objects the
/// pre-p4-s3 generator used to create. An arm already carrying a legacy or
/// foreign signalLines object is suppressed inside junction_stoplines(), so
/// that object's own branch above draws it exactly once.
void build_stopline_markings(const RoadNetwork& network,
                             const Road& road,
                             RoadId road_id,
                             RoadMesh& mesh) {
  const auto paint = [&](const JunctionStopLineInfo& info) {
    const ObjectFrame frame = pose_frame(road, info.s_center, info.t_center, 0.0);
    SubMesh marking;
    marking.material = LaneType::None;
    marking.name = fmt::format("road {} stopline {}",
                               road.odr_id,
                               info.arm.contact == ContactPoint::End ? "end" : "start");
    emit_object_quad(frame, info.thickness, info.span, marking);
    if (!marking.indices.empty()) {
      mesh.markings.push_back(std::move(marking));
    }
  };

  for (const ContactPoint contact : {ContactPoint::Start, ContactPoint::End}) {
    const RoadEnd arm{.road = road_id, .contact = contact};
    const std::optional<JunctionId> junction = edit::junction_at_end(network, arm);
    if (!junction.has_value()) {
      continue;
    }
    for (const JunctionStopLineInfo& info : junction_stoplines(network, *junction)) {
      if (info.arm == arm) {
        paint(info);
      }
    }
  }

  // Span (virtual) junction faces (p4-s4, issue #319): the road is never cut,
  // so junction_at_end finds nothing and the owning junction has to be looked
  // up through its span list instead. A road can carry several span junctions
  // (two crosswalks), so every one of them is asked.
  network.for_each_junction([&](JunctionId junction_id, const Junction& junction) {
    const bool spans_this_road =
        std::any_of(junction.spans.begin(), junction.spans.end(), [&](const SpanArm& span) {
          return span.road == road_id;
        });
    if (!spans_this_road) {
      return;
    }
    for (const JunctionStopLineInfo& info : junction_stoplines(network, junction_id)) {
      if (info.arm.road == road_id) {
        paint(info);
      }
    }
  });
}

/// Placed props (trees/vegetation) a road owns, as INSTANCES of bundled prop
/// models. An object contributes an instance only when it is a Tree/Vegetation
/// whose @name resolves to a prop_library model; markings and foreign objects
/// are skipped here (they round-trip in xodr and, for markings, mesh as paint
/// in build_object_markings). The instance transform is the object's world
/// pose from its road-relative s/t (no marking z-lift — a prop sits on the
/// surface, offset only by its z_offset).
void build_object_instances(const RoadNetwork& network,
                            const Road& road,
                            RoadId road_id,
                            std::vector<ObjectInstance>& out) {
  for (const ObjectId object_id : objects_of(network, road_id)) {
    const Object& object = *network.object(object_id);
    // Instanced prop classes: trees, vegetation, poles (streetlights), and
    // buildings all resolve their @name to a bundled prop model. Crosswalks are
    // excluded — they mesh as paint in build_object_markings, not as props.
    if (object.type != ObjectType::Tree && object.type != ObjectType::Vegetation &&
        object.type != ObjectType::Pole && object.type != ObjectType::Building) {
      continue;
    }
    const props::PropModel* prop_model = props::model(object.name);
    if (prop_model == nullptr) {
      continue;
    }
    // One derivation per object: every instance it emits (single or repeated)
    // renders at the size the object declares.
    const double scale = props::instance_scale(object, prop_model);

    // Which instances exist and where they land is object_placement's single
    // answer, shared with the obstruction query (#464) so the flag and the
    // picture can never disagree. Repeat-series precedence, the chord of a
    // detached section, and the past-the-end skip all live there.
    for (const object_placement::PlacedInstance& placed :
         object_placement::placed_instances(road, object)) {
      out.push_back(ObjectInstance{.object = object_id,
                                   .road = road_id,
                                   .model_id = object.name,
                                   .position = placed.position,
                                   .heading = placed.heading,
                                   .scale = scale});
    }
  }
}

/// The catalogue entry a placed <signal> is drawn and labelled as, or nullptr.
///
/// The full designation is (@country, @type, @subtype) — §14.1 is explicit that
/// @type and @subtype only identify a signal together — and the shipped pack
/// depends on it: One Way (R6-1) ships a right- and a left-arrow variant that
/// differ ONLY in @subtype and draw different models. Resolving on
/// (@country, @type) alone therefore drew every left One Way with the right
/// arrow, which is what #429 fixes. signs::resolve_designation keeps the
/// @subtype-blind match as its fallback; see its header comment.
const signs::SignDef* signal_designation(const Signal& signal) {
  return signs::resolve_designation(signal.country, signal.type, signal.subtype);
}

/// The bundled signal model a <signal> renders as.
///
/// The shipped sign catalogue (roadmaker::signs, spec §1.4) IS the mapping: a
/// placed signal's designation names its model, so a designation gains a
/// silhouette by joining the table rather than by growing a branch here. Two
/// degradations are deliberate and must both stay:
///
///  - an identity this build does not ship (a foreign-country sign, or the
///    German StVO plates RoadMaker itself authored before #414) still has to
///    draw, so it falls back to the generic silhouette rather than vanishing;
///  - a catalogue entry whose model the asset bundle does not carry falls back
///    the same way, because the table can legitimately outrun the meshes.
///
/// @dynamic is optional in the schema; an absent value is treated as static (a
/// sign) — the conservative default, and the reader already warns on the
/// missing attribute.
std::string_view signal_model_id(const Signal& signal) {
  const signs::SignDef* def = signal_designation(signal);
  if (def != nullptr && props::model(def->model_id) != nullptr) {
    return def->model_id;
  }
  return signal.dynamic.value_or(false) ? "signal_light" : "sign_generic";
}

/// The legend a sign's face shows.
///
/// An authored @text always wins. A speed-limit designation with no @text
/// derives its legend from @value instead, so the value the properties pane
/// edits is what the face reads. §1.4 puts **mph on the face regardless of the
/// editor's display-unit toggle**, which is why this takes the authored value
/// straight: it never converts, and it never consults a UI setting (the kernel
/// could not, and must not, see one).
std::string signal_face_text(const Signal& signal) {
  if (!signal.text.empty()) {
    return signal.text;
  }
  const signs::SignDef* def = signal_designation(signal);
  if (def == nullptr || !def->default_value.has_value() || !signal.value.has_value()) {
    return {};
  }
  return fmt::format("SPEED\nLIMIT\n{:.0f}", *signal.value);
}

/// A model-space text-face quad for a sign plate: a rectangle +0.005 m in front
/// of the plate's +x face, spanning y (−half_w→+half_w) and z (centred on
/// plate.z). Normals point +x (front-facing); UVs are [0,1] with v=0 at the TOP
/// (z=+half_h) so the row-0-top rasterised bitmap drapes with no flip. Winding is
/// CCW seen from +x. The bitmap is not baked here — consumers raster `text`.
SignalFaceOverlay make_face_overlay(const std::string& text, const props::FacePlate& plate) {
  const double x = plate.x + 0.005; // just in front of the plate face
  const double z_top = plate.z + plate.half_h;
  const double z_bot = plate.z - plate.half_h;
  const double y_left = -plate.half_w; // u = 0
  const double y_right = plate.half_w; // u = 1
  SignalFaceOverlay face;
  face.text = text;
  // Vertices: 0 top-left, 1 bottom-left, 2 bottom-right, 3 top-right.
  face.positions = {x, y_left, z_top, x, y_left, z_bot, x, y_right, z_bot, x, y_right, z_top};
  face.normals = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
  face.uvs = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0};
  face.indices = {0, 1, 2, 0, 2, 3};
  return face;
}

/// Placed signals a road owns, as INSTANCES of bundled signal models — the
/// signal analogue of build_object_instances. The pole base sits on the road
/// surface at the signal's s/t, lifted by z_offset.
///
/// The FACING follows ASAM OpenDRIVE 1.9.0 §14.1 exactly: @orientation names
/// the traffic the signal applies to, and a signal so marked with @hOffset 0
/// "faces to the drivers travelling in" that direction — so "+" traffic runs
/// toward +s and the face therefore looks back down -s. §14.1 says as much
/// about the offset's datum: for "+", @hOffset is applied counter-clockwise
/// from the NEGATIVE reference line direction, i.e. from where the face
/// already points. "-" and "none" measure from the positive direction.
///
/// Until p6-s14 (#416) this ignored @orientation entirely and aimed every
/// signal along +s, which rendered the "-" datum in all three cases.
void build_signal_instances(const RoadNetwork& network,
                            const Road& road,
                            RoadId road_id,
                            std::vector<SignalInstance>& out) {
  for (const SignalId signal_id : signals_of(network, road_id)) {
    const Signal& signal = *network.signal(signal_id);
    const StationFrame frame = make_frame(road, signal.s);
    std::array<double, 3> position = lateral_point(frame, signal.t);
    position[2] += signal.z_offset;
    const double datum = std::atan2(frame.sin_h, frame.cos_h) +
                         (signal.orientation == ObjectOrientation::Plus ? std::numbers::pi : 0.0);
    const double heading = datum + signal.h_offset;
    SignalInstance instance{.signal = signal_id,
                            .road = road_id,
                            .model_id = std::string(signal_model_id(signal)),
                            .position = position,
                            .heading = heading};
    // The face: a STATIC sign whose model carries a face plate AND has
    // something to show on it.
    //
    // The gate used to be "non-empty @text", from when a face could only ever
    // be typed words. It cannot be that any more: a US pack sign's face is
    // baked artwork plus a fixed legend (a Do Not Enter roundel, a ONE WAY
    // arrow), and those carry no @text at all. What the signal contributes is
    // the EDITABLE layer — its @text, or a speed limit's @value — which may
    // legitimately be empty.
    //
    // A plate with none of the three still gets no face, which is the point:
    // rendering a flat fill as a texture would cost an image per sign and look
    // identical to the plate underneath. Dynamic signals (traffic lights) never
    // take a face.
    if (!signal.dynamic.value_or(false)) {
      const props::PropModel* model = props::model(instance.model_id);
      if (model != nullptr && model->face_plate.has_value()) {
        const props::FacePlate& plate = *model->face_plate;
        const std::string face_text = signal_face_text(signal);
        if (!plate.symbol.empty() || !plate.legend.empty() || !face_text.empty()) {
          instance.face = make_face_overlay(face_text, plate);
        }
      }
    }
    out.push_back(std::move(instance));
  }
}

// The raised-pavement presentation can use an existing crosswalk as the
// location of a flush kerb. All dimensions are derived from the authored road
// object; the OpenDRIVE geometry itself is not modified.
constexpr double kCurbRampRun = 3.0; // smooth 15 cm rise, max slope 7.5 %
constexpr double kCurbEdgeReach = 0.35; // crosswalk extent must actually reach the kerb
struct CrosswalkCurbZone {
  double s0, s1;
  double t0, t1;
};

std::vector<CrosswalkCurbZone> curb_zones(const RoadNetwork& network,
                                          RoadId road_id,
                                          const MeshOptions& options) {
  std::vector<CrosswalkCurbZone> result;
  if (!options.raised_sidewalks) {
    return result;
  }
  for (const ObjectId id : objects_of(network, road_id)) {
    const Object& object = *network.object(id);
    // Free-form marking curves are painted stripes, not ordinary rectangular
    // pedestrian crossings. A lateral span is essential to identify the curb.
    if (object.type != ObjectType::Crosswalk || object.marking_curve.has_value() ||
        !object.length.has_value() || !object.width.has_value() ||
        *object.length <= tol::kLength || *object.width <= tol::kLength ||
        std::abs(std::sin(object.hdg)) < 0.95) {
      // A crossing drawn along the road cannot be treated as a lateral
      // pedestrian crossing; do not cut a curb just from its box dimensions.
      continue;
    }
    result.push_back(CrosswalkCurbZone{
        .s0 = object.s - (*object.width / 2.0),
        .s1 = object.s + (*object.width / 2.0),
        .t0 = object.t - (*object.length / 2.0),
        .t1 = object.t + (*object.length / 2.0)});
  }
  return result;
}

// A raised kerb is kept at full height before the taper, lowered to road
// level throughout the painted crossing, and restored afterwards. The smooth
// cubic has zero slope at both ends of each transition.
struct CurbFade {
  double scale = 1.0;
  double derivative = 0.0; // change of scale per metre along the road
};

CurbFade crosswalk_curb_fade(double s, double curb_t,
                             std::span<const CrosswalkCurbZone> zones,
                             bool raised_start, bool raised_end, double road_length) {
  CurbFade chosen;
  const auto smooth = [](double t) {
    const double x = std::clamp(t, 0.0, 1.0);
    return x * x * (3.0 - (2.0 * x));
  };
  const auto gradient = [](double t) {
    const double x = std::clamp(t, 0.0, 1.0);
    return (6.0 * x * (1.0 - x)) / kCurbRampRun;
  };
  for (const CrosswalkCurbZone& crossing : zones) {
    if (std::min(std::abs(curb_t - crossing.t0), std::abs(curb_t - crossing.t1)) >
        kCurbEdgeReach) {
      continue; // the crossing does not meet this side of the road
    }
    // S2.1's junction band expects full height at its arm seam. A crossing
    // too close to that seam needs a junction-corner ramp, not a road-only
    // ramp that would leave a visible vertical tear. That is a separate step.
    if ((raised_start && crossing.s0 < kCurbRampRun - tol::kLength) ||
        (raised_end && crossing.s1 > road_length - kCurbRampRun + tol::kLength)) {
      continue;
    }
    if (s < crossing.s0 - kCurbRampRun || s > crossing.s1 + kCurbRampRun) {
      continue;
    }
    CurbFade candidate;
    if (s < crossing.s0) {
      const double t = (crossing.s0 - s) / kCurbRampRun;
      candidate = {smooth(t), -gradient(t)};
    } else if (s <= crossing.s1) {
      candidate = {0.0, 0.0};
    } else {
      const double t = (s - crossing.s1) / kCurbRampRun;
      candidate = {smooth(t), gradient(t)};
    }
    if (candidate.scale < chosen.scale) {
      chosen = candidate;
    }
  }
  return chosen;
}

/// Sampling stations for one road (mandatory profile knots + adaptive fill).
std::vector<double> road_stations(const RoadNetwork& network, const Road& road,
                                  const MeshOptions& options,
                                  std::span<const CrosswalkCurbZone> zones = {}) {
  SamplingOptions sampling = options.sampling;
  const std::vector<double> extra = mandatory_stations(network, road);
  sampling.extra_stations.insert(sampling.extra_stations.end(), extra.begin(), extra.end());
  for (const CrosswalkCurbZone& zone : zones) {
    // Explicit knots keep the ramp deterministic even for coarse sampling.
    for (const double s : {zone.s0 - kCurbRampRun, zone.s0 - kCurbRampRun / 2.0,
                           zone.s0, (zone.s0 + zone.s1) / 2.0, zone.s1,
                           zone.s1 + kCurbRampRun / 2.0, zone.s1 + kCurbRampRun}) {
      if (s >= 0.0 && s <= road.plan_view.length()) {
        sampling.extra_stations.push_back(s);
      }
    }
  }
  return sample_stations(road.plan_view, sampling);
}

/// One road's tessellation; empty (no lanes, no markings) for degenerate
/// roads — callers drop empty results.
RoadMesh build_one_road(const RoadNetwork& network,
                        RoadId road_id,
                        const Road& road,
                        const MeshOptions& options) {
  RoadMesh mesh;
  mesh.road = road_id;
  if (road.plan_view.empty() || road.sections.empty()) {
    return mesh; // parser already diagnosed these
  }

  const std::vector<CrosswalkCurbZone> crosswalks = curb_zones(network, road_id, options);
  const std::vector<double> stations = road_stations(network, road, options, crosswalks);

  mesh.name = road.name.empty() ? fmt::format("road {}", road.odr_id) : road.name;

  // A connecting road's surface IS the junction floor (the floor is built
  // from the union of exactly these footprints and stitched to the arms) —
  // emitting its lane grid too would draw two coplanar surfaces that
  // z-fight across the whole junction interior (issue #103). Markings still
  // emit below (they are lifted above the surface and the floor carries no
  // marks of its own).
  const bool surface_is_floor = options.junction_floors && road.junction.is_valid();
  // Join a sidewalk to an actual raised corner band at full height. Retain
  // the legacy four-metre fade where no meshed junction band can accept it.
  const auto joins_raised_band = [&](const std::optional<RoadLink>& link) {
    if (!options.raised_sidewalks || !options.junction_floors || !link.has_value()) {
      return false;
    }
    const JunctionId* junction_id = std::get_if<JunctionId>(&link->target);
    if (junction_id == nullptr || network.junction(*junction_id) == nullptr) {
      return false;
    }
    const auto bands = junction_sidewalk_bands(network, *junction_id);
    return std::ranges::any_of(bands, [&](const JunctionSidewalkBand& band) {
      return band.arm_a.road == road_id || band.arm_b.road == road_id;
    });
  };
  const bool raised_predecessor = joins_raised_band(road.predecessor);
  const bool raised_successor = joins_raised_band(road.successor);

  for (std::size_t si = 0; !surface_is_floor && si < road.sections.size(); ++si) {
    const LaneSection& section = *network.lane_section(road.sections[si]);
    const double section_end = si + 1 < road.sections.size()
                                   ? network.lane_section(road.sections[si + 1])->s0
                                   : road.plan_view.length();

    std::vector<double> rows;
    for (const double s : stations) {
      if (s >= section.s0 - tol::kLength && s <= section_end + tol::kLength) {
        rows.push_back(std::clamp(s, section.s0, section_end));
      }
    }
    if (rows.size() < 2) {
      continue;
    }

    const std::uint32_t grid_base = static_cast<std::uint32_t>(mesh.positions.size() / 3);
    std::size_t columns = 0;

    // Vertex grid: one row per station, one column per lane boundary.
    std::vector<std::vector<double>> row_offsets;
    row_offsets.reserve(rows.size());
    for (const double s : rows) {
      const StationFrame frame = make_frame(road, s);
      std::vector<double> offsets = boundary_offsets(network, road, section, s);
      columns = offsets.size();
      for (const double t : offsets) {
        const auto p = lateral_point(frame, t);
        mesh.positions.insert(mesh.positions.end(), p.begin(), p.end());
        const auto n = surface_normal(frame);
        mesh.normals.insert(mesh.normals.end(), n.begin(), n.end());
        // Planar texture coordinates in meters: u follows s along the road, v
        // follows t across it, so textures tile continuously across lane
        // boundaries and welded seams (adjacent roads share s/t at the joint).
        mesh.uvs.insert(mesh.uvs.end(), {s, t});
      }
      row_offsets.push_back(std::move(offsets));
    }

    // One patch per non-center lane; triangles skip degenerate quads.
    const std::size_t center_col = static_cast<std::size_t>(std::ranges::count_if(
        section.lanes, [&](LaneId id) { return network.lane(id)->odr_id > 0; }));
    for (const LaneId lane_id : section.lanes) {
      const Lane& lane = *network.lane(lane_id);
      if (lane.odr_id == 0) {
        continue;
      }
      std::size_t p = 0;
      for (const LaneId other_id : section.lanes) {
        const Lane& other = *network.lane(other_id);
        if ((lane.odr_id > 0) == (other.odr_id > 0) && other.odr_id != 0 &&
            std::abs(other.odr_id) <= std::abs(lane.odr_id)) {
          ++p;
        }
      }
      // Columns of this lane's left and right boundaries in the grid.
      const std::size_t col_left = lane.odr_id > 0 ? center_col - p : center_col + p - 1;
      const std::size_t col_right = col_left + 1;

      // PrawkoRoadStudio cross-section: the sidewalk has a separate raised
      // top plus a vertical curb and a beveled lip, all swept along the true
      // reference-line frames. The old flat grid stays the road datum and is
      // still shared by the driving lanes and OpenDRIVE lane boundaries.
      // These vertices are a presentation layer; they never mutate .xodr.
      const bool raised_sidewalk = options.raised_sidewalks && lane.type == LaneType::Sidewalk;
      std::vector<std::array<std::uint32_t, 2>> raised_top;
      std::vector<std::array<std::uint32_t, 2>> curb_wall;
      std::vector<std::array<std::uint32_t, 2>> curb_bevel;
      if (raised_sidewalk) {
        raised_top.reserve(rows.size());
        curb_wall.reserve(rows.size());
        curb_bevel.reserve(rows.size());
        const double side = lane.odr_id > 0 ? 1.0 : -1.0;
        const auto junction_end = [](const std::optional<RoadLink>& link) {
          return link.has_value() && std::holds_alternative<JunctionId>(link->target);
        };
        const auto vertex = [&](const StationFrame& frame,
                                double s, double t, double z,
                                const std::array<double, 3>& normal) {
          auto p = lateral_point(frame, t);
          p[2] += z;
          const auto id = static_cast<std::uint32_t>(mesh.positions.size() / 3);
          mesh.positions.insert(mesh.positions.end(), p.begin(), p.end());
          mesh.normals.insert(mesh.normals.end(), normal.begin(), normal.end());
          mesh.uvs.insert(mesh.uvs.end(), {s, t});
          return id;
        };
        for (std::size_t ri = 0; ri < rows.size(); ++ri) {
          const double s = rows[ri];
          const StationFrame frame = make_frame(road, s);
          const double t_left = row_offsets[ri][col_left];
          const double t_right = row_offsets[ri][col_right];
          const double width = std::abs(t_left - t_right);
          const double inner_t = side > 0.0 ? t_right : t_left;
          double fade = 1.0;
          // A raised junction band now joins the road at full curb height.
          // For junctions without a corresponding band keep the old 4 m fade
          // rather than leave an exposed vertical cap at an unmeshed mouth.
          constexpr double kJoinRampM = 4.0;
          if (junction_end(road.predecessor) && !raised_predecessor) {
            fade = std::min(fade, std::clamp(s / kJoinRampM, 0.0, 1.0));
          }
          if (junction_end(road.successor) && !raised_successor) {
            fade = std::min(
                fade, std::clamp((road.plan_view.length() - s) / kJoinRampM, 0.0, 1.0));
          }
          // Lower the entire sidewalk lane to the crossing datum and ease the
          // 15 cm curb over three metres. Skip crossings that do not reach
          // this side's curb or cannot safely rejoin a raised junction band.
          const CurbFade crosswalk = crosswalk_curb_fade(
              s, inner_t, crosswalks, raised_predecessor, raised_successor,
              road.plan_view.length());
          const double road_fade = fade;
          fade = std::min(fade, crosswalk.scale);
          const double height = defaults::kCurbHeight * fade;
          // The junction's first 3-D curb pass has a vertical face. At the
          // shared arm face, taper the road's 5 cm chamfer down to zero over
          // the final metre, so the upper edge and the wall coincide with
          // the junction seam rather than leaving a narrow open end-cap.
          constexpr double kBevelTransitionM = 1.0;
          double bevel_fade = 1.0;
          if (raised_predecessor) {
            bevel_fade = std::min(bevel_fade, std::clamp(s / kBevelTransitionM, 0.0, 1.0));
          }
          if (raised_successor) {
            bevel_fade = std::min(
                bevel_fade, std::clamp((road.plan_view.length() - s) / kBevelTransitionM, 0.0, 1.0));
          }
          const double wall_height = height * (1.0 - (0.20 * bevel_fade));
          const double bevel_width = std::min(0.05 * fade * bevel_fade, width * 0.20);
          const double top_inner_t = inner_t + side * bevel_width;
          const double top_left_t = side > 0.0 ? t_left : top_inner_t;
          const double top_right_t = side > 0.0 ? top_inner_t : t_right;
          // The curb drop tilts the walking surface along s. Its normal must
          // reflect that longitudinal grade for correct light response; the
          // ordinary road-frame normal alone would falsely shade it as flat.
          const auto base_normal = surface_normal(frame);
          const double extra_grade =
              crosswalk.scale <= road_fade + tol::kLength
                  ? defaults::kCurbHeight * crosswalk.derivative
                  : 0.0;
          const std::array<double, 3> tilted{
              base_normal[0] - (extra_grade * frame.cos_h),
              base_normal[1] - (extra_grade * frame.sin_h),
              base_normal[2]};
          const double normal_length = std::hypot(tilted[0], tilted[1], tilted[2]);
          const std::array<double, 3> top_normal{
              tilted[0] / normal_length, tilted[1] / normal_length,
              tilted[2] / normal_length};
          const std::array<double, 3> inward{
              side * frame.sin_h, -side * frame.cos_h, 0.0};
          const double slope = height - wall_height;
          const double denom = std::hypot(bevel_width, slope);
          const double weight_in = denom > 1e-12 ? slope / denom : 0.0;
          const double weight_up = denom > 1e-12 ? bevel_width / denom : 1.0;
          const std::array<double, 3> bevel_normal{
              (weight_in * inward[0]) + (weight_up * top_normal[0]),
              (weight_in * inward[1]) + (weight_up * top_normal[1]),
              (weight_in * inward[2]) + (weight_up * top_normal[2])};
          const auto top_l = vertex(frame, s, top_left_t, height, top_normal);
          const auto top_r = vertex(frame, s, top_right_t, height, top_normal);
          raised_top.push_back({top_l, top_r});

          const auto wall_bottom = vertex(frame, s, inner_t, 0.0, inward);
          const auto wall_top = vertex(frame, s, inner_t, wall_height, inward);
          curb_wall.push_back(side > 0.0
                                  ? std::array<std::uint32_t, 2>{wall_top, wall_bottom}
                                  : std::array<std::uint32_t, 2>{wall_bottom, wall_top});

          const auto lip_bottom = vertex(frame, s, inner_t, wall_height, bevel_normal);
          const auto lip_top = vertex(frame, s, top_inner_t, height, bevel_normal);
          curb_bevel.push_back(side > 0.0
                                   ? std::array<std::uint32_t, 2>{lip_top, lip_bottom}
                                   : std::array<std::uint32_t, 2>{lip_bottom, lip_top});
        }
      }

      // Surface code covering a section-local station: the <material> record
      // with the greatest sOffset <= s (§11.8.2, "valid until the next element
      // starts"). Empty when the lane has no record there — the renderer then
      // uses the lane-type palette. Material boundaries are mandatory stations,
      // so each quad resolves to a single code with an exact edge.
      const auto surface_at = [&lane](double s_local) -> std::string {
        const LaneMaterial* best = nullptr;
        for (const LaneMaterial& material : lane.materials) {
          if (material.s_offset <= s_local + tol::kLength &&
              (best == nullptr || material.s_offset > best->s_offset)) {
            best = &material;
          }
        }
        return best != nullptr ? best->surface.value_or(std::string{}) : std::string{};
      };
      const auto new_patch = [&](std::string surface) {
        RoadMesh::LanePatch patch;
        patch.lane = lane_id;
        patch.odr_lane_id = lane.odr_id;
        patch.material = lane.type;
        patch.surface = std::move(surface);
        return patch;
      };

      RoadMesh::LanePatch patch = new_patch(std::string{});
      bool patch_open = false;
      for (std::uint32_t row = 0; row + 1 < rows.size(); ++row) {
        const double width_here =
            std::abs(row_offsets[row][col_left] - row_offsets[row][col_right]);
        const double width_next =
            std::abs(row_offsets[row + 1][col_left] - row_offsets[row + 1][col_right]);
        if (width_here < tol::kLength && width_next < tol::kLength) {
          continue; // fully pinched-off quad
        }
        // Contiguous quads sharing a record merge into one patch; a change of
        // surface code flushes the current patch and opens a new one.
        std::string surface = surface_at(rows[row] - section.s0);
        if (patch_open && surface != patch.surface) {
          mesh.lanes.push_back(std::move(patch));
          patch = new_patch(std::move(surface));
        } else if (!patch_open) {
          patch.surface = std::move(surface);
          patch_open = true;
        }
        const std::uint32_t a = raised_sidewalk
                                    ? raised_top[row][0]
                                    : grid_base + (row * static_cast<std::uint32_t>(columns)) +
                                          static_cast<std::uint32_t>(col_left);
        const std::uint32_t b = raised_sidewalk
                                    ? raised_top[row][1]
                                    : grid_base + (row * static_cast<std::uint32_t>(columns)) +
                                          static_cast<std::uint32_t>(col_right);
        const std::uint32_t c = raised_sidewalk
                                    ? raised_top[row + 1][1]
                                    : b + static_cast<std::uint32_t>(columns);
        const std::uint32_t d = raised_sidewalk
                                    ? raised_top[row + 1][0]
                                    : a + static_cast<std::uint32_t>(columns);
        patch.indices.insert(patch.indices.end(), {a, b, c, a, c, d});
      }
      if (!patch.indices.empty()) {
        mesh.lanes.push_back(std::move(patch));
      }
      if (raised_sidewalk) {
        // Each curb face has its own vertices: normals stay sharp at the
        // vertical rise and the chamfer instead of averaging with the walk.
        const auto emit_strip = [&](const auto& edge, std::string_view part) {
          RoadMesh::LanePatch face = new_patch("concrete");
          face.material = LaneType::Curb;
          (void)part; // the material class names the two strips consistently
          const auto add_non_degenerate = [&](std::uint32_t a, std::uint32_t b,
                                              std::uint32_t c) {
            const auto p = [&](std::uint32_t i, int axis) {
              return mesh.positions[(3 * i) + static_cast<std::uint32_t>(axis)];
            };
            const double x1 = p(b, 0) - p(a, 0);
            const double y1 = p(b, 1) - p(a, 1);
            const double z1 = p(b, 2) - p(a, 2);
            const double x2 = p(c, 0) - p(a, 0);
            const double y2 = p(c, 1) - p(a, 1);
            const double z2 = p(c, 2) - p(a, 2);
            const double nx = (y1 * z2) - (z1 * y2);
            const double ny = (z1 * x2) - (x1 * z2);
            const double nz = (x1 * y2) - (y1 * x2);
            if ((nx * nx) + (ny * ny) + (nz * nz) > 1e-18) {
              face.indices.insert(face.indices.end(), {a, b, c});
            }
          };
          for (std::uint32_t row = 0; row + 1 < rows.size(); ++row) {
            const auto [a, b] = edge[row];
            const auto [d, c] = edge[row + 1];
            add_non_degenerate(a, b, c);
            add_non_degenerate(a, c, d);
          }
          if (!face.indices.empty()) {
            mesh.lanes.push_back(std::move(face));
          }
        };
        emit_strip(curb_wall, "wall");
        emit_strip(curb_bevel, "bevel");
      }
    }
  }

  if (options.markings) {
    build_markings(network, road, road_id, stations, mesh);
    build_object_markings(network, road, road_id, mesh);
    build_stopline_markings(network, road, road_id, mesh);
  }
  return mesh;
}

/// One junction's blended 3D surface, keyed by id; empty mesh (no indices)
/// when the junction has no usable connecting-road footprints — callers drop
/// empty results. The pipeline itself lives in junction_surface.cpp.
JunctionFloor build_one_junction_floor(const RoadNetwork& network,
                                       JunctionId junction_id,
                                       const Junction& junction,
                                       const SamplingOptions& sampling,
                                       bool raised_sidewalks) {
  // Sidewalked junctions (issue #357): the floor is split into its carriageway
  // core (kept as `mesh`, Driving) and per-corner sidewalk bands (Sidewalk),
  // which join the authored median-nose overlays in `details`. A rural junction
  // splits to itself, so `mesh` stays byte-identical to the un-split floor.
  JunctionFloorSplit split = build_junction_floor_split(network, junction, sampling, raised_sidewalks);
  std::vector<SubMesh> details = std::move(split.sidewalk_bands);
  for (SubMesh& curb : split.curb_faces) {
    details.push_back(std::move(curb));
  }
  for (SubMesh& overlay : build_junction_corner_details(network, junction, sampling)) {
    details.push_back(std::move(overlay));
  }
  return JunctionFloor{
      .junction = junction_id, .mesh = std::move(split.carriageway), .details = std::move(details)};
}

/// One enclosed-area ground surface, keyed by id; empty mesh (no indices) when
/// the ring encloses no area — callers drop empty results. The pipeline itself
/// lives in surface_fill.cpp.
SurfaceMesh build_one_surface(const RoadNetwork& network,
                              SurfaceId surface_id,
                              const Surface& surface,
                              const SamplingOptions& sampling) {
  SubMesh mesh = build_surface_mesh(network, surface, sampling);
  if (!mesh.indices.empty()) {
    mesh.name = fmt::format("surface {}", surface_id.index);
  }
  return SurfaceMesh{.surface = surface_id, .mesh = std::move(mesh)};
}

bool road_mesh_is_empty(const RoadMesh& mesh) {
  return mesh.lanes.empty() && mesh.markings.empty();
}

} // namespace

NetworkMesh build_network_mesh(const RoadNetwork& network, const MeshOptions& options) {
  NetworkMesh result;
  network.for_each_road([&](RoadId road_id, const Road& road) {
    RoadMesh mesh = build_one_road(network, road_id, road, options);
    if (!road_mesh_is_empty(mesh)) {
      result.roads.push_back(std::move(mesh));
    }
    build_object_instances(network, road, road_id, result.objects);
    build_signal_instances(network, road, road_id, result.signal_instances);
  });
  if (options.junction_floors) {
    network.for_each_junction([&](JunctionId junction_id, const Junction& junction) {
      JunctionFloor floor =
          build_one_junction_floor(network, junction_id, junction,
                                   options.sampling, options.raised_sidewalks);
      if (!floor.mesh.indices.empty()) {
        result.junction_floors.push_back(std::move(floor));
      }
    });
  }
  // Enclosed-area ground surfaces: mesh whatever surfaces already exist in the
  // arena (derive_surfaces owns them; meshing stays const on the network).
  network.for_each_surface([&](SurfaceId surface_id, const Surface& surface) {
    SurfaceMesh built = build_one_surface(network, surface_id, surface, options.sampling);
    if (!built.mesh.indices.empty()) {
      result.surfaces.push_back(std::move(built));
    }
  });
  // Terrain: it fills the ground AROUND the roads and surfaces, so it reads
  // their footprints and must run after they exist. Empty unless the network
  // carries a height field.
  remesh_terrain(network, result, options);
  // Bridge solids LAST: piers read the terrain height field for their footings,
  // so terrain must exist first. Empty unless a road carries a <bridge>.
  remesh_bridges(network, result, options);
  return result;
}

void remesh_roads(const RoadNetwork& network,
                  NetworkMesh& mesh,
                  std::span<const RoadId> roads,
                  const MeshOptions& options) {
  for (const RoadId road_id : roads) {
    const auto existing = std::ranges::find(mesh.roads, road_id, &RoadMesh::road);
    const Road* road = network.road(road_id);
    RoadMesh rebuilt =
        road != nullptr ? build_one_road(network, road_id, *road, options) : RoadMesh{};
    if (road == nullptr || road_mesh_is_empty(rebuilt)) {
      if (existing != mesh.roads.end()) {
        // vector::erase move-assigns the tail entries; their heap buffers
        // are stolen, not copied, so untouched roads keep their data().
        mesh.roads.erase(existing);
      }
      continue;
    }
    if (existing != mesh.roads.end()) {
      *existing = std::move(rebuilt);
    } else {
      mesh.roads.push_back(std::move(rebuilt));
    }
  }
}

void remesh_object_instances(const RoadNetwork& network,
                             NetworkMesh& mesh,
                             std::span<const RoadId> roads) {
  bool touched = false;
  for (const RoadId road_id : roads) {
    const Road* road = network.road(road_id);
    // Instanced-prop layer: drop this road's placed props and signals and
    // re-derive them from the network. Idempotent — it reads only stored state,
    // so running it twice in a tick (an edit that named both channels) costs a
    // rebuild, never a wrong answer.
    std::size_t moved = std::erase_if(
        mesh.objects, [&](const ObjectInstance& inst) { return inst.road == road_id; });
    moved += std::erase_if(mesh.signal_instances,
                           [&](const SignalInstance& inst) { return inst.road == road_id; });
    if (road != nullptr) {
      const std::size_t objects_before = mesh.objects.size();
      const std::size_t signals_before = mesh.signal_instances.size();
      build_object_instances(network, *road, road_id, mesh.objects);
      build_signal_instances(network, *road, road_id, mesh.signal_instances);
      moved +=
          (mesh.objects.size() - objects_before) + (mesh.signal_instances.size() - signals_before);
    }
    touched = touched || moved > 0;
  }
  if (!touched) {
    return; // the listed roads carry no placements — nothing to reorder
  }
  // Re-deriving APPENDS, so restore the order a full build_network_mesh()
  // produces: instances grouped by their road in arena order (Arena::for_each
  // walks slots ascending). A stable sort is enough — it preserves each road's
  // own object order, which is the order build_object_instances just emitted.
  //
  // This is not cosmetic. The channel's order is observable in exported files:
  // the USD backend names every prim after its index (`prop_<i>`, `signal_<i>`
  // in usd_exporter.cpp) and the glTF backend emits one node per instance in
  // sequence, so without this a translated road would silently renumber every
  // prop that follows it — the same scene exporting differently depending on
  // whether it was edited or freshly loaded.
  std::ranges::stable_sort(
      mesh.objects, {}, [](const ObjectInstance& inst) { return inst.road.index; });
  std::ranges::stable_sort(
      mesh.signal_instances, {}, [](const SignalInstance& inst) { return inst.road.index; });
}

void remesh_objects(const RoadNetwork& network,
                    NetworkMesh& mesh,
                    std::span<const RoadId> roads,
                    const MeshOptions& options) {
  // An object was added, moved or removed: the placements go first (independent
  // of the road's surface mesh), then the marking-only fast path below.
  remesh_object_instances(network, mesh, roads);
  for (const RoadId road_id : roads) {
    const auto existing = std::ranges::find(mesh.roads, road_id, &RoadMesh::road);
    const Road* road = network.road(road_id);

    if (existing == mesh.roads.end()) {
      // Road not meshed yet — fall back to a full build so the object markings
      // still land (keeps the entry point total for a fresh network).
      if (road != nullptr) {
        RoadMesh rebuilt = build_one_road(network, road_id, *road, options);
        if (!road_mesh_is_empty(rebuilt)) {
          mesh.roads.push_back(std::move(rebuilt));
        }
      }
      continue;
    }
    // A crosswalk is also the source of the S2.2 sidewalk curb-drop shape.
    // Object edits can add, move, or REMOVE a crossing. Rebuild the whole
    // shaped road even when none remain, or an old curb depression would stay
    // stuck in the viewport until an unrelated edit triggers a full remesh.
    if (options.raised_sidewalks) {
      if (road != nullptr) {
        *existing = build_one_road(network, road_id, *road, options);
      } else {
        mesh.roads.erase(existing);
      }
      continue;
    }
    // Legacy object-layer fast path: regenerate ONLY marking submeshes.
    existing->markings.clear();
    if (road != nullptr && options.markings && !road->plan_view.empty() &&
        !road->sections.empty()) {
      const std::vector<double> stations = road_stations(network, *road, options);
      build_markings(network, *road, road_id, stations, *existing);
      build_object_markings(network, *road, road_id, *existing);
      build_stopline_markings(network, *road, road_id, *existing);
    }
  }
}

void remesh_junctions(const RoadNetwork& network,
                      NetworkMesh& mesh,
                      std::span<const JunctionId> junctions,
                      const MeshOptions& options) {
  for (const JunctionId junction_id : junctions) {
    const auto existing =
        std::ranges::find(mesh.junction_floors, junction_id, &JunctionFloor::junction);
    const Junction* junction = options.junction_floors ? network.junction(junction_id) : nullptr;
    JunctionFloor rebuilt =
        junction != nullptr
            ? build_one_junction_floor(network, junction_id, *junction,
                                       options.sampling, options.raised_sidewalks)
            : JunctionFloor{.junction = junction_id, .mesh = {}};
    if (rebuilt.mesh.indices.empty()) {
      if (existing != mesh.junction_floors.end()) {
        mesh.junction_floors.erase(existing);
      }
      continue;
    }
    if (existing != mesh.junction_floors.end()) {
      *existing = std::move(rebuilt);
    } else {
      mesh.junction_floors.push_back(std::move(rebuilt));
    }
  }
}

void remesh_surfaces(const RoadNetwork& network,
                     NetworkMesh& mesh,
                     std::span<const SurfaceId> surfaces,
                     const MeshOptions& options) {
  for (const SurfaceId surface_id : surfaces) {
    const auto existing = std::ranges::find(mesh.surfaces, surface_id, &SurfaceMesh::surface);
    const Surface* surface = network.surface(surface_id);
    SurfaceMesh rebuilt = surface != nullptr
                              ? build_one_surface(network, surface_id, *surface, options.sampling)
                              : SurfaceMesh{.surface = surface_id, .mesh = {}};
    if (rebuilt.mesh.indices.empty()) {
      if (existing != mesh.surfaces.end()) {
        mesh.surfaces.erase(existing);
      }
      continue;
    }
    if (existing != mesh.surfaces.end()) {
      *existing = std::move(rebuilt);
    } else {
      mesh.surfaces.push_back(std::move(rebuilt));
    }
  }
}

} // namespace roadmaker
