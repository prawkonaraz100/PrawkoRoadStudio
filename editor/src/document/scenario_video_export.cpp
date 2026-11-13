// SPDX-License-Identifier: Apache-2.0

#include "document/scenario_video_export.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <filesystem>
#include <utility>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace roadmaker::editor {

int ScenarioVideoSettings::frame_count() const {
  if (start_ms < 0 || end_ms <= start_ms || fps <= 0 || fps > 60) {
    return 0;
  }
  const auto duration_ms = static_cast<qint64>(end_ms) - start_ms;
  return static_cast<int>((duration_ms * fps + 999) / 1000);
}

double ScenarioVideoSettings::time_at(int frame) const {
  return start_ms / 1000.0 + static_cast<double>(frame) / fps;
}

ScenarioVideoExport::ScenarioVideoExport(QObject* parent) : QObject(parent) {
  next_frame_.setSingleShot(true);
  watchdog_.setSingleShot(true);
  watchdog_.setInterval(30'000);
  process_.setStandardOutputFile(QProcess::nullDevice());
#ifdef Q_OS_WIN
  process_.setCreateProcessArgumentsModifier(
      [](QProcess::CreateProcessArguments* args) { args->flags |= CREATE_NO_WINDOW; });
#endif
  connect(&process_, &QProcess::started, this, [this] {
    if (stop_reason_.isEmpty()) {
      next_frame_.start();
    } else {
      process_.kill();
    }
  });
  connect(&process_, &QProcess::bytesWritten, this, [this](qint64) {
    if (!directory_ || !stop_reason_.isEmpty()) {
      return;
    }
    watchdog_.start();
    if (process_.bytesToWrite() == 0 && frame_ < total_) {
      next_frame_.start();
    }
  });
  connect(&process_,
          &QProcess::readyReadStandardError,
          this,
          &ScenarioVideoExport::collect_error_output);
  connect(&process_, &QProcess::finished, this, &ScenarioVideoExport::process_finished);
  connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (directory_ && error == QProcess::FailedToStart) {
      finish(stop_reason_.isEmpty() ? tr("Could not start FFmpeg. %1").arg(process_.errorString())
                                    : stop_reason_);
    }
    // Write errors and crashes can precede finished(). Preserve FFmpeg stderr there.
  });
  connect(&next_frame_, &QTimer::timeout, this, &ScenarioVideoExport::send_frame);
  connect(&watchdog_, &QTimer::timeout, this, [this] {
    stop(tr("FFmpeg stopped responding during export."));
  });
}

ScenarioVideoExport::~ScenarioVideoExport() {
  process_.disconnect(this);
  if (process_.state() != QProcess::NotRunning) {
    process_.kill();
    process_.waitForFinished(2000);
  }
}

QString ScenarioVideoExport::find_encoder(const QString& configured) {
  if (!configured.isEmpty() && QFileInfo(configured).isExecutable()) {
    return QFileInfo(configured).absoluteFilePath();
  }
  const QString encoder = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"),
                                                         {QCoreApplication::applicationDirPath()});
  return encoder.isEmpty() ? QStandardPaths::findExecutable(QStringLiteral("ffmpeg")) : encoder;
}

