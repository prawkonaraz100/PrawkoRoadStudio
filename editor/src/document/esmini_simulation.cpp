// SPDX-License-Identifier: Apache-2.0

#include "document/esmini_simulation.hpp"

#include "roadmaker/osc/network_validation.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtConcurrentRun>

#include "document/document.hpp"
#include "document/esmini_preview.hpp"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace roadmaker::editor {
namespace {

constexpr auto kRoad = "preview.xodr";
constexpr auto kScenario = "preview.xosc";
constexpr auto kRecording = "run.dat";
constexpr auto kTelemetry = "run.csv";
constexpr auto kEngineLog = "esmini.log";
constexpr auto kConverterLog = "dat2csv.log";
constexpr auto kMaxLogBytes = 8 * 1024 * 1024;
constexpr auto kMaxRecordingBytes = 64 * 1024 * 1024;
constexpr auto kMaxTelemetryBytes = 64 * 1024 * 1024;

QByteArray read_snapshot(const QString& path) {
  QFile input(path);
  return input.open(QIODevice::ReadOnly) ? input.readAll() : QByteArray{};
}

QStringList event_names(const osc::Scenario& scenario) {
  QStringList names;
  for (const auto& story : scenario.storyboard.stories) {
    for (const auto& act : story.acts) {
      for (const auto& group : act.maneuver_groups) {
        for (const auto& maneuver : group.maneuvers) {
          for (const auto& event : maneuver.events) {
            names.push_back(QString::fromStdString(event.name));
          }
        }
      }
    }
  }
  return names;
}

QString error_tail(const QString& path) {
  QFile file(path);
  constexpr qint64 limit = 4096;
  if (!file.open(QIODevice::ReadOnly) || !file.seek(qMax(0LL, file.size() - limit))) {
    return {};
  }
  return QString::fromUtf8(file.read(limit)).trimmed();
}

} // namespace

EsminiSimulation::EsminiSimulation(QObject* parent) : QObject(parent) {
  deadline_.setSingleShot(true);
  process_.setProcessChannelMode(QProcess::MergedChannels);
#ifdef Q_OS_WIN
  process_.setCreateProcessArgumentsModifier(
      [](QProcess::CreateProcessArguments* args) { args->flags |= CREATE_NO_WINDOW; });
#endif
  connect(&process_, &QProcess::finished, this, &EsminiSimulation::process_finished);
  connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) {
      finish(stop_reason_.isEmpty()
                 ? tr("Could not start %1. %2")
                       .arg(QFileInfo(process_.program()).fileName(), process_.errorString())
                 : stop_reason_);
    }
  });
  connect(&deadline_, &QTimer::timeout, this, [this] {
    stop(tr("Simulation exceeded the time limit."));
  });
  connect(&reader_, &QFutureWatcher<Expected<SimulationTrace>>::finished, this, [this] {
    if (!stop_reason_.isEmpty()) {
      finish(stop_reason_);
      return;
    }
    auto trace = reader_.future().takeResult();
    if (!trace) {
      finish(QString::fromStdString(trace.error().message));
      return;
    }
    pending_.trace = std::move(*trace);
    result_ = std::move(pending_);
    finish();
  });
}

EsminiSimulation::~EsminiSimulation() {
  process_.disconnect(this);
  if (process_.state() != QProcess::NotRunning) {
    process_.kill();
  }
}

QString EsminiSimulation::find_converter(const QString& engine) {
  return QStandardPaths::findExecutable(QStringLiteral("dat2csv"),
                                        {QFileInfo(engine).absolutePath()});
}

bool EsminiSimulation::running() const {
  return phase_ != Phase::Idle;
}

