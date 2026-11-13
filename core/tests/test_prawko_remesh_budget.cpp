/*
 * Copyright 2026 Robomous
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 */

// Reproducible PrawkoRoadStudio 3-D curb-drop object-edit benchmark.
// Timings are informational on dev PCs: runner-dependent performance must not
// silently change geometry, and a hard 16 ms frame budget is NOT claimed here.

#include "roadmaker/edit/markings.hpp"
#include "roadmaker/edit/operations.hpp"
#include "roadmaker/mesh/mesh_builder.hpp"
#include "roadmaker/road/authoring.hpp"
#include "roadmaker/road/network.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace roadmaker;

struct Case {
  RoadNetwork network;
  std::vector<RoadId> dirty;
  MeshOptions options;
};

Case make_case(const std::vector<double>& angles) {
  Case fixture;
  fixture.options.raised_sidewalks = true;
  fixture.options.curb_render_lod = true; // measure the real Studio viewport path
  fixture.options.markings = true;
  std::vector<RoadEnd> ends;
  for (std::size_t i = 0; i < angles.size(); ++i) {
    const double a = angles[i];
    const std::array<Waypoint, 2> waypoints{
        Waypoint{75.0 * std::cos(a), 75.0 * std::sin(a)},
        Waypoint{20.0 * std::cos(a), 20.0 * std::sin(a)}};
    auto result = author_clothoid_road(
        fixture.network, waypoints, LaneProfile::local_road(), "", std::to_string(i + 1));
    if (!result) throw std::runtime_error("cannot author benchmark road");
    ends.push_back(RoadEnd{.road = *result, .contact = ContactPoint::End});
  }
  auto junction_command = edit::create_junction(fixture.network, ends);
  if (!junction_command || !junction_command->apply(fixture.network))
    throw std::runtime_error("cannot create benchmark junction");

  JunctionId junction;
  fixture.network.for_each_junction([&](JunctionId id, const Junction&) { junction = id; });
  edit::CrosswalkParams params;
  params.setback_m = 1.0;
  params.depth_m = 3.0;
  auto crosswalks = edit::junction_crosswalks(fixture.network, junction, params);
  if (crosswalks.size() != angles.size())
    throw std::runtime_error("missing benchmark crosswalks");
  for (const auto& [road, object] : crosswalks) {
    auto command = edit::add_object(fixture.network, road, object);
    if (!command || !command->apply(fixture.network))
      throw std::runtime_error("cannot add benchmark crosswalk");
    fixture.dirty.push_back(road);
  }
  return fixture;
}

void run_case(const char* label, const std::vector<double>& angles) {
  Case f = make_case(angles);
  NetworkMesh incremental = build_network_mesh(f.network, f.options);
  ASSERT_EQ(incremental.junction_floors.size(), 1U);
  ASSERT_EQ(incremental.roads.size(), angles.size());
  remesh_objects(f.network, incremental, f.dirty, f.options);
  const NetworkMesh fresh = build_network_mesh(f.network, f.options);
  ASSERT_EQ(incremental.roads.size(), fresh.roads.size());
  ASSERT_EQ(incremental.junction_floors.size(), fresh.junction_floors.size());
  for (std::size_t i = 0; i < fresh.roads.size(); ++i) {
    EXPECT_EQ(incremental.roads[i].positions, fresh.roads[i].positions);
    ASSERT_EQ(incremental.roads[i].lanes.size(), fresh.roads[i].lanes.size());
    for (std::size_t lane = 0; lane < fresh.roads[i].lanes.size(); ++lane) {
      EXPECT_EQ(incremental.roads[i].lanes[lane].indices,
                fresh.roads[i].lanes[lane].indices);
    }
  }
  const auto& a = incremental.junction_floors[0];
  const auto& b = fresh.junction_floors[0];
  EXPECT_EQ(a.mesh.positions, b.mesh.positions);
  EXPECT_EQ(a.mesh.indices, b.mesh.indices);
  ASSERT_EQ(a.details.size(), b.details.size());
  for (std::size_t i = 0; i < a.details.size(); ++i) {
    EXPECT_EQ(a.details[i].positions, b.details[i].positions);
    EXPECT_EQ(a.details[i].indices, b.details[i].indices);
  }

  constexpr int runs = 41;
  std::vector<double> samples_ms;
  samples_ms.reserve(runs);
  for (int run = 0; run < runs; ++run) {
    const auto begin = std::chrono::steady_clock::now();
    remesh_objects(f.network, incremental, f.dirty, f.options);
    const auto end = std::chrono::steady_clock::now();
    samples_ms.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
  }
  std::ranges::sort(samples_ms);
  const double p50 = samples_ms[runs / 2];
  const double p95 = samples_ms[static_cast<std::size_t>(0.95 * (runs - 1))];
  std::printf("[PrawkoRemesh] %s arms=%zu crossings=%zu raised=1: p50=%.3fms p95=%.3fms runs=%d\n",
              label, angles.size(), f.dirty.size(), p50, p95, runs);
  EXPECT_TRUE(std::isfinite(p50));
  EXPECT_TRUE(std::isfinite(p95));
  EXPECT_GE(p95, p50);
  // No hardware-independent deadline: a 16 ms target needs release GUI profiling.
}
} // namespace

TEST(PrawkoRemeshBudget, XUrbanFourCrosswalks) {
  run_case("X", {0.0, std::numbers::pi / 2.0, std::numbers::pi,
                 3.0 * std::numbers::pi / 2.0});
}
TEST(PrawkoRemeshBudget, TUrbanThreeCrosswalks) {
  run_case("T", {0.0, std::numbers::pi / 2.0, 3.0 * std::numbers::pi / 2.0});
}
TEST(PrawkoRemeshBudget, YUrbanThreeCrosswalks) {
  run_case("Y", {0.0, 2.0 * std::numbers::pi / 3.0,
                 4.0 * std::numbers::pi / 3.0});
}
