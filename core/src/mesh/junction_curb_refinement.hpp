/*
 * Copyright 2026 Robomous
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

// S2.2f: render-mesh refinement ONLY AFTER the unchanged constrained-Delaunay
// floor has been classified into driving and sidewalk triangles. Each midpoint
// belongs to the SHARED seam, so both sides receive the identical new vertex.
// The OpenDRIVE model, corner solver and CDT constraints are not modified.

#include "roadmaker/mesh/mesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace roadmaker::junction_curb_refinement {
namespace detail {
using Vertex = std::uint32_t;
using Edge = std::pair<Vertex, Vertex>;
struct Side {
  std::size_t triangle;
  Vertex opposite;
  int region;
};
inline Edge edge(Vertex a, Vertex b) { return {std::min(a, b), std::max(a, b)}; }
inline std::array<double, 2> xy(const SubMesh& mesh, Vertex v) {
  return {mesh.positions[3 * v], mesh.positions[3 * v + 1]};
}
inline double cross(const std::array<double, 2>& a,
                    const std::array<double, 2>& b,
                    const std::array<double, 2>& c) {
  return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
}
inline double min_angle(const std::array<double, 2>& a,
                        const std::array<double, 2>& b,
                        const std::array<double, 2>& c) {
  const auto angle = [](const auto& origin, const auto& p, const auto& q) {
    const double px = p[0] - origin[0], py = p[1] - origin[1];
    const double qx = q[0] - origin[0], qy = q[1] - origin[1];
    return std::atan2(std::abs(px*qy - py*qx), px*qx + py*qy);
  };
  return std::min({angle(a,b,c),angle(b,c,a),angle(c,a,b)});
}
inline bool valid_half(const std::array<double, 2>& a,
                       const std::array<double, 2>& mid,
                       const std::array<double, 2>& b,
                       const std::array<double, 2>& opposite) {
  // Extra safety margin: the 5-degree public sliver gate also sees the
  // subsequent 5 cm sidewalk inset, which can reduce angles slightly.
  constexpr double kMinSubdivisionAngle = 0.13962634015954636; // 8 degrees
  const double original = cross(a, b, opposite);
  const double first = cross(a, mid, opposite);
  const double second = cross(mid, b, opposite);
  if (std::abs(original) < 1e-10 ||
      first * original < 0.0 || second * original < 0.0 ||
      std::abs(first) < std::abs(original) * 0.22 ||
      std::abs(second) < std::abs(original) * 0.22) return false;
  return min_angle(a, mid, opposite) >= kMinSubdivisionAngle &&
         min_angle(mid, b, opposite) >= kMinSubdivisionAngle;
}
} // namespace detail

/// Returns inserted seam vertices; zero means no sufficiently curved,
/// safe isolated seam on this junction. No changes for straight or legacy
/// meshes. The caller must pass the triangle classification from the
/// original constrained Delaunay surface.
inline std::size_t refine(SubMesh& floor, std::vector<int>& assignment) {
  using namespace detail;
  if (floor.indices.size() != assignment.size() * 3 || floor.indices.empty())
    return 0;
  std::map<Edge, std::vector<Side>> sides;
  for (std::size_t t = 0; t < assignment.size(); ++t) {
    const auto a = floor.indices[t * 3], b = floor.indices[t * 3 + 1];
    const auto c = floor.indices[t * 3 + 2];
    sides[edge(a,b)].push_back({t,c,assignment[t]});
    sides[edge(b,c)].push_back({t,a,assignment[t]});
    sides[edge(c,a)].push_back({t,b,assignment[t]});
  }
  std::map<Edge, std::array<Side, 2>> seams;
  std::map<int, std::map<Vertex, std::vector<Vertex>>> adjacency;
  for (const auto& [key, pair] : sides) {
    if (pair.size() != 2) continue;
    const Side& one = pair[0], &two = pair[1];
    if (!((one.region >= 0 && two.region == -1) ||
          (two.region >= 0 && one.region == -1))) continue;
    const int region = one.region >= 0 ? one.region : two.region;
    seams.emplace(key, std::array<Side,2>{one,two});
    adjacency[region][key.first].push_back(key.second);
    adjacency[region][key.second].push_back(key.first);
  }
  // Never refine branched seams: winding and cap geometry remain untouched.
  std::map<Edge, Vertex> midpoint;
  std::set<std::size_t> used_triangles;
  const std::size_t source_vertices = floor.positions.size() / 3;
  const bool uv_complete = floor.uvs.size() == source_vertices * 2;
  const bool normals_complete = floor.normals.size() == source_vertices * 3;
  if (!normals_complete || (!floor.uvs.empty() && !uv_complete)) return 0;
  for (const auto& [key, pair] : seams) {
    if (used_triangles.contains(pair[0].triangle) ||
        used_triangles.contains(pair[1].triangle)) continue;
    const int region = pair[0].region >= 0 ? pair[0].region : pair[1].region;
    const auto& graph = adjacency.at(region);
    const auto ai = graph.find(key.first), bi = graph.find(key.second);
    if (ai == graph.end() || bi == graph.end() ||
        ai->second.size() != 2 || bi->second.size() != 2) continue;
    const auto prev = ai->second[0] == key.second ? ai->second[1] : ai->second[0];
    const auto next = bi->second[0] == key.first ? bi->second[1] : bi->second[0];
    if (prev == next || prev == key.second || next == key.first) continue;
    const auto a = xy(floor,key.first), b = xy(floor,key.second);
    const auto p = xy(floor,prev), q = xy(floor,next);
    const double length = std::hypot(a[0]-b[0], a[1]-b[1]);
    if (length < 0.45 || length > 5.0) continue;
    // Catmull-Rom midpoint on a smooth arc. It equals the straight
    // midpoint for a straight seam and approximates true circular arc
    // sagitta to fourth order. Conservative cap: 8 mm, 1% chord length.
    std::array<double,2> shift{
        (a[0] + b[0] - p[0] - q[0]) / 16.0,
        (a[1] + b[1] - p[1] - q[1]) / 16.0};
    // Uneven CDT chord lengths create a tangential Catmull-Rom offset even
    // on a perfectly straight seam. Keep ONLY normal curvature; otherwise
    // we would add vertices to straight arm mouths without improving shape.
    const double nx = -(b[1] - a[1]) / length;
    const double ny = (b[0] - a[0]) / length;
    const double normal_offset = shift[0] * nx + shift[1] * ny;
    shift = {normal_offset * nx, normal_offset * ny};
    const double mag = std::abs(normal_offset);
    if (mag < 0.0008) continue;
    const double cap = std::min(0.008, length * 0.01);
    if (mag > cap) { shift[0] *= cap/mag; shift[1] *= cap/mag; }
    std::array<double,2> mid{(a[0]+b[0])*0.5+shift[0],
                              (a[1]+b[1])*0.5+shift[1]};
    // The two original triangles must both remain well conditioned.
    bool safe = true;
    for (const Side& side : pair) {
      if (!valid_half(a,mid,b,xy(floor,side.opposite))) {
        safe=false;
        break;
      }
    }
    if (!safe) continue;
    const Vertex m = static_cast<Vertex>(floor.positions.size()/3);
    midpoint[key]=m;
    for (int axis=0;axis<3;++axis) {
      const double z = (floor.positions[3*key.first+axis] +
                        floor.positions[3*key.second+axis]) * 0.5;
      floor.positions.push_back(axis == 0 ? mid[0] : axis == 1 ? mid[1] : z);
      if (normals_complete)
        floor.normals.push_back((floor.normals[3*key.first+axis] +
                                 floor.normals[3*key.second+axis])*0.5);
    }
    if (uv_complete) {
      for (int axis=0;axis<2;++axis)
        floor.uvs.push_back((floor.uvs[2*key.first+axis] +
                             floor.uvs[2*key.second+axis])*0.5);
    }
    used_triangles.insert(pair[0].triangle);
    used_triangles.insert(pair[1].triangle);
  }
  if (midpoint.empty()) return 0;
  std::vector<Vertex> indices;
  indices.reserve(floor.indices.size() + midpoint.size() * 6);
  std::vector<int> regions;
  regions.reserve(assignment.size() + midpoint.size() * 2);
  const auto emit = [&](Vertex a,Vertex b,Vertex c,int region) {
    indices.insert(indices.end(),{a,b,c});
    regions.push_back(region);
  };
  for (std::size_t t=0;t<assignment.size();++t) {
    const Vertex a=floor.indices[3*t],b=floor.indices[3*t+1],c=floor.indices[3*t+2];
    const auto ab=midpoint.find(edge(a,b));
    const auto bc=midpoint.find(edge(b,c));
    const auto ca=midpoint.find(edge(c,a));
    // Each triangle can have at most one refined edge. The selection above
    // enforces this globally, and we never create Steiner fans/slivers.
    if (ab!=midpoint.end()) {
      emit(a,ab->second,c,assignment[t]);
      emit(ab->second,b,c,assignment[t]);
    } else if (bc!=midpoint.end()) {
      emit(b,bc->second,a,assignment[t]);
      emit(bc->second,c,a,assignment[t]);
    } else if (ca!=midpoint.end()) {
      emit(c,ca->second,b,assignment[t]);
      emit(ca->second,a,b,assignment[t]);
    } else emit(a,b,c,assignment[t]);
  }
  floor.indices=std::move(indices);
  assignment=std::move(regions);
  return midpoint.size();
}
} // namespace roadmaker::junction_curb_refinement
