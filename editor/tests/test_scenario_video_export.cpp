// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <QColor>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "document/scenario_video_export.hpp"

namespace roadmaker::editor {
namespace {

QImage color_frame(const QSize& size, const QColor& color) {
  QImage frame(size, QImage::Format_RGBA8888);
  frame.fill(color);
  return frame;
}

QByteArray read_file(const QString& path) {
  QFile file(path);
  return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

} // namespace

TEST(ScenarioVideoExport, UsesFrameRateToSampleTheSelectedTimeRange) {
  const ScenarioVideoSettings options{.start_ms = 1950, .end_ms = 3910, .fps = 25};
  EXPECT_EQ(options.frame_count(), 49);
  EXPECT_DOUBLE_EQ(options.time_at(0), 1.95);
  EXPECT_DOUBLE_EQ(options.time_at(48), 3.87);
  const ScenarioVideoSettings short_range{.start_ms = 0, .end_ms = 1, .fps = 24};
  EXPECT_EQ(short_range.frame_count(), 1);
  const ScenarioVideoSettings reversed{.start_ms = 100, .end_ms = 50, .fps = 24};
  EXPECT_EQ(reversed.frame_count(), 0);
}

TEST(ScenarioVideoExport, RealEncoderPreservesExistingFileUntilExportSucceeds) {
  const QString encoder = ScenarioVideoExport::find_encoder();
  if (encoder.isEmpty()) {
    GTEST_SKIP() << "Put FFmpeg with libx264 on PATH to run the MP4 integration test.";
  }
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const QString path = directory.filePath(QStringLiteral("wyjazd-łódź.mp4"));
  const QByteArray previous = "previous export";
  {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write(previous), previous.size());
  }
  const ScenarioVideoSettings options{
      .start_ms = 1000, .end_ms = 2000, .fps = 24, .size = {64, 48}};
  const auto capture = [&](double time) {
    return color_frame(options.size, time < 1.5 ? Qt::red : Qt::blue);
  };
  ScenarioVideoExport exporter;
  QSignalSpy finished(&exporter, &ScenarioVideoExport::finished);
  const auto cancel = QObject::connect(
      &exporter, &ScenarioVideoExport::progress, &exporter, [&](int, int) { exporter.cancel(); });
  ASSERT_TRUE(exporter.start(encoder, path, options, capture).isEmpty());
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(10'000));
  EXPECT_TRUE(finished.takeFirst().at(0).toString().contains(QStringLiteral("cancelled")));
  EXPECT_EQ(read_file(path), previous);
  QObject::disconnect(cancel);

  int captured = 0;
  ASSERT_TRUE(exporter
                  .start(encoder,
                         path,
                         options,
                         [&](double time) { return ++captured < 3 ? capture(time) : QImage{}; })
                  .isEmpty());
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(10'000));
  EXPECT_TRUE(finished.takeFirst().at(0).toString().contains(QStringLiteral("frame 3")));
  EXPECT_EQ(read_file(path), previous);

  const QString probe = QStandardPaths::findExecutable(QStringLiteral("ffprobe"));
  if (!probe.isEmpty()) {
    ASSERT_TRUE(exporter.start(probe, path, options, capture).isEmpty());
    ASSERT_TRUE(!finished.isEmpty() || finished.wait(10'000));
    EXPECT_TRUE(finished.takeFirst().at(0).toString().contains(QStringLiteral("could not finish")));
    EXPECT_EQ(read_file(path), previous);
  }

  const auto verify_unpublished =
      QObject::connect(&exporter, &ScenarioVideoExport::progress, &exporter, [&](int, int) {
        EXPECT_EQ(read_file(path), previous);
      });
  ASSERT_TRUE(exporter.start(encoder, path, options, capture).isEmpty());
  ASSERT_TRUE(!finished.isEmpty() || finished.wait(10'000));
  const QString error = finished.takeFirst().at(0).toString();
  ASSERT_TRUE(error.isEmpty()) << error.toStdString();
  QObject::disconnect(verify_unpublished);
  EXPECT_NE(read_file(path), previous);

  QProcess decoder;
  decoder.start(
      encoder,
      {"-v", "error", "-i", path, "-f", "rawvideo", "-pix_fmt", "rgba", "-threads", "1", "pipe:1"});
  ASSERT_TRUE(decoder.waitForFinished(10'000));
  ASSERT_EQ(decoder.exitCode(), 0) << decoder.readAllStandardError().constData();
  const QByteArray rgba = decoder.readAllStandardOutput();
  const auto frame_bytes = options.size.width() * options.size.height() * 4;
  ASSERT_EQ(rgba.size(), frame_bytes * 24);
  EXPECT_GT(static_cast<unsigned char>(rgba.at(0)), 200);
  EXPECT_GT(static_cast<unsigned char>(rgba.at(23 * frame_bytes + 2)), 200);
}

} // namespace roadmaker::editor
