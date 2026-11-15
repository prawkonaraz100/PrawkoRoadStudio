/*
 * Copyright 2026 PrawkoRoadStudio
 * Licensed under the Apache License, Version 2.0.
 */

#include "render/cockpit.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace roadmaker::editor {
namespace {

TEST(CockpitBlenderAssets, ThreeAssembliesAreRealTriangleMeshes) {
  const auto meshes = load_cockpit_meshes();
  ASSERT_TRUE(meshes.has_value()) << "Three bundled original Blender GLBs must import in Qt";
  ASSERT_TRUE(meshes->valid());
  const auto verify = [](const std::vector<RenderMeshData>& parts) {
    std::size_t tris = 0;
    for (const RenderMeshData& part: parts) {
      EXPECT_EQ(part.kind, PrimitiveKind::Triangles);
      EXPECT_EQ(part.positions.size() % 3, 0U);
      EXPECT_EQ(part.indices.size() % 3, 0U);
      tris += part.indices.size() / 3;
    }
    return tris;
  };
  EXPECT_GT(verify(meshes->shell), 500U);
  EXPECT_GT(verify(meshes->wheel), 100U);
  EXPECT_GT(verify(meshes->wiper), 20U);
  // Imported model is anchored back in Blender's camera-relative frame,
  // not left floating with the GLB reader's normalised base at Z=0.
  float min_z = 100;
  float max_z = -100;
  for (const auto& part : meshes->shell) {
    for (std::size_t j = 2; j < part.positions.size(); j += 3) {
      min_z = std::min(min_z, part.positions[j]);
      max_z = std::max(max_z, part.positions[j]);
    }
  }
  float min_x=100, max_x=-100, min_y=100, max_y=-100;
  for (const auto& part : meshes->shell) {
    for (std::size_t j = 0; j + 2 < part.positions.size(); j += 3) {
      min_x=std::min(min_x,part.positions[j]); max_x=std::max(max_x,part.positions[j]);
      min_y=std::min(min_y,part.positions[j+1]); max_y=std::max(max_y,part.positions[j+1]);
    }
  }
  EXPECT_GT(max_x - min_x, 2.2F);
  EXPECT_GT(min_y, 0.30F) << "Cabin must not intersect the camera eye";
  EXPECT_GT(max_y, 2.0F);
  EXPECT_LT(min_z, -0.9F);
  EXPECT_GT(max_z, 0.3F);
}

TEST(CockpitPose, RealMeshesStayAnchoredToDriverAtArbitraryVehicleRoll) {
  DriverCamera driver{
      .eye={10.0F, 22.0F, 1.7F},
      .forward={0.0F,1.0F,0.0F},
      .right={0.0F,0.0F,-1.0F},
      .up={-1.0F,0.0F,0.0F},
  };
  const auto m = cockpit_pose(driver, {0.0F,0.67F,-0.31F}).model;
  EXPECT_FLOAT_EQ(m[0], driver.right[0]);
  EXPECT_FLOAT_EQ(m[1], driver.right[1]);
  EXPECT_FLOAT_EQ(m[2], driver.right[2]);
  EXPECT_FLOAT_EQ(m[4], driver.forward[0]);
  EXPECT_FLOAT_EQ(m[5], driver.forward[1]);
  EXPECT_FLOAT_EQ(m[6], driver.forward[2]);
  EXPECT_NEAR(m[12], 10.31F, 1e-5F);
  EXPECT_NEAR(m[13], 22.67F, 1e-5F);
  EXPECT_NEAR(m[14], 1.7F, 1e-5F);
  const auto rotated = cockpit_pose(driver, {}, std::numbers::pi_v<float>/2.0F);
  EXPECT_NEAR(rotated.model[0], driver.up[0], 1e-5F);
  EXPECT_NEAR(rotated.model[2], driver.up[2], 1e-5F);
}

TEST(CockpitWipers, SweepIsRepeatableAndStoppedOnDryRoad) {
  EXPECT_FLOAT_EQ(cockpit_wiper_sweep(0.0, true), 0.0F);
  EXPECT_FLOAT_EQ(cockpit_wiper_sweep(1.0, false), 0.0F);
  EXPECT_GT(cockpit_wiper_sweep(0.55, true), 0.0F);
  const float first = cockpit_wiper_sweep(0.55, true);
  EXPECT_FLOAT_EQ(first, cockpit_wiper_sweep(0.55, true));
  EXPECT_GE(first, 0.0F);
  EXPECT_LE(first, 1.08F);
}

} // namespace
} // namespace roadmaker::editor
