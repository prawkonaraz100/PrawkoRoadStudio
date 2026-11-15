// SPDX-License-Identifier: Apache-2.0
// Acceptance checks for authored Prawko projects, not old RoadMaker examples.
#include <gtest/gtest.h>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <algorithm>
#include <array>
#include <filesystem>
#include <string>

#include "document/document.hpp"
#include "document/esmini_preview.hpp"
#include "document/esmini_simulation.hpp"
#include "roadmaker/osc/network_validation.hpp"
#include "roadmaker/osc/reader.hpp"
#include "roadmaker/osc/writer.hpp"
#include "roadmaker/xodr/reader.hpp"

namespace roadmaker::editor {
namespace {
std::filesystem::path pdir() {
  return std::filesystem::path(RM_REPO_ROOT_DIR) / "projects/Prawko-Scenariusze";
}
const std::array<const char*,4> kScenes{
  "01-miasto-lusterka","02-przejscie-pieszy","03-deszcz-luk","04-zima-mgla"
};
}
TEST(PrawkoNewScenes, FourIndependentRoadsScenariosAndDriverViewsLoadCleanly) {
  for(const char* name:kScenes){
    SCOPED_TRACE(name);
    auto base=pdir()/name;
    base += ".xodr";
    Document document;
    auto result=document.load(base);
    ASSERT_TRUE(result.has_value()) << result.error().message;
    ASSERT_EQ(document.network().road_count(), 1U);
    ASSERT_TRUE(document.has_scenario()) << name;
    EXPECT_TRUE(document.scene_state().textured.value_or(false));
    EXPECT_EQ(document.scene_state().driver_view_actor, QStringLiteral("Ego"));
    EXPECT_FALSE(document.scene_state().time_of_day.isEmpty());
    EXPECT_FALSE(document.scene_state().weather.isEmpty());
    EXPECT_FALSE(document.scene_state().season.isEmpty());
    ASSERT_GE(document.scenario().entities.scenario_objects.size(),2U);
    const auto& entities=document.scenario().entities.scenario_objects;
    EXPECT_EQ(entities.front().name,"Ego");
    const auto findings=osc::validate_scenario_against_network(
         document.scenario(), document.network());
    for(const auto& item:findings){
      if(item.severity==Severity::Error){
        ADD_FAILURE() << name << ": " << item.message;
      }
    }
    const auto scenario_findings=osc::validate_scenario(document.scenario());
    for(const auto& item:scenario_findings){
      if(item.severity==Severity::Error) ADD_FAILURE() << item.message;
    }
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const auto xodr=std::filesystem::path(dir.filePath(QStringLiteral("accept.xodr")).toStdWString());
    const auto xosc=std::filesystem::path(dir.filePath(QStringLiteral("accept.xosc")).toStdWString());
    auto snapshot=write_esmini_snapshot(document,xodr,xosc);
    ASSERT_TRUE(snapshot.has_value()) << snapshot.error().message;
    auto parsed=osc::load_xosc(xosc);
    ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
    EXPECT_GE(parsed->scenario.entities.scenario_objects.size(),2U);
  }
}
TEST(PrawkoNewScenes, RealEsminiRecordsActualEgoMotionWhenInstalled) {
  const auto exe = std::filesystem::path(RM_REPO_ROOT_DIR) /
      "toolchain/esmini-v3.11.0/esmini/bin/esmini.exe";
  if (!std::filesystem::exists(exe)) {
    GTEST_SKIP() << "esmini is optional on CI; local real-engine proof uses official v3.11.0.";
  }
  Document document;
  ASSERT_TRUE(document.load(pdir()/"01-miasto-lusterka.xodr").has_value());
  EsminiSimulation simulator;
  QSignalSpy finished(&simulator, &EsminiSimulation::finished);
  auto started = simulator.start(document, QString::fromStdWString(exe.wstring()), 60'000);
  ASSERT_TRUE(started.has_value()) << started.error().message;
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(60'000))
      << "The real traffic engine did not complete";
  const QString error = finished.takeFirst().front().toString();
  ASSERT_TRUE(error.isEmpty()) << error.toStdString();
  ASSERT_TRUE(simulator.result().has_value());
  const auto& trace = simulator.result()->trace;
  ASSERT_GT(trace.samples.size(), 250);
  const SimulationPlayback playback(trace, simulator.result()->timestep);
  const auto start = playback.poses_at(0);
  const auto after = playback.poses_at(5);
  ASSERT_EQ(start.size(), 3);
  ASSERT_EQ(after.size(), 3);
  EXPECT_EQ(start.front().actor_name, QStringLiteral("Ego"));
  EXPECT_GT(after.front().position[0] - start.front().position[0], 40.0);
  EXPECT_GT(after.front().speed_mps, 8.0);
  EXPECT_EQ(simulator.result()->engine_version.isEmpty(), false);
}

TEST(PrawkoNewScenes, PedestrianEventAndWetCurvedRoadArePresent) {
  Document pedestrian;
  ASSERT_TRUE(pedestrian.load(pdir()/"02-przejscie-pieszy.xodr").has_value());
  ASSERT_EQ(pedestrian.scenario().entities.scenario_objects.size(),3U);
  EXPECT_GE(pedestrian.scenario().storyboard.stories.size(),1U);
  Document rain;
  ASSERT_TRUE(rain.load(pdir()/"03-deszcz-luk.xodr").has_value());
  EXPECT_EQ(rain.scene_state().weather,QStringLiteral("rain"));
  EXPECT_GE(rain.scenario().storyboard.stories.size(),1U);
  Document winter;
  ASSERT_TRUE(winter.load(pdir()/"04-zima-mgla.xodr").has_value());
  EXPECT_EQ(winter.scene_state().weather,QStringLiteral("fog"));
  EXPECT_EQ(winter.scene_state().season,QStringLiteral("winter"));
}
} // namespace roadmaker::editor
