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

// Document::push_command bridge: apply-once semantics, undo/redo through
// the kernel, failed applies never pushed, stack cleared on load.

#include "roadmaker/edit/operations.hpp"
#include "roadmaker/mesh/mesh_builder.hpp"
#include "roadmaker/road/object.hpp"

#include <gtest/gtest.h>

#include <QSignalSpy>
#include <filesystem>
#include <optional>
#include <string>

#include "document/document.hpp"

using roadmaker::RoadId;
using roadmaker::editor::Document;

namespace {

std::filesystem::path sample() {
  return std::filesystem::path(RM_SAMPLES_DIR) / "t_junction.xodr";
}

RoadId any_road(const Document& document) {
  RoadId found;
  document.network().for_each_road([&](RoadId id, const roadmaker::Road&) {
    if (!found.is_valid()) {
      found = id;
    }
  });
  return found;
}

} // namespace

TEST(DocumentCommands, PushAppliesOnceAndEnablesUndo) {
  Document document;
  ASSERT_TRUE(document.load(sample()).has_value());
  const RoadId road = any_road(document);
  const std::string original = document.network().road(road)->name;

  QSignalSpy mesh_spy(&document, &Document::mesh_changed);
  ASSERT_TRUE(
      document.push_command(roadmaker::edit::rename_road(document.network(), road, "Renamed"))
          .has_value());

  EXPECT_EQ(document.network().road(road)->name, "Renamed"); // applied exactly once
  EXPECT_EQ(mesh_spy.count(), 1);
  EXPECT_TRUE(document.undo_stack()->canUndo());
  EXPECT_FALSE(document.undo_stack()->canRedo());

  document.undo_stack()->undo();
  EXPECT_EQ(document.network().road(road)->name, original);
  document.undo_stack()->redo();
  EXPECT_EQ(document.network().road(road)->name, "Renamed");
}

TEST(DocumentCommands, FailedApplyIsNotPushedAndDiagnosed) {
  Document document;
  ASSERT_TRUE(document.load(sample()).has_value());
  const std::size_t diagnostics_before = document.diagnostics().size();

  QSignalSpy diag_spy(&document, &Document::diagnostics_changed);
  const auto result =
      document.push_command(roadmaker::edit::rename_road(document.network(), RoadId{}, "x"));

  ASSERT_FALSE(result.has_value());
  EXPECT_FALSE(document.undo_stack()->canUndo());
  EXPECT_EQ(document.diagnostics().size(), diagnostics_before + 1);
  EXPECT_EQ(diag_spy.count(), 1);
  EXPECT_FALSE(document.push_command(nullptr).has_value());
}

TEST(DocumentCommands, TopologyChangedFiresOnlyForTopologyEdits) {
  Document document;
  ASSERT_TRUE(document.load(sample()).has_value());
  const RoadId road = any_road(document);

  QSignalSpy topology_spy(&document, &Document::topology_changed);
  ASSERT_TRUE(
      document.push_command(roadmaker::edit::rename_road(document.network(), road, "Renamed"))
          .has_value());
  EXPECT_EQ(topology_spy.count(), 0);

  ASSERT_TRUE(
      document
          .push_command(roadmaker::edit::create_road({roadmaker::Waypoint{.x = 500.0, .y = 500.0},
                                                      roadmaker::Waypoint{.x = 580.0, .y = 510.0}},
                                                     roadmaker::LaneProfile::two_lane_default(),
                                                     "New Road"))
          .has_value());
  EXPECT_EQ(topology_spy.count(), 1);

  document.undo_stack()->undo(); // undoing the create is also a topology change
  EXPECT_EQ(topology_spy.count(), 2);
}

