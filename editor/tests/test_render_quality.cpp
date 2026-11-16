// SPDX-License-Identifier: Apache-2.0
#include <gtest/gtest.h>
#include "render/render_quality.hpp"

namespace roadmaker::editor {
TEST(RenderQuality, UnchangedInstancesNeedOnlyOneUpload) {
  InstanceBufferCache cache;
  std::vector<InstanceData> instances(1000);
  EXPECT_TRUE(cache.update(instances));
  for (int pass=0;pass<20;++pass) EXPECT_FALSE(cache.update(instances));
  const auto different_allocation=instances;
  EXPECT_FALSE(cache.update(different_allocation));
}
TEST(RenderQuality, InPlaceAnimationInvalidatesTheBuffer) {
  InstanceBufferCache cache;
  std::vector<InstanceData> instances(2);
  EXPECT_TRUE(cache.update(instances));
  instances[1].model[12] = 7.0F;
  EXPECT_TRUE(cache.update(instances));
  EXPECT_FALSE(cache.update(instances));
  instances[0].model[0] = 0.5F;
  EXPECT_TRUE(cache.update(instances));
}
TEST(RenderQuality, SharedMeshWithDifferentSpansNeverReusesWrongTransforms) {
  InstanceBufferCache cache;
  std::vector<InstanceData> first(2), second(2);
  second[0].model[13] = 5.0F;
  EXPECT_TRUE(cache.update(first));
  EXPECT_TRUE(cache.update(second));
  EXPECT_TRUE(cache.update(first));
  first.pop_back();
  EXPECT_TRUE(cache.update(first));
  EXPECT_TRUE(cache.update({}));
  EXPECT_TRUE(cache.update(first));
}
TEST(RenderQuality, SubTexelCameraMotionKeepsTheShadowGridStable) {
  std::array<float,16> a{};
  a[0]=a[5]=a[10]=a[15]=1;
  auto b=a;
  b[12]=0.0003F; b[13]=-0.0003F;
  EXPECT_EQ(stabilize_shadow_projection(a,1024),stabilize_shadow_projection(b,1024));
}
TEST(RenderQuality, ShadowGridMovesInWholeTexelsAndPreservesDepth) {
  std::array<float,16> a{};
  a[0]=a[5]=a[10]=a[15]=1;
  a[12]=0.0023F; a[13]=-0.0023F; a[14]=-0.75F;
  const auto b=stabilize_shadow_projection(a,1024);
  EXPECT_FLOAT_EQ(b[12],2.0F/1024.0F);
  EXPECT_FLOAT_EQ(b[13],-2.0F/1024.0F);
  for (int i=0;i<16;++i) if(i!=12 && i!=13) EXPECT_EQ(a[i],b[i]);
  EXPECT_EQ(stabilize_shadow_projection(a,0),a);
}
} // namespace roadmaker::editor
