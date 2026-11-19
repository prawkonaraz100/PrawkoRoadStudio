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

#include "viewport/gizmo.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "viewport/projection.hpp"

namespace roadmaker::editor {

namespace {

/// Distance from point `p` to the segment [a, b] (all logical px).
double
point_segment_distance(std::array<double, 2> p, std::array<double, 2> a, std::array<double, 2> b) {
  const double abx = b[0] - a[0];
  const double aby = b[1] - a[1];
  const double len_sq = (abx * abx) + (aby * aby);
  if (len_sq < 1e-9) {
    return std::hypot(p[0] - a[0], p[1] - a[1]);
  }
  double t = (((p[0] - a[0]) * abx) + ((p[1] - a[1]) * aby)) / len_sq;
  t = std::clamp(t, 0.0, 1.0);
  const double cx = a[0] + (t * abx);
  const double cy = a[1] + (t * aby);
  return std::hypot(p[0] - cx, p[1] - cy);
}

std::array<double, 2> axis_tip(const CameraMatrices& camera,
                               std::array<double, 3> pivot,
                               std::array<double, 3> axis,
                               std::array<double, 2> origin,
                               double width,
                               double height,
                               double len_px) {
  // Project pivot + 1 m along the world axis; the screen direction, renormalized
  // to a fixed pixel length, gives a zoom-independent arrow. Degenerate (axis
  // near-parallel to the view ray) collapses the tip onto the origin.
  const auto probe = project_to_screen(
      camera, pivot[0] + axis[0], pivot[1] + axis[1], pivot[2] + axis[2], width, height);
  if (!probe.has_value()) {
    return origin;
  }
  const double dx = (*probe)[0] - origin[0];
  const double dy = (*probe)[1] - origin[1];
  const double len = std::hypot(dx, dy);
  if (len < 1e-6) {
    return origin;
  }
  return {origin[0] + (dx / len * len_px), origin[1] + (dy / len * len_px)};
}

} // namespace

GizmoScreen gizmo_screen(const CameraMatrices& camera,
                         std::array<double, 3> pivot,
                         double width,
                         double height,
                         GizmoSizes sizes) {
  GizmoScreen gizmo;
  const auto origin = project_to_screen(camera, pivot[0], pivot[1], pivot[2], width, height);
  if (!origin.has_value()) {
    return gizmo; // behind the camera
  }
  gizmo.valid = true;
  gizmo.origin = *origin;
  gizmo.ring_radius = sizes.ring_radius;
  gizmo.pad_half = sizes.pad_half;
  gizmo.x_tip =
      axis_tip(camera, pivot, {1.0, 0.0, 0.0}, gizmo.origin, width, height, sizes.axis_len);
  gizmo.y_tip =
      axis_tip(camera, pivot, {0.0, 1.0, 0.0}, gizmo.origin, width, height, sizes.axis_len);
  gizmo.z_tip =
      axis_tip(camera, pivot, {0.0, 0.0, 1.0}, gizmo.origin, width, height, sizes.axis_len);
  // The Z and a planar world axis can project to almost the same screen
  // line. In that camera angle the blue handle is visually occluded and
  // impossible to grab reliably. Only when their tips overlap, fan the
  // SCREEN-SPACE Z affordance away from the planar axes. The underlying edit
  // still moves world Z; no model geometry or transform is changed here.
  const auto tip_gap = [](const std::array<double, 2>& a,
                          const std::array<double, 2>& b) {
    return std::hypot(a[0] - b[0], a[1] - b[1]);
  };
  if (std::min(tip_gap(gizmo.z_tip, gizmo.x_tip),
               tip_gap(gizmo.z_tip, gizmo.y_tip)) < 25.0 &&
      tip_gap(gizmo.z_tip, gizmo.origin) > 1e-6) {
    const double ux = (gizmo.z_tip[0] - gizmo.origin[0]) / sizes.axis_len;
    const double uy = (gizmo.z_tip[1] - gizmo.origin[1]) / sizes.axis_len;
    const auto fan_tip = [&](double radians) {
      const double c = std::cos(radians);
      const double sn = std::sin(radians);
      return std::array<double, 2>{
          gizmo.origin[0] + sizes.axis_len * ((c * ux) - (sn * uy)),
          gizmo.origin[1] + sizes.axis_len * ((sn * ux) + (c * uy))};
    };
    const auto left = fan_tip(0.65);
    const auto right = fan_tip(-0.65);
    const auto clearance = [&](const std::array<double, 2>& p) {
      return std::min(tip_gap(p, gizmo.x_tip), tip_gap(p, gizmo.y_tip));
    };
    gizmo.z_tip = clearance(left) >= clearance(right) ? left : right;
  }
  return gizmo;
}

GizmoHandle gizmo_hit_test(const GizmoScreen& gizmo, std::array<double, 2> cursor, double tol) {
  if (!gizmo.valid) {
    return GizmoHandle::None;
  }
  // Centre pad (free planar move) wins near the origin.
  if (std::abs(cursor[0] - gizmo.origin[0]) <= gizmo.pad_half &&
      std::abs(cursor[1] - gizmo.origin[1]) <= gizmo.pad_half) {
    return GizmoHandle::PlaneXY;
  }
  // Arrowheads have a small, explicit grab target. A nearby planar
  // shaft must not steal the blue Z tip even when its centreline is closer.
  // This is checked before the shaft (and before the yaw ring at radius 58).
  const std::array<std::pair<GizmoHandle, std::array<double, 2>>, 3> tips{{
      {GizmoHandle::AxisX, gizmo.x_tip},
      {GizmoHandle::AxisY, gizmo.y_tip},
      {GizmoHandle::AxisZ, gizmo.z_tip},
  }};
  GizmoHandle nearest_tip = GizmoHandle::None;
  double nearest_tip_dist = std::max(12.0, tol);
  for (const auto& [handle, tip] : tips) {
    if (std::hypot(tip[0] - gizmo.origin[0], tip[1] - gizmo.origin[1]) < 1e-6) {
      continue;
    }
    const double d = std::hypot(cursor[0] - tip[0], cursor[1] - tip[1]);
    if (d < nearest_tip_dist) {
      nearest_tip_dist = d;
      nearest_tip = handle;
    }
  }
  if (nearest_tip != GizmoHandle::None) {
    return nearest_tip;
  }
  // Then the nearest axis arm within tolerance.
  const std::array<std::pair<GizmoHandle, std::array<double, 2>>, 3> arms{{
      {GizmoHandle::AxisX, gizmo.x_tip},
      {GizmoHandle::AxisY, gizmo.y_tip},
      {GizmoHandle::AxisZ, gizmo.z_tip},
  }};
  GizmoHandle best = GizmoHandle::None;
  double best_dist = tol;
  for (const auto& [handle, tip] : arms) {
    // Skip a collapsed (degenerate) arm.
    if (std::hypot(tip[0] - gizmo.origin[0], tip[1] - gizmo.origin[1]) < 1e-6) {
      continue;
    }
    const double dist = point_segment_distance(cursor, gizmo.origin, tip);
    if (dist < best_dist) {
      best_dist = dist;
      best = handle;
    }
  }
  if (best != GizmoHandle::None) {
    return best;
  }
  // Finally the yaw ring.
  const double dr = std::abs(std::hypot(cursor[0] - gizmo.origin[0], cursor[1] - gizmo.origin[1]) -
                             gizmo.ring_radius);
  if (dr <= tol) {
    return GizmoHandle::YawRing;
  }
  return GizmoHandle::None;
}

std::array<double, 2> gizmo_constrain_translation(GizmoHandle handle,
                                                  std::array<double, 2> from,
                                                  std::array<double, 2> to) {
  const double dx = to[0] - from[0];
  const double dy = to[1] - from[1];
  switch (handle) {
  case GizmoHandle::AxisX:
    return {dx, 0.0};
  case GizmoHandle::AxisY:
    return {0.0, dy};
  case GizmoHandle::PlaneXY:
    return {dx, dy};
  default:
    return {0.0, 0.0};
  }
}

double
gizmo_yaw_angle(std::array<double, 2> pivot, std::array<double, 2> from, std::array<double, 2> to) {
  const double a0 = std::atan2(from[1] - pivot[1], from[0] - pivot[0]);
  const double a1 = std::atan2(to[1] - pivot[1], to[0] - pivot[0]);
  return wrap_angle(a1 - a0);
}

double snap_to_increment(double value, double increment) {
  if (increment <= 0.0) {
    return value;
  }
  return std::round(value / increment) * increment;
}

double wrap_angle(double radians) {
  // atan2 of the angle's own sine/cosine is the branch-free wrap; it lands in
  // (-pi, pi] and is exact at the boundaries a modulo would straddle.
  return std::atan2(std::sin(radians), std::cos(radians));
}

} // namespace roadmaker::editor
