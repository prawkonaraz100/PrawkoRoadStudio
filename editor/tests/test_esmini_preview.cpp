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

// Preparing an esmini preview (p8-s5, issue #249, GW-6 step 14).
//
// ★ WHAT IS TESTED IS THE POLICY, NEVER THE LAUNCH. `prepare_preview` is a pure
// function of the document and a directory, so these run without esmini
// installed — which is the whole reason the launch was split out of it. The
// subprocess itself is one QProcess call with nothing to assert that a test
// could reach.
//
// The property that matters most is the LOGIC FILE REWRITE: a preview that
// pointed at the scene's own `.xodr` would resolve the last SAVE rather than
// what is on screen, and would resolve nothing at all for a scene that has
// never been saved.

#include "roadmaker/edit/operations.hpp"
#include "roadmaker/osc/catalog.hpp"
#include "roadmaker/osc/edit.hpp"
#include "roadmaker/osc/reader.hpp"
#include "roadmaker/osc/writer.hpp"
#include "roadmaker/road/network.hpp"
#include "roadmaker/road/road.hpp"
#include "roadmaker/xodr/reader.hpp"

#include <gtest/gtest.h>

#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTemporaryDir>
#include <QTest>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <string>

#include "app/settings.hpp"
#include "document/document.hpp"
#include "document/esmini_preview.hpp"
#include "document/esmini_simulation.hpp"
#include "panels/simulation_panel.hpp"

