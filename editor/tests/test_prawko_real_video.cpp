// SPDX-License-Identifier: Apache-2.0
// Opt-in real GPU/real esmini/real FFmpeg acceptance. CI runs without this test
// when PRAWKO_VIDEO_PROOF_OUT is absent; it never modifies user documents.
#include <gtest/gtest.h>
#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QGuiApplication>
#include <QOpenGLContext>
#include <QSignalSpy>
#include <QSurfaceFormat>
#include <QTest>
#include <QWidget>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <array>
#include <algorithm>
#include <set>
#include <QRect>

#include "document/document.hpp"
#include "document/esmini_simulation.hpp"
#include "document/scenario_video_export.hpp"
#include "document/selection_model.hpp"
#include "tools/tool_manager.hpp"
#include "viewport/viewport_widget.hpp"

namespace roadmaker::editor {
TEST(PrawkoRealVideo, DriverViewStreamsActualTrafficAndMirrorsToMp4) {
  const QString output=qEnvironmentVariable("PRAWKO_VIDEO_PROOF_OUT");
  if(output.isEmpty()) GTEST_SKIP() << "Opt-in proof only; set PRAWKO_VIDEO_PROOF_OUT.";
  if(QGuiApplication::platformName() != QStringLiteral("windows")) {
    GTEST_SKIP() << "Requires a real Windows OpenGL 3.3 context";
  }
  ASSERT_FALSE(QFileInfo::exists(output)) << "Never overwrite an existing MP4";
  const QString base=qEnvironmentVariable("PRAWKO_VIDEO_PROOF_SCENE",
                                           QStringLiteral("01-miasto-lusterka"));
  const auto src=std::filesystem::path(RM_REPO_ROOT_DIR) /
      "projects/Prawko-Scenariusze" / (base.toStdString()+".xodr");
  const auto exe=std::filesystem::path(RM_REPO_ROOT_DIR) /
      "toolchain/esmini-v3.11.0/esmini/bin/esmini.exe";
  if(!std::filesystem::exists(exe))GTEST_SKIP() << "Install the official esmini binary first";
  Document document;
  auto loaded=document.load(src);
  ASSERT_TRUE(loaded.has_value()) << loaded.error().message;
  EsminiSimulation engine;
  QSignalSpy simulated(&engine,&EsminiSimulation::finished);
  auto started=engine.start(document,QString::fromStdWString(exe.wstring()),120'000);
  ASSERT_TRUE(started.has_value()) << started.error().message;
  ASSERT_TRUE(!simulated.isEmpty()||simulated.wait(120'000));
  const QString engine_error=simulated.takeFirst().first().toString();
  ASSERT_TRUE(engine_error.isEmpty()) << engine_error.toStdString();
  ASSERT_TRUE(engine.result().has_value());
  const SimulationPlayback playback(engine.result()->trace,engine.result()->timestep);
  ASSERT_GE(playback.duration(),6.0);
  EXPECT_GE(playback.poses_at(2.0).size(),3);

  QSurfaceFormat format;
  format.setVersion(3,3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setDepthBufferSize(24);
  QOpenGLContext probe;
  probe.setFormat(format);
  if(!probe.create())GTEST_SKIP() << "No OpenGL 3.3 context, don't crash the Qt renderer";
  SelectionModel selection(document);
  ToolManager tools;
  ViewportWidget view(document,selection,tools);
  view.setFormat(format);
  view.resize(1024,700);
  view.set_textured_rendering(true);
  // Developer-only opt-in: exercise actual nighttime GL beams without
  // modifying any saved scene, XODR or sidecar.
  view.set_time_of_day(qEnvironmentVariableIsSet("PRAWKO_VIDEO_PROOF_FORCE_NIGHT")
                           ? QStringLiteral("night") : document.scene_state().time_of_day);
  view.set_weather(document.scene_state().weather);
  view.set_season(document.scene_state().season);
  view.show();
  QTest::qWait(300);
  ASSERT_NE(view.context(),nullptr);
  ASSERT_TRUE(view.context()->isValid());
  // Optional HD proof settings; defaults remain deliberately small for
  // quick developer smoke tests. This is not a runtime editor preference.
  const auto opt = [](const char* key, int fallback, int lo, int hi) {
    const int requested = qEnvironmentVariableIntValue(key);
    return requested >= lo && requested <= hi ? requested : fallback;
  };
  const QSize kSize{opt("PRAWKO_VIDEO_PROOF_WIDTH",640,320,1920),
                    opt("PRAWKO_VIDEO_PROOF_HEIGHT",360,240,1080)};
  const int output_fps=opt("PRAWKO_VIDEO_PROOF_FPS",12,6,60);
  const OrbitCamera camera=view.camera();
  QImage before=view.capture_simulation_frame(kSize,camera,playback.poses_at(0.0),
                                             QStringLiteral("Ego"),0.0);
  ASSERT_FALSE(before.isNull());
  ASSERT_EQ(before.size(),kSize);
  QImage later=view.capture_simulation_frame(kSize,camera,playback.poses_at(4.0),
                                            QStringLiteral("Ego"),4.0);
  ASSERT_EQ(later.size(),kSize);
  EXPECT_NE(std::memcmp(before.constBits(),later.constBits(),before.sizeInBytes()),0)
      << "Actual esmini vehicle and mirror frames cannot be identical";
  // R4b: pin the three actual mirror FBO results, not just the main windshield.
  // The boxes match the viewport's center/left/right layout. Inspect only their
  // inner pixels, excluding rounded frames, HUD and cockpit. This detects a
  // black/empty FBO (including a broken depth-map FBO restoration) and verifies
  // that each mirror contains independently changing image content.
  const int width = kSize.width(), height = kSize.height();
  const int margin = std::max(7, width / 115);
  const int center_width = std::clamp(static_cast<int>(width * 0.35), 160, 480);
  const int side_width = std::clamp(static_cast<int>(width * 0.19), 110, 250);
  const int center_height = std::max(40, center_width / 4);
  const int side_height = std::max(58, side_width * 3 / 5);
  const std::array<QRect, 3> mirror_boxes{
      QRect((width - center_width) / 2, margin, center_width, center_height),
      QRect(margin, height - side_height - margin, side_width, side_height),
      QRect(width - side_width - margin, height - side_height - margin, side_width, side_height)};
  const std::array<const char*, 3> mirror_names{"center", "left", "right"};
  const bool night_proof = qEnvironmentVariableIsSet("PRAWKO_VIDEO_PROOF_FORCE_NIGHT");
  int total_moving_samples = 0;
  for (std::size_t i = 0; i < mirror_boxes.size(); ++i) {
    const QRect interior = mirror_boxes[i].adjusted(12, 10, -12, -10);
    std::set<unsigned> color_buckets;
    int bright_samples = 0, changed_samples = 0;
    for (int y = interior.top(); y < interior.bottom(); y += 3) {
      for (int x = interior.left(); x < interior.right(); x += 3) {
        const QRgb p = before.pixel(x, y);
        const QRgb q = later.pixel(x, y);
        if ((qRed(p) + qGreen(p) + qBlue(p)) / 3 > 40) ++bright_samples;
        color_buckets.insert(static_cast<unsigned>(
            ((qRed(p) / 20) << 16) | ((qGreen(p) / 20) << 8) | (qBlue(p) / 20)));
        if (std::abs(qRed(p) - qRed(q)) +
            std::abs(qGreen(p) - qGreen(q)) +
            std::abs(qBlue(p) - qBlue(q)) > 18) ++changed_samples;
      }
    }
    // At night the rear-facing mirrors are unlit except for road markings;
    // never demand daytime luminance from a real nighttime scene.
    EXPECT_GT(bright_samples, night_proof ? 5 : 20)
        << mirror_names[i] << " mirror is black";
    EXPECT_GE(color_buckets.size(), night_proof ? 3U : 4U)
        << mirror_names[i] << " mirror has no scene";
    EXPECT_GT(changed_samples, 0) << mirror_names[i] << " mirror is frozen";
    total_moving_samples += changed_samples;
    std::cout << "R4B_MIRROR_CAPTURE_PASS " << mirror_names[i]
              << " colors=" << color_buckets.size()
              << " bright=" << bright_samples
              << " changed=" << changed_samples << '\n';
  }
  EXPECT_GT(total_moving_samples, 5);
  if (night_proof) {
    // Pin actual illuminated asphalt ahead, not unlit lane-marking pixels.
    // This excludes most of the white markings and the dashboard. A no-lamp
    // nighttime frame remains too dark to meet the threshold.
    int illuminated = 0;
    for (int y = height * 54 / 100; y < height * 60 / 100; y += 3) {
      for (int x = width * 43 / 100; x < width * 57 / 100; x += 3) {
        const QRgb color = before.pixel(x, y);
        const int luma = (qRed(color) + qGreen(color) + qBlue(color)) / 3;
        if (luma >= 70) ++illuminated;
      }
    }
    EXPECT_GT(illuminated, 30) << "Night road should receive real beam lighting";
    std::cout << "R4B2_NIGHT_ROAD_LIGHT_PASS illuminated_samples="
              << illuminated << '\n';
  }
  ASSERT_TRUE(before.save(output+".first.png","PNG"));
  ASSERT_TRUE(later.save(output+".moving.png","PNG"));

  const QString encoder=ScenarioVideoExport::find_encoder(
      qEnvironmentVariable("PRAWKO_FFMPEG"));
  ASSERT_FALSE(encoder.isEmpty());
  const int end_ms = qEnvironmentVariableIntValue("PRAWKO_VIDEO_PROOF_END_MS")
                         ? qEnvironmentVariableIntValue("PRAWKO_VIDEO_PROOF_END_MS")
                         : 6000;
  const ScenarioVideoSettings settings{.start_ms=0,.end_ms=end_ms,
                                       .fps=output_fps,.size=kSize};
  ScenarioVideoExport exporter;
  QSignalSpy completed(&exporter,&ScenarioVideoExport::finished);
  QElapsedTimer clock;
  clock.start();
  const QString fail=exporter.start(encoder,output,settings,[&](double t) {
    return view.capture_simulation_frame(settings.size,camera,playback.poses_at(t),
                                         QStringLiteral("Ego"),t);
  });
  ASSERT_TRUE(fail.isEmpty()) << fail.toStdString();
  ASSERT_TRUE(!completed.isEmpty()||completed.wait(240'000))
      << "Real GL + FFmpeg integration timed out";
  const QString result=completed.takeFirst().first().toString();
  ASSERT_TRUE(result.isEmpty()) << result.toStdString();
  ASSERT_GT(QFileInfo(output).size(),30000);
  std::cout << "PRAWKO_REAL_MP4_PASS " << output.toStdString()
            << " frames=" << settings.frame_count()
            << " seconds_wall=" << clock.elapsed()/1000.0
            << " render_export_fps=" << settings.frame_count()*1000.0/
                 std::max<qint64>(1,clock.elapsed())
            << " bytes=" << QFileInfo(output).size() << std::endl;
  view.hide();
}
} // namespace roadmaker::editor