QString ScenarioVideoExport::start(const QString& encoder,
                                   const QString& destination,
                                   const ScenarioVideoSettings& options,
                                   Capture capture) {
  if (directory_) {
    return tr("A video export is already running.");
  }
  const QFileInfo output(destination);
  if (output.suffix().compare(QStringLiteral("mp4"), Qt::CaseInsensitive) != 0 || output.isDir()) {
    return tr("Choose a file ending in .mp4.");
  }
  const int count = options.frame_count();
  if (count == 0 || !options.size.isValid() || options.size.isEmpty() ||
      options.size.width() % 2 != 0 || options.size.height() % 2 != 0 || !capture) {
    return tr("Choose a valid time range and even video dimensions.");
  }
  if (encoder.isEmpty() || !QFileInfo(encoder).isExecutable()) {
    return tr("The FFmpeg executable could not be found.");
  }
  auto directory = std::make_unique<QTemporaryDir>(
      output.absoluteDir().filePath(QStringLiteral(".prawko-video-XXXXXX")));
  if (!directory->isValid()) {
    return tr("Could not create a temporary video file in %1.").arg(output.absolutePath());
  }
  destination_ = output.absoluteFilePath();
  staged_path_ = directory->filePath(QStringLiteral("video.mp4"));
  directory_ = std::move(directory);
  options_ = options;
  capture_ = std::move(capture);
  frame_ = 0;
  total_ = count;
  stop_reason_.clear();
  error_output_.clear();
  const QStringList arguments{
      "-hide_banner",
      "-loglevel",
      "error",
      "-nostdin",
      "-xerror",
      "-f",
      "rawvideo",
      "-pixel_format",
      "rgba",
      "-video_size",
      QStringLiteral("%1x%2").arg(options.size.width()).arg(options.size.height()),
      "-framerate",
      QString::number(options.fps),
      "-i",
      "pipe:0",
      "-an",
      "-c:v",
      "libx264",
      "-preset",
      "fast",
      "-crf",
      "18",
      "-pix_fmt",
      "yuv420p",
      "-movflags",
      "+faststart",
      "-f",
      "mp4",
      staged_path_,
  };
  watchdog_.start();
  process_.start(QFileInfo(encoder).absoluteFilePath(), arguments);
  return {};
}

void ScenarioVideoExport::send_frame() {
  if (!directory_ || !stop_reason_.isEmpty() || process_.state() != QProcess::Running ||
      process_.bytesToWrite() != 0) {
    return;
  }
  const QImage image = capture_(options_.time_at(frame_)).convertToFormat(QImage::Format_RGBA8888);
  if (image.size() != options_.size || image.isNull()) {
    stop(tr("Could not capture video frame %1 from the 3D view.").arg(frame_ + 1));
    return;
  }
  const auto* bytes = reinterpret_cast<const char*>(image.constBits());
  if (process_.write(bytes, image.sizeInBytes()) != image.sizeInBytes()) {
    stop(tr("Could not send a frame to FFmpeg."));
    return;
  }
  watchdog_.start();
  ++frame_;
  if (frame_ == total_) {
    process_.closeWriteChannel();
  } else if (process_.bytesToWrite() == 0) {
    next_frame_.start();
  }
  emit progress(frame_, total_);
}

void ScenarioVideoExport::collect_error_output() {
  constexpr qsizetype max_error_bytes = 8192;
  error_output_ = (error_output_ + process_.readAllStandardError()).right(max_error_bytes);
}

void ScenarioVideoExport::process_finished(int exit_code, QProcess::ExitStatus exit_status) {
  if (!directory_) {
    return;
  }
  collect_error_output();
  if (!stop_reason_.isEmpty()) {
    finish(stop_reason_);
    return;
  }
  if (exit_status != QProcess::NormalExit || exit_code != 0 || frame_ != total_ ||
      process_.bytesToWrite() != 0 || QFileInfo(staged_path_).size() == 0) {
    QString error = tr("FFmpeg could not finish the MP4.");
    const QString details = QString::fromLocal8Bit(error_output_).trimmed();
    if (!details.isEmpty()) {
      error += QLatin1Char('\n') + details;
    }
    finish(error);
    return;
  }
  // QFile::rename refuses an existing destination. The standard rename
  // operation replaces it in one step on the same filesystem.
  std::error_code error;
  std::filesystem::rename(std::filesystem::path(staged_path_.toStdU16String()),
                          std::filesystem::path(destination_.toStdU16String()),
                          error);
  finish(error ? tr("Could not publish the MP4 to %1.").arg(destination_) : QString{});
}

void ScenarioVideoExport::cancel() {
  stop(tr("Video export cancelled."));
}

void ScenarioVideoExport::stop(const QString& reason) {
  if (!directory_ || !stop_reason_.isEmpty()) {
    return;
  }
  stop_reason_ = reason;
  if (process_.state() == QProcess::NotRunning) {
    finish(reason);
  } else {
    process_.kill();
  }
}

void ScenarioVideoExport::finish(const QString& error) {
  const QString message = error;
  next_frame_.stop();
  watchdog_.stop();
  capture_ = {};
  directory_.reset();
  emit finished(message);
}

} // namespace roadmaker::editor
