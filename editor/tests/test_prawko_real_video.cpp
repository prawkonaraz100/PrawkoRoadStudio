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

#include "roadmaker/assets/prop_library.hpp"
#include "document/asset_import.hpp"
#include "document/document.hpp"
#include "document/library_manifest.hpp"
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
  // R6b2 opt-in integration proof loads the same project overlay as MainWindow.
  // Document::load alone intentionally does not register a project's GLB assets.
  // Keep existing generic proof behaviour untouched for every other scene.
  const bool r6b2_car_proof = base == QStringLiteral("06c-r6b2-samochody") ||
      base == QStringLiteral("06l-r6b2j-dwie-wysokosci-kaluz");
  struct OverlayCleanup {
    bool registered = false;
    ~OverlayCleanup() { if (registered) props::clear_project_models(); }
  } cleanup;
  if (r6b2_car_proof) {
    const auto project_dir = src.parent_path();
    const auto manifest = LibraryManifest::load(project_dir / "assets/library/manifest.json");
    ASSERT_TRUE(manifest.has_value()) << manifest.error().message;
    cleanup.registered = true;
    const auto diagnostics = register_project_prop_models(project_dir, *manifest);
    ASSERT_TRUE(props::model("r6b2-samochod") != nullptr)
        << "R6b2 dynamic proof requires the actual authored GLB";
    ASSERT_TRUE(diagnostics.empty())
        << "R6b2 asset catalog must import without diagnostics";
  }
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

  // R6b2l hardware regression: reuse the same real driver pose while the
  // output FBO and all three mirror FBOs shrink, grow and return to the
  // initial size. A size-only unit test cannot catch invalid GL attachment
  // lifetimes or a stale QOpenGLWidget default framebuffer after resize.
  const auto resize_poses = playback.poses_at(2.0);
  constexpr std::array<std::pair<int,int>,4> resize_cases{{
      {320,240}, {960,540}, {640,360}, {640,360}}};
  for (const auto& [target_width, target_height] : resize_cases) {
    const QSize target_size{target_width,target_height};
    const QImage proof = view.capture_simulation_frame(
        target_size,camera,resize_poses,QStringLiteral("Ego"),2.0);
    ASSERT_FALSE(proof.isNull()) << "Driver framebuffer failed after resize";
    ASSERT_EQ(proof.size(),target_size);
    const auto mirror_sizes = ViewportWidget::driver_mirror_sizes(target_size);
    const int margin = std::max(7,target_width/115);
    const std::array<QRect,3> boxes{{
        QRect((target_width-mirror_sizes[0].width())/2,margin,
              mirror_sizes[0].width(),mirror_sizes[0].height()),
        QRect(margin,target_height-mirror_sizes[1].height()-margin,
              mirror_sizes[1].width(),mirror_sizes[1].height()),
        QRect(target_width-mirror_sizes[2].width()-margin,
              target_height-mirror_sizes[2].height()-margin,
              mirror_sizes[2].width(),mirror_sizes[2].height())}};
    for (std::size_t m = 0; m < boxes.size(); ++m) {
      const QRect interior=boxes[m].adjusted(12,9,-12,-9);
      ASSERT_TRUE(interior.isValid());
      int visible_pixels=0;
      std::set<unsigned> colors;
      for (int y=interior.top(); y<interior.bottom(); y+=4) {
        for (int x=interior.left(); x<interior.right(); x+=4) {
          const QRgb p=proof.pixel(x,y);
          const int light=qRed(p)+qGreen(p)+qBlue(p);
          if (light>75) ++visible_pixels;
          colors.insert(static_cast<unsigned>(
              ((qRed(p)/20)<<16)|((qGreen(p)/20)<<8)|(qBlue(p)/20)));
        }
      }
      EXPECT_GT(visible_pixels,4) << "Black mirror FBO after resize: "<<m;
      EXPECT_GT(colors.size(),3U) << "Empty mirror FBO after resize: "<<m;
    }
  }
  std::cout<<"R6B2L_REAL_GL_FBO_RESIZE_PASS 320x240 960x540 640x360 640x360"
           <<std::endl;
  // R6b2j opt-in: identical ACTUAL esmini driver pose (not orbit-camera
  // screenshots). Compare the second elevated-water reflection ON/OFF,
  // keeping cockpit, precipitation, sunlight and vehicle movement fixed.
  if(qEnvironmentVariableIsSet("PRAWKO_R6B2J_VIDEO_COMPARE_LEVELS")) {
    const auto poses=playback.poses_at(2.0);
    qunsetenv("PRAWKO_R6B2J_PROOF_DISABLE_SECOND_LEVEL");
    const QImage two_levels=view.capture_simulation_frame(
        kSize,camera,poses,QStringLiteral("Ego"),2.0);
    qputenv("PRAWKO_R6B2J_PROOF_DISABLE_SECOND_LEVEL","1");
    const QImage one_level=view.capture_simulation_frame(
        kSize,camera,poses,QStringLiteral("Ego"),2.0);
    qunsetenv("PRAWKO_R6B2J_PROOF_DISABLE_SECOND_LEVEL");
    ASSERT_FALSE(two_levels.isNull());
    ASSERT_EQ(two_levels.size(),one_level.size());
    ASSERT_TRUE(two_levels.save(output+".dual-on.png","PNG"));
    ASSERT_TRUE(one_level.save(output+".dual-off.png","PNG"));
    int changed=0;
    for(int y=0;y<two_levels.height();y+=2) {
      for(int x=0;x<two_levels.width();x+=2) {
        const QRgb a=two_levels.pixel(x,y),b=one_level.pixel(x,y);
        const int delta=std::abs(qRed(a)-qRed(b))+
                        std::abs(qGreen(a)-qGreen(b))+
                        std::abs(qBlue(a)-qBlue(b));
        if(delta>18)++changed;
      }
    }
    std::cout<<"R6B2J_REAL_ESMINI_DRIVER_DUAL_LEVEL_AB changed_samples="
             <<changed<<" samples_at_t=2s"<<std::endl;
    EXPECT_GT(changed,10)
        << "Elevated-water reflection must show from real esmini driver pose";
  }
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
  if (night_proof && qEnvironmentVariableIsSet("PRAWKO_VIDEO_PROOF_COMPARE_TRAFFIC")) {
    // R4b3b: same esmini poses, same camera, same time: only the traffic
    // vehicle's light toggles. This avoids mistaking movement for lighting.
    const auto identical_poses = playback.poses_at(2.0);
    qunsetenv("PRAWKO_VIDEO_PROOF_NO_TRAFFIC_LIGHTS");
    const QImage traffic_on = view.capture_simulation_frame(
        kSize, camera, identical_poses, QStringLiteral("Ego"), 2.0);
    qputenv("PRAWKO_VIDEO_PROOF_NO_TRAFFIC_LIGHTS", "1");
    const QImage traffic_off = view.capture_simulation_frame(
        kSize, camera, identical_poses, QStringLiteral("Ego"), 2.0);
    qunsetenv("PRAWKO_VIDEO_PROOF_NO_TRAFFIC_LIGHTS");
    ASSERT_FALSE(traffic_on.isNull());
    ASSERT_EQ(traffic_on.size(), traffic_off.size());
    const QString on_name = output + ".traffic-on.png";
    const QString off_name = output + ".traffic-off.png";
    ASSERT_FALSE(QFileInfo::exists(on_name));
    ASSERT_FALSE(QFileInfo::exists(off_name));
    ASSERT_TRUE(traffic_on.save(on_name,"PNG"));
    ASSERT_TRUE(traffic_off.save(off_name,"PNG"));
    int brighter = 0, darker = 0, changed = 0;
    for (int y = 0; y < height; y += 2) {
      for (int x = 0; x < width; x += 2) {
        const QRgb on = traffic_on.pixel(x, y);
        const QRgb off = traffic_off.pixel(x, y);
        const int delta = (qRed(on) + qGreen(on) + qBlue(on))
                        - (qRed(off) + qGreen(off) + qBlue(off));
        if (delta > 12) ++brighter;
        if (delta < -12) ++darker;
        if (std::abs(delta) > 12) ++changed;
      }
    }
    std::cout << "R4B3B_TRAFFIC_AB_GPU_PROOF brighter=" << brighter
              << " darker=" << darker << " changed=" << changed << '\n';
    EXPECT_GT(brighter, 10) << "Other-car lights are not visibly illuminating the scene";
    EXPECT_GT(brighter, darker) << "Turning on lights should brighten, not darken";
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