namespace roadmaker::editor {
namespace {

/// The first road's OpenDRIVE `@id`.
///
/// NOT `find_road("main")`: `create_road`'s third argument is the road's NAME,
/// and `find_road` looks a road up by its `@id`, which the kernel assigns. The
/// two are different strings, and reading the wrong one hands back an invalid
/// handle whose `road()` is a null pointer.
std::string first_road_odr_id(const Document& document) {
  std::string odr_id;
  document.network().for_each_road([&odr_id](RoadId, const Road& road) {
    if (odr_id.empty()) {
      odr_id = road.odr_id;
    }
  });
  return odr_id;
}

/// A straight road with an actor on it — the smallest scene worth previewing.
void author_scene(Document& document) {
  ASSERT_TRUE(document
                  .push_command(edit::create_road(
                      {Waypoint{.x = 0.0, .y = 0.0}, Waypoint{.x = 100.0, .y = 0.0}},
                      LaneProfile::two_lane_default(),
                      "main"))
                  .has_value());

  osc::LanePosition position;
  position.road_id = first_road_odr_id(document);
  ASSERT_FALSE(position.road_id.empty());
  position.lane_id = "-1";
  position.s = 20.0;
  ASSERT_TRUE(document
                  .push_scenario_command(osc::edit::place_scenario_object(
                      document.scenario(), osc::make_actor(osc::ActorKind::Car, "Ego"), position))
                  .has_value());
}

void write_test_file(const QString& path, const QByteArray& text) {
  QFile file(path);
  ASSERT_TRUE(file.open(QIODevice::WriteOnly));
  ASSERT_EQ(file.write(text), text.size());
}

osc::Trigger after_time(double seconds) {
  osc::Condition condition;
  condition.name = "after_" + std::to_string(seconds);
  condition.simulation_time.emplace();
  condition.simulation_time->value = seconds;
  osc::ConditionGroup group;
  group.conditions.push_back(std::move(condition));
  osc::Trigger trigger;
  trigger.condition_groups.push_back(std::move(group));
  return trigger;
}

void author_simulation(Document& document, double event_time, double stop_time) {
  author_scene(document);
  osc::SpeedAction speed;
  speed.absolute_target.emplace();
  speed.absolute_target->value = 8.0;
  osc::PrivateAction change;
  change.longitudinal.emplace();
  change.longitudinal->speed = speed;
  osc::Action action;
  action.name = "accelerate";
  action.action = change;
  osc::Event event;
  event.name = "start_moving";
  event.actions.push_back(action);
  event.start_trigger = after_time(event_time);
  osc::StoryManeuver maneuver;
  maneuver.name = "move";
  maneuver.events.push_back(event);
  osc::EntityRef actor;
  actor.entity_ref = "Ego";
  osc::ManeuverGroup group;
  group.name = "ego_group";
  group.actors.push_back(actor);
  group.maneuvers.push_back(maneuver);
  osc::Act act;
  act.name = "act";
  act.maneuver_groups.push_back(group);
  osc::Story story;
  story.name = "story";
  story.acts.push_back(act);
  ASSERT_TRUE(document.push_scenario_command(osc::edit::set_story(document.scenario(), 0, story)));
  ASSERT_TRUE(document.push_scenario_command(
      osc::edit::set_stop_trigger(document.scenario(), after_time(stop_time))));
}

} // namespace

TEST(SimulationTrace, ReadsFullPoseAndSeparatesStartedCompletedAndStoppedEvents) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString csv = directory.filePath(QStringLiteral("run.csv"));
  const QString log = directory.filePath(QStringLiteral("esmini.log"));
  write_test_file(csv,
                  "time, id, name, x, y, z, h, p, r, roadId, laneId\n"
                  "0, 7, Car, 0, -1, 0, 0, 0, 0, 1, -2\n"
                  "0.25, 7, Car, 1, -1, 2, 0.1, 0.2, 0.3, 1, -1\n"
                  "0.25, 9, Car, red, 2, -2, 0, 0, 0, 0, 1, -2\n");
  write_test_file(log,
                  "[0.050] [info] First standbyState -> startTransition -> runningState\n"
                  "[0.100] [info] First runningState -> endTransition -> completeState\n"
                  "[0.100] [info] Second standbyState -> startTransition -> runningState\n"
                  "[0.150] [info] Third standbyState -> stopTransition -> completeState\n"
                  "[0.200] [warn] Collision between Car and Car, red\n"
                  "[0.250] [warn] Collision between Car and Car, red dissolved\n");
  const auto trace = read_simulation_trace(
      csv,
      log,
      {QStringLiteral("First"), QStringLiteral("Second"), QStringLiteral("Third"),
       QStringLiteral("Duplicate"), QStringLiteral("Duplicate")});
  ASSERT_TRUE(trace.has_value());
  ASSERT_EQ(trace->samples.size(), 3);
  EXPECT_DOUBLE_EQ(trace->samples.at(1).roll, 0.3);
  EXPECT_DOUBLE_EQ(trace->samples.at(1).pitch, 0.2);
  EXPECT_EQ(trace->samples.at(1).lane_id, -1);
  EXPECT_EQ(trace->samples.back().actor_name, QStringLiteral("Car, red"));
  ASSERT_EQ(trace->events.size(), 4);
  EXPECT_EQ(trace->events.at(1).kind, SimulationEventKind::Completed);
  EXPECT_EQ(trace->events.back().kind, SimulationEventKind::Collision);
  EXPECT_EQ(trace->unstarted_events, QStringList{QStringLiteral("Third")});
  EXPECT_EQ(trace->incomplete_events, QStringList{QStringLiteral("Second")});
  EXPECT_EQ(trace->ambiguous_events, QStringList{QStringLiteral("Duplicate")});
}

TEST(SimulationTrace, RejectsMissingOrMalformedTelemetry) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString csv = directory.filePath(QStringLiteral("run.csv"));
  const QString log = directory.filePath(QStringLiteral("esmini.log"));
  write_test_file(log, "engine output\n");
  EXPECT_FALSE(read_simulation_trace(csv, log, {}));
  write_test_file(csv,
                  "time, id, name, x, y, z, h, p, r, roadId, laneId\n"
                  "0, 7, Car, nan, -1, 0, 0, 0, 0, 1, -2\n");
  EXPECT_FALSE(read_simulation_trace(csv, log, {}));
}

