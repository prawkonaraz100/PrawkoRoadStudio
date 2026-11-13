// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QImage>
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <functional>
#include <memory>

namespace roadmaker::editor {

struct ScenarioVideoSettings {
  int start_ms = 0;
  int end_ms = 0;
  int fps = 30;
  QSize size{1280, 720};

  [[nodiscard]] int frame_count() const;
  [[nodiscard]] double time_at(int frame) const;
};

/// Streams clean frames to FFmpeg. The destination is replaced only after
/// the encoder has closed a complete MP4 in the destination directory.
class ScenarioVideoExport : public QObject {
  Q_OBJECT

public:
  using Capture = std::function<QImage(double)>;

  explicit ScenarioVideoExport(QObject* parent = nullptr);
  ~ScenarioVideoExport() override;

  [[nodiscard]] static QString find_encoder(const QString& configured = {});
  /// Returns an error before launch, or an empty string and a later finished signal.
  [[nodiscard]] QString start(const QString& encoder,
                              const QString& destination,
                              const ScenarioVideoSettings& options,
                              Capture capture);
  void cancel();

signals:
  void progress(int frames, int total);
  void finished(const QString& error);

private:
  void send_frame();
  void process_finished(int exit_code, QProcess::ExitStatus exit_status);
  void stop(const QString& reason);
  void finish(const QString& error);
  void collect_error_output();

  QProcess process_;
  QTimer next_frame_;
  QTimer watchdog_;
  std::unique_ptr<QTemporaryDir> directory_;
  QString destination_;
  QString staged_path_;
  QByteArray error_output_;
  QString stop_reason_;
  ScenarioVideoSettings options_;
  Capture capture_;
  int frame_ = 0;
  int total_ = 0;
};

} // namespace roadmaker::editor