Expected<void>
EsminiSimulation::start(const Document& document, const QString& engine, int timeout_ms) {
  if (running()) {
    return make_error(ErrorCode::InvalidArgument, "A simulation is already running.");
  }
  if (document.preview_active()) {
    return make_error(ErrorCode::InvalidArgument, "Finish the current edit before running.");
  }
  if (document.scenario().entities.scenario_objects.empty()) {
    return make_error(ErrorCode::InvalidDocument, "Place an actor before running the scenario.");
  }
  if (engine.isEmpty() || !QFileInfo(engine).isExecutable()) {
    return make_error(ErrorCode::FileNotFound, "The esmini executable could not be found.");
  }
  const QString converter = find_converter(engine);
  if (converter.isEmpty()) {
    return make_error(ErrorCode::FileNotFound,
                      "Place dat2csv from the same esmini release beside esmini.");
  }
  for (const auto& finding :
       osc::validate_scenario_against_network(document.scenario(), document.network())) {
    if (finding.severity == Severity::Error) {
      return make_error(ErrorCode::InvalidDocument, finding.message);
    }
  }

  auto directory = std::make_shared<QTemporaryDir>();
  if (!directory->isValid()) {
    return make_error(ErrorCode::IoFailure, "Could not create the temporary simulation folder.");
  }
  const QDir work(directory->path());
  const QString network = work.filePath(QLatin1String(kRoad));
  const QString scenario = work.filePath(QLatin1String(kScenario));
  if (auto written = write_esmini_snapshot(document,
                                           std::filesystem::path(network.toStdU16String()),
                                           std::filesystem::path(scenario.toStdU16String()));
      !written) {
    return written;
  }
  QByteArray network_snapshot = read_snapshot(network);
  QByteArray scenario_snapshot = read_snapshot(scenario);
  if (network_snapshot.isEmpty() || scenario_snapshot.isEmpty()) {
    return make_error(ErrorCode::IoFailure, "Could not read the simulation snapshot.");
  }
  QCryptographicHash hash(QCryptographicHash::Sha256);
  hash.addData(network_snapshot);
  hash.addData(QByteArrayView("\0", 1));
  hash.addData(scenario_snapshot);

  pending_ = SimulationRun{
      .source_scene = document.file_path(),
      .snapshot_sha256 = QString::fromLatin1(hash.result().toHex()),
      .engine_version = {},
      .network_snapshot = std::move(network_snapshot),
      .scenario_snapshot = std::move(scenario_snapshot),
      .trace = {},
  };
  expected_events_ = event_names(document.scenario());
  result_.reset();
  stop_reason_.clear();
  directory_ = std::move(directory);
  converter_ = converter;
  phase_ = Phase::Engine;
  process_.setWorkingDirectory(work.absolutePath());
  process_.setStandardOutputFile(work.filePath(QLatin1String(kEngineLog)));
  const QStringList arguments{
      QStringLiteral("--headless"),
      QStringLiteral("--osc"),
      QLatin1String(kScenario),
      QStringLiteral("--fixed_timestep"),
      QString::number(pending_.timestep),
      QStringLiteral("--seed"),
      QString::number(pending_.seed),
      QStringLiteral("--collision"),
      QStringLiteral("--record"),
      QLatin1String(kRecording),
      QStringLiteral("--disable_log"),
      QStringLiteral("--log_level"),
      QStringLiteral("info"),
  };
  deadline_.start(timeout_ms);
  process_.start(QFileInfo(engine).absoluteFilePath(), arguments);
  if (running()) {
    emit progress(tr("Running the scenario in the background…"));
  }
  return {};
}

void EsminiSimulation::cancel() {
  stop(tr("Simulation cancelled."));
}

void EsminiSimulation::stop(const QString& reason) {
  if (!running() || !stop_reason_.isEmpty()) {
    return;
  }
  stop_reason_ = reason;
  if (process_.state() != QProcess::NotRunning) {
    process_.kill();
  }
}

void EsminiSimulation::process_finished(int exit_code, QProcess::ExitStatus exit_status) {
  if (!stop_reason_.isEmpty()) {
    finish(stop_reason_);
    return;
  }
  const QDir work(directory_->path());
  const QString log_path = work.filePath(phase_ == Phase::Engine ? QLatin1String(kEngineLog)
                                                                 : QLatin1String(kConverterLog));
  if (exit_status != QProcess::NormalExit || exit_code != 0) {
    finish(tr("%1 failed (exit code %2).\n%3")
               .arg(QFileInfo(process_.program()).fileName())
               .arg(exit_code)
               .arg(error_tail(log_path))
               .trimmed());
    return;
  }
  if (phase_ == Phase::Engine) {
    QFile log(log_path);
    if (!log.open(QIODevice::ReadOnly) || log.size() > kMaxLogBytes) {
      finish(tr("Could not read the esmini log or it exceeded the size limit."));
      return;
    }
    const QString output = QString::fromUtf8(log.readAll());
    const auto error = QRegularExpression(QStringLiteral(R"(^.*\[(?:error|critical)\].*$)"),
                                          QRegularExpression::MultilineOption)
                           .match(output);
    if (error.hasMatch()) {
      finish(tr("esmini reported an error.\n%1").arg(error.captured(0)));
      return;
    }
    const auto version =
        QRegularExpression(QStringLiteral(R"(esmini GIT TAG: ([^\r\n]+))")).match(output);
    pending_.engine_version = version.captured(1).trimmed();
    const QFileInfo recording(work.filePath(QLatin1String(kRecording)));
    if (recording.size() <= 0 || recording.size() > kMaxRecordingBytes) {
      finish(tr("esmini produced no recording or the recording exceeded the size limit."));
      return;
    }
    phase_ = Phase::Converter;
    emit progress(tr("Converting the actor recording…"));
    process_.setStandardOutputFile(work.filePath(QLatin1String(kConverterLog)));
    process_.start(converter_,
                   {QStringLiteral("--file"),
                    QLatin1String(kRecording),
                    QStringLiteral("--csv"),
                    QLatin1String(kTelemetry),
                    QStringLiteral("--extended"),
                    QStringLiteral("--precision"),
                    QStringLiteral("9")});
    return;
  }

  const qint64 telemetry_size = QFileInfo(work.filePath(QLatin1String(kTelemetry))).size();
  if (telemetry_size <= 0 || telemetry_size > kMaxTelemetryBytes) {
    finish(tr("dat2csv produced no telemetry or exceeded the size limit."));
    return;
  }
  phase_ = Phase::Reading;
  emit progress(tr("Reading actor samples and scenario events…"));
  reader_.setFuture(QtConcurrent::run([directory = directory_, names = expected_events_] {
    const QDir work_dir(directory->path());
    return read_simulation_trace(work_dir.filePath(QLatin1String(kTelemetry)),
                                 work_dir.filePath(QLatin1String(kEngineLog)),
                                 names);
  }));
}

void EsminiSimulation::finish(const QString& error) {
  deadline_.stop();
  phase_ = Phase::Idle;
  directory_.reset();
  pending_ = {};
  emit finished(error);
}

} // namespace roadmaker::editor