TEST(SimulationPlayback, InterpolatesOnlyBetweenSamplesOfTheSameActor) {
  const auto sample = [](int id, double time, double x, double heading) {
    return SimulationSample{.time = time,
                            .actor_id = id,
                            .actor_name = QString::number(id),
                            .x = x,
                            .y = x / 2,
                            .z = x / 4,
                            .heading = heading};
  };
  SimulationTrace trace;
  trace.samples = {sample(7, 0.0, 0.0, 6.2), sample(7, 0.05, 4.0, 0.2),
                   sample(9, 0.05, 30.0, 1.0), sample(9, 0.1, 32.0, 1.2),
                   sample(7, 0.2, 40.0, 0.3)};
  const SimulationPlayback playback(trace, 0.05);
  ASSERT_DOUBLE_EQ(playback.duration(), 0.2);
  const auto middle = playback.poses_at(0.025);
  ASSERT_EQ(middle.size(), 1);
  EXPECT_EQ(middle.front().actor_name, QStringLiteral("7"));
  EXPECT_DOUBLE_EQ(middle.front().position[0], 2.0);
  EXPECT_DOUBLE_EQ(middle.front().position[1], 1.0);
  EXPECT_DOUBLE_EQ(middle.front().position[2], 0.5);
  EXPECT_NEAR(middle.front().heading, std::numbers::pi + 3.2, 1e-12);
  const auto exact = playback.poses_at(0.05);
  ASSERT_EQ(exact.size(), 2);
  EXPECT_DOUBLE_EQ(exact.front().heading, 0.2);
  EXPECT_TRUE(playback.poses_at(0.18).isEmpty());
  const auto end = playback.poses_at(10.0);
  ASSERT_EQ(end.size(), 1);
  EXPECT_DOUBLE_EQ(end.front().position[0], 40.0);
}

TEST(EsminiSimulation, RequiresAnActorAndTheEngine) {
  Document document;
  EsminiSimulation simulation;
  auto result = simulation.start(document, {});
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code, ErrorCode::InvalidDocument);
  author_scene(document);
  result = simulation.start(document, {});
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().code, ErrorCode::FileNotFound);
  EXPECT_FALSE(simulation.running());
}