TEST(DocumentCommands, GeometryEditsRemeshIncrementallyWithRoadPayload) {
  Document document;
  ASSERT_TRUE(document.load(sample()).has_value());

  // A fresh, junction-free road is the deterministic partial-path subject.
  ASSERT_TRUE(
      document
          .push_command(roadmaker::edit::create_road({roadmaker::Waypoint{.x = 400.0, .y = 400.0},
                                                      roadmaker::Waypoint{.x = 480.0, .y = 420.0},
                                                      roadmaker::Waypoint{.x = 560.0, .y = 400.0}},
                                                     roadmaker::LaneProfile::two_lane_default(),
                                                     "Editable"))
          .has_value());
  RoadId edited;
  document.network().for_each_road([&](RoadId id, const roadmaker::Road& road) {
    if (road.name == "Editable") {
      edited = id;
    }
  });
  ASSERT_TRUE(edited.is_valid());

  // Capture payloads with a plain lambda (std::vector<RoadId> is not a
  // registered metatype, so QSignalSpy cannot record it).
  std::vector<std::vector<roadmaker::RoadId>> payloads;
  QObject::connect(
      &document,
      &Document::mesh_changed,
      &document,
      [&payloads](const std::vector<roadmaker::RoadId>& roads) { payloads.push_back(roads); });

  // Untouched roads must keep their exact vertex buffers across the edit.
  std::vector<std::pair<roadmaker::RoadId, const double*>> untouched;
  for (const roadmaker::RoadMesh& road : document.mesh().roads) {
    if (road.road != edited) {
      untouched.emplace_back(road.road, road.positions.data());
    }
  }
  ASSERT_FALSE(untouched.empty());

  ASSERT_TRUE(document
                  .push_command(roadmaker::edit::move_waypoint(
                      document.network(), edited, 1, roadmaker::Waypoint{.x = 480.0, .y = 460.0}))
                  .has_value());

  // Pure geometry edit on a junction-free road: partial payload.
  ASSERT_EQ(payloads.size(), 1U);
  EXPECT_EQ(payloads[0], (std::vector<roadmaker::RoadId>{edited}));
  for (const auto& [id, data] : untouched) {
    for (const roadmaker::RoadMesh& road : document.mesh().roads) {
      if (road.road == id) {
        EXPECT_EQ(road.positions.data(), data);
      }
    }
  }

  // Undo rides the same incremental path with the same payload.
  document.undo_stack()->undo();
  ASSERT_EQ(payloads.size(), 2U);
  EXPECT_EQ(payloads[1], (std::vector<roadmaker::RoadId>{edited}));
}

TEST(DocumentCommands, UndoStackClearsOnLoad) {
  Document document;
  ASSERT_TRUE(document.load(sample()).has_value());
  const RoadId road = any_road(document);
  ASSERT_TRUE(
      document.push_command(roadmaker::edit::rename_road(document.network(), road, "Renamed"))
          .has_value());
  ASSERT_TRUE(document.undo_stack()->canUndo());

  ASSERT_TRUE(document.load(sample()).has_value());
  EXPECT_FALSE(document.undo_stack()->canUndo());
  EXPECT_FALSE(document.undo_stack()->canRedo());
}

TEST(DocumentCommands, NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity) {
  Document document;
  ASSERT_TRUE(document.load(std::filesystem::path(RM_SAMPLES_DIR) /
                            "gs1_urban_intersection.xodr").has_value());
  roadmaker::ObjectId first_crosswalk;
  document.network().for_each_object([&](roadmaker::ObjectId id,
                                         const roadmaker::Object& object) {
    if (!first_crosswalk.is_valid() &&
        object.type == roadmaker::ObjectType::Crosswalk) {
      first_crosswalk = id;
    }
  });
  ASSERT_TRUE(first_crosswalk.is_valid());

  // Unlike the legacy flat kernel default, the editor always requests the
  // realistic 15 cm curbs. The result of an object edit and Undo/Redo must
  // match a full fresh 3-D build — including every corner-detail mesh.
  const auto expect_parity = [&]() {
    roadmaker::MeshOptions options;
    options.raised_sidewalks = true;
    options.curb_render_lod = true; // match production GUI level of detail
    const auto fresh = roadmaker::build_network_mesh(document.network(), options);
    const auto& cached = document.mesh();
    ASSERT_EQ(cached.roads.size(), fresh.roads.size());
    for (std::size_t i = 0; i < fresh.roads.size(); ++i) {
      EXPECT_EQ(cached.roads[i].positions, fresh.roads[i].positions);
      EXPECT_EQ(cached.roads[i].normals, fresh.roads[i].normals);
      ASSERT_EQ(cached.roads[i].lanes.size(), fresh.roads[i].lanes.size());
    }
    ASSERT_EQ(cached.junction_floors.size(), fresh.junction_floors.size());
    for (std::size_t i = 0; i < fresh.junction_floors.size(); ++i) {
      const auto& a = cached.junction_floors[i].details;
      const auto& b = fresh.junction_floors[i].details;
      ASSERT_EQ(a.size(), b.size());
      for (std::size_t k = 0; k < a.size(); ++k) {
        EXPECT_EQ(a[k].positions, b[k].positions);
        EXPECT_EQ(a[k].normals, b[k].normals);
        EXPECT_EQ(a[k].indices, b[k].indices);
      }
    }
  };
  expect_parity();
  QSignalSpy spy(&document, &Document::mesh_changed);
  ASSERT_TRUE(document.push_command(
      roadmaker::edit::delete_object(document.network(), first_crosswalk)).has_value());
  expect_parity();
  document.undo_stack()->undo();
  expect_parity();
  document.undo_stack()->redo();
  expect_parity();
  EXPECT_GE(spy.count(), 3);
}
