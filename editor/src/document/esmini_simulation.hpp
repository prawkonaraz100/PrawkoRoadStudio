// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>
#include <optional>

#include "document/simulation_trace.hpp"

namespace roadmaker::editor {

class Document;

struct SimulationRun {
  QString source_scene;
  QString snapshot_sha256;
  QString engine_version;
  QByteArray network_snapshot;
  QByteArray scenario_snapshot;
  double timestep = 0.05;
  int seed = 123;
  SimulationTrace trace;
};

class EsminiSimulation : public QObject {
  Q_OBJECT

public:
  explicit EsminiSimulation(QObject* parent = nullptr);
  ~EsminiSimulation() override;

  [[nodiscard]] static QString find_converter(const QString& engine);
  [[nodiscard]] Expected<void>
  start(const Document& document, const QString& engine, int timeout_ms = 60'000);
  void cancel();
  [[nodiscard]] bool running() const;

  [[nodiscard]] const std::optional<SimulationRun>& result() const { return result_; }

signals:
  void progress(const QString& message);
  void finished(const QString& error);

private:
  enum class Phase { Idle, Engine, Converter, Reading };

  void process_finished(int exit_code, QProcess::ExitStatus exit_status);
  void stop(const QString& reason);
  void finish(const QString& error = {});

  std::shared_ptr<QTemporaryDir> directory_;
  QProcess process_;
  QTimer deadline_;
  QFutureWatcher<Expected<SimulationTrace>> reader_;
  Phase phase_ = Phase::Idle;
  QString converter_;
  QStringList expected_events_;
  QString stop_reason_;
  SimulationRun pending_;
  std::optional<SimulationRun> result_;
};

} // namespace roadmaker::editor