TEST(EsminiSimulation, RealEngineDistinguishesCompletedAndUnstartedEvents) {
  const QString engine = resolve_esmini({});
  if (engine.isEmpty() || EsminiSimulation::find_converter(engine).isEmpty()) {
    GTEST_SKIP() << "Set ESMINI_PATH to test the real esmini release with dat2csv.";
  }
  EsminiSimulation simulation;
  QSignalSpy finished(&simulation, &EsminiSimulation::finished);
  QString first_snapshot;
  for (const double event_time : {0.1, 5.0}) {
    Document document;
    author_simulation(document, event_time, 1.0);
    const auto started = simulation.start(document, engine);
    ASSERT_TRUE(started.has_value()) << (started ? "" : started.error().message);
    EXPECT_TRUE(simulation.running());
    EXPECT_FALSE(simulation.start(document, engine));
    ASSERT_TRUE(!finished.isEmpty() || finished.wait(15'000));
    ASSERT_TRUE(finished.takeFirst().at(0).toString().isEmpty());
    ASSERT_TRUE(simulation.result());
    const SimulationRun& run = *simulation.result();
    EXPECT_EQ(run.snapshot_sha256.size(), 64);
    EXPECT_TRUE(run.network_snapshot.contains("<OpenDRIVE>"));
    EXPECT_TRUE(run.scenario_snapshot.contains("preview.xodr"));
    EXPECT_EQ(run.seed, 123);
    EXPECT_TRUE(document.is_dirty());
    ASSERT_GE(run.trace.samples.size(), 2);
    if (event_time < 1.0) {
      EXPECT_GT(run.trace.samples.back().x, run.trace.samples.front().x + 1.0);
      EXPECT_TRUE(run.trace.unstarted_events.isEmpty());
      EXPECT_TRUE(run.trace.incomplete_events.isEmpty());
      ASSERT_EQ(run.trace.events.size(), 2);
      EXPECT_EQ(run.trace.events.back().kind, SimulationEventKind::Completed);
      first_snapshot = run.snapshot_sha256;
    } else {
      EXPECT_DOUBLE_EQ(run.trace.samples.front().x, run.trace.samples.back().x);
      EXPECT_EQ(run.trace.unstarted_events, QStringList{QStringLiteral("start_moving")});
      EXPECT_NE(run.snapshot_sha256, first_snapshot);
    }
  }
}

TEST(EsminiSimulation, RealEngineCanBeCancelledAndRunAgain) {
  const QString engine = resolve_esmini({});
  if (engine.isEmpty() || EsminiSimulation::find_converter(engine).isEmpty()) {
    GTEST_SKIP() << "Set ESMINI_PATH to test the real esmini release with dat2csv.";
  }
  Document document;
  author_simulation(document, 0.1, 1.0);
  EsminiSimulation simulation;
  QSignalSpy finished(&simulation, &EsminiSimulation::finished);
  ASSERT_TRUE(simulation.start(document, engine));
  simulation.cancel();
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(15'000));
  EXPECT_TRUE(finished.takeFirst().at(0).toString().contains(QStringLiteral("cancelled")));
  EXPECT_FALSE(simulation.running());
  EXPECT_FALSE(simulation.result());
  ASSERT_TRUE(simulation.start(document, engine));
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(15'000));
  EXPECT_TRUE(finished.takeFirst().at(0).toString().isEmpty());
  EXPECT_TRUE(simulation.result());
}

TEST(SimulationPanel, PlaysAndSeeksRealTelemetryThenInvalidatesAfterAMapEdit) {
  const QString engine = resolve_esmini({});
  if (engine.isEmpty() || EsminiSimulation::find_converter(engine).isEmpty()) {
    GTEST_SKIP() << "Set ESMINI_PATH to test the real esmini release with dat2csv.";
  }
  const Settings::ReadOnlySession read_only;
  Settings settings;
  Document document;
  author_simulation(document, 0.1, 2.0);
  const int index_before = document.undo_stack()->index();
  SimulationPanel panel(document, settings);
  panel.show();
  QSignalSpy frames(&panel, &SimulationPanel::preview_frame);
  QSignalSpy closed(&panel, &SimulationPanel::preview_closed);
  QVector<SimulationActorPose> poses;
  QObject::connect(&panel, &SimulationPanel::preview_frame, &panel,
                   [&poses](const auto& frame) { poses = frame; });
  panel.run();
  ASSERT_TRUE(!frames.isEmpty() || frames.wait(15'000));
  ASSERT_TRUE(panel.result());
  auto* timeline = panel.findChild<QSlider*>(QStringLiteral("simulation_timeline"));
  auto* events = panel.findChild<QComboBox*>(QStringLiteral("simulation_events"));
  auto* play = panel.findChild<QPushButton*>(QStringLiteral("simulation_play"));
  auto* restart = panel.findChild<QPushButton*>(QStringLiteral("simulation_restart"));
  auto* video = panel.findChild<QPushButton*>(QStringLiteral("simulation_export_video"));
  ASSERT_NE(timeline, nullptr);
  ASSERT_NE(events, nullptr);
  ASSERT_NE(play, nullptr);
  ASSERT_NE(restart, nullptr);
  ASSERT_NE(video, nullptr);
  EXPECT_TRUE(video->isEnabled());
  ASSERT_GE(events->count(), 3);

  // Activate a marker as the widget would, then compare that frame to CSV.
  const double event_time = panel.result()->trace.events.front().time;
  events->activated(1);
  EXPECT_EQ(timeline->value(), events->itemData(1).toInt());
  const auto& samples = panel.result()->trace.samples;
  const auto sample = std::ranges::find_if(samples, [event_time](const auto& keyframe) {
    return std::abs(keyframe.time - event_time) < 1e-6;
  });
  ASSERT_NE(sample, samples.end());
  ASSERT_EQ(poses.size(), 1);
  EXPECT_NEAR(poses.front().position[0], sample->x, 1e-6);
  EXPECT_NEAR(poses.front().position[1], sample->y, 1e-6);
  EXPECT_NEAR(poses.front().position[2], sample->z, 1e-6);

  play->click();
  const auto emitted = frames.size();
  ASSERT_TRUE(frames.wait(1000));
  EXPECT_GT(frames.size(), emitted);
  play->click();
  const int paused_at = timeline->value();
  QTest::qWait(50);
  EXPECT_EQ(timeline->value(), paused_at);
  timeline->setValue(timeline->maximum());
  ASSERT_EQ(poses.size(), 1);
  EXPECT_DOUBLE_EQ(poses.front().position[0], samples.back().x);
  restart->click();
  EXPECT_EQ(timeline->value(), 0);
  ASSERT_EQ(poses.size(), 1);
  EXPECT_DOUBLE_EQ(poses.front().position[0], samples.front().x);
  EXPECT_EQ(document.undo_stack()->index(), index_before);

  ASSERT_TRUE(document.begin_preview(edit::create_road(
      {Waypoint{.x = 0.0, .y = 30.0}, Waypoint{.x = 100.0, .y = 30.0}},
      LaneProfile::two_lane_default(), "other")));
  EXPECT_EQ(closed.size(), 1);
  EXPECT_FALSE(timeline->isEnabled());
  EXPECT_FALSE(play->isEnabled());
  EXPECT_FALSE(events->isEnabled());
  EXPECT_FALSE(video->isEnabled());
  EXPECT_TRUE(panel.result());
  document.cancel_preview();
}

TEST(EsminiPreview, APreviewWritesThePairAndPointsTheScenarioAtIt) {
  Document document;
  author_scene(document);

  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto preview =
      prepare_preview(document, std::filesystem::path(directory.path().toStdString()));
  ASSERT_TRUE(preview.has_value()) << (preview ? "" : preview.error().message);

  EXPECT_TRUE(std::filesystem::exists(preview->network_path));
  EXPECT_TRUE(std::filesystem::exists(preview->scenario_path));
  EXPECT_EQ(preview->network_path.parent_path(), preview->scenario_path.parent_path())
      << "<LogicFile> is relative — the pair must travel together";

  // ★ THE REWRITE. The exported scenario points at the exported network by
  // FILENAME, not at wherever the scene's own .xodr lives (which, for a scene
  // that was never saved, is nowhere).
  const auto reloaded = osc::load_xosc(preview->scenario_path);
  ASSERT_TRUE(reloaded.has_value());
  ASSERT_TRUE(reloaded->scenario.road_network.logic_file.has_value());
  EXPECT_EQ(reloaded->scenario.road_network.logic_file->filepath,
            preview->network_path.filename().string());

  // And the network really is the one on screen.
  const auto network = roadmaker::load_xodr(preview->network_path);
  ASSERT_TRUE(network.has_value());
  bool saw_road = false;
  network->network.for_each_road([&saw_road](RoadId, const Road&) { saw_road = true; });
  EXPECT_TRUE(saw_road);
}

TEST(EsminiPreview, ThePreviewIsOfWhatIsOnScreenNotOfTheLastSave) {
  Document document;
  author_scene(document);

  QTemporaryDir first;
  const auto before = prepare_preview(document, std::filesystem::path(first.path().toStdString()));
  ASSERT_TRUE(before.has_value());

  // An edit that is NOT saved anywhere.
  ASSERT_TRUE(
      document
          .push_scenario_command(osc::edit::set_entity_init_speed(document.scenario(), "Ego", 22.0))
          .has_value());

  QTemporaryDir second;
  const auto after = prepare_preview(document, std::filesystem::path(second.path().toStdString()));
  ASSERT_TRUE(after.has_value());

  const auto reloaded = osc::load_xosc(after->scenario_path);
  ASSERT_TRUE(reloaded.has_value());
  const auto written = osc::write_xosc(reloaded->scenario);
  ASSERT_TRUE(written.has_value());
  EXPECT_NE(written->find("value=\"22\""), std::string::npos)
      << "the unsaved speed edit did not reach the preview";
}

TEST(EsminiPreview, ASceneWithNoScenarioIsRefusedRatherThanPreviewedEmpty) {
  // An empty `.xosc` loads fine in esmini and shows nothing, which looks like a
  // broken preview rather than an empty one.
  Document document;
  ASSERT_TRUE(document
                  .push_command(edit::create_road(
                      {Waypoint{.x = 0.0, .y = 0.0}, Waypoint{.x = 100.0, .y = 0.0}},
                      LaneProfile::two_lane_default(),
                      "main"))
                  .has_value());

  QTemporaryDir directory;
  const auto preview =
      prepare_preview(document, std::filesystem::path(directory.path().toStdString()));
  EXPECT_FALSE(preview.has_value());
}

TEST(EsminiPreview, TheArgumentsAskForAWindowAndAFixedTimestep) {
  // NOT --headless: this is the user watching their scenario, unlike the CI
  // smoke gate, which is the one place --headless belongs.
  Document document;
  author_scene(document);

  QTemporaryDir directory;
  const auto preview =
      prepare_preview(document, std::filesystem::path(directory.path().toStdString()));
  ASSERT_TRUE(preview.has_value());

  EXPECT_TRUE(preview->arguments.contains(QStringLiteral("--osc")));
  EXPECT_TRUE(preview->arguments.contains(QString::fromStdString(preview->scenario_path.string())));
  EXPECT_TRUE(preview->arguments.contains(QStringLiteral("--fixed_timestep")));
  EXPECT_FALSE(preview->arguments.contains(QStringLiteral("--headless")));
}

TEST(EsminiPreview, ResolutionPrefersTheSettingThenTheEnvironmentThenPath) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());

  // A file that exists but is not executable must NOT resolve — otherwise the
  // launch fails later with a message about the process rather than about the
  // path the user chose. An extensionless file is non-executable on Windows by
  // construction, and non-executable on POSIX because nothing chmods it, so
  // this half needs no platform split.
  const QString plain = directory.filePath(QStringLiteral("not-executable"));
  {
    QFile file(plain);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("x");
  }
  EXPECT_NE(resolve_esmini(plain), plain);

  // ★ THE STAND-IN NEEDS A .bat ON WINDOWS, and the reason is a real difference
  // rather than test scaffolding: `QFileInfo::isExecutable()` is a PERMISSION
  // BIT on POSIX and a FILE EXTENSION on Windows (PATHEXT — exe/com/bat/cmd).
  // An extensionless file with the +x bit set therefore resolves on macOS and
  // Linux and never on Windows, which is what this test caught. The product is
  // unaffected — a real `esmini.exe` satisfies the Windows rule — so the fix
  // belongs in the fixture, not in `resolve_esmini`.
#ifdef _WIN32
  const QString name = QStringLiteral("fake-esmini.bat");
  const char* body = "@echo off\n";
#else
  const QString name = QStringLiteral("fake-esmini");
  const char* body = "#!/bin/sh\n";
#endif
  const QString runnable = directory.filePath(name);
  {
    QFile file(runnable);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(body);
    file.close();
    // A no-op on Windows, where the extension above is what counts.
    ASSERT_TRUE(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
  }
  ASSERT_TRUE(QFileInfo(runnable).isExecutable())
      << "the stand-in is not executable by this platform's rule, so the rest of this "
         "test would assert nothing";
  EXPECT_EQ(resolve_esmini(runnable), runnable);

  // With nothing configured, the environment wins over PATH.
  qputenv("ESMINI_PATH", runnable.toUtf8());
  EXPECT_EQ(resolve_esmini(QString()), runnable);
  qunsetenv("ESMINI_PATH");
}

TEST(EsminiPreview, LaunchingWithNoBinaryIsAnErrorRatherThanASilentNoOp) {
  Document document;
  author_scene(document);
  QTemporaryDir directory;
  const auto preview =
      prepare_preview(document, std::filesystem::path(directory.path().toStdString()));
  ASSERT_TRUE(preview.has_value());

  const auto started = launch_preview(QString(), *preview);
  ASSERT_FALSE(started.has_value());
  EXPECT_NE(started.error().message.find("esmini"), std::string::npos);
}

} // namespace roadmaker::editor
