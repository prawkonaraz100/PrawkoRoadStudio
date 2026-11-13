// SPDX-License-Identifier: Apache-2.0

#include "document/simulation_trace.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <array>
#include <cmath>

namespace roadmaker::editor {
namespace {

constexpr auto kMaxTraceBytes = 64 * 1024 * 1024;
constexpr auto kMaxLogBytes = 8 * 1024 * 1024;
constexpr auto kMaxLineBytes = 64 * 1024;

Expected<QVector<SimulationSample>> read_samples(const QString& path) {
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly) || file.size() > kMaxTraceBytes) {
    return make_error(ErrorCode::IoFailure, "Cannot read the simulation CSV or it is too large.");
  }
  const QStringList header = QString::fromUtf8(file.readLine(kMaxLineBytes)).trimmed().split(',');
  constexpr std::array<const char*, 11> columns{
      "time", "id", "name", "x", "y", "z", "h", "p", "r", "roadId", "laneId"};
  const auto invalid_csv = [] {
    return make_error(ErrorCode::InvalidDocument, "Unexpected esmini telemetry CSV format.");
  };
  if (header.size() < static_cast<qsizetype>(columns.size())) {
    return invalid_csv();
  }
  // dat2csv writes actor names without quoting. Preserve commas in a name by
  // reading time and ID from the front and the numeric fields from the back.
  for (std::size_t i = 0; i < columns.size(); ++i) {
    if (header.at(static_cast<qsizetype>(i)).trimmed() != QLatin1String(columns[i])) {
      return invalid_csv();
    }
  }

  QVector<SimulationSample> samples;
  while (!file.atEnd()) {
    const QByteArray line = file.readLine(kMaxLineBytes);
    if (line.size() >= kMaxLineBytes - 1 || line.isEmpty()) {
      return invalid_csv();
    }
    const QStringList fields = QString::fromUtf8(line).trimmed().split(',');
    const qsizetype extra = fields.size() - header.size();
    if (extra < 0) {
      return invalid_csv();
    }
    bool valid = true;
    const auto number = [&](qsizetype index) {
      bool ok = false;
      const double value = fields.at(index).toDouble(&ok);
      valid &= ok && std::isfinite(value);
      return value;
    };
    SimulationSample sample;
    sample.time = number(0);
    bool id_ok = false;
    sample.actor_id = fields.at(1).toInt(&id_ok);
    sample.actor_name = fields.mid(2, 1 + extra).join(',').trimmed();
    sample.x = number(3 + extra);
    sample.y = number(4 + extra);
    sample.z = number(5 + extra);
    sample.heading = number(6 + extra);
    sample.pitch = number(7 + extra);
    sample.roll = number(8 + extra);
    sample.road_id = fields.at(9 + extra).trimmed();
    bool lane_ok = false;
    sample.lane_id = fields.at(10 + extra).toInt(&lane_ok);
    if (!valid || !id_ok || !lane_ok || sample.actor_name.isEmpty() ||
        (!samples.isEmpty() && sample.time < samples.back().time)) {
      return invalid_csv();
    }
    samples.push_back(std::move(sample));
  }
  if (samples.isEmpty()) {
    return make_error(ErrorCode::InvalidDocument, "The simulation produced no actor samples.");
  }
  return samples;
}

} // namespace

Expected<SimulationTrace> read_simulation_trace(const QString& csv_path,
                                                const QString& log_path,
                                                const QStringList& expected_events) {
  auto samples = read_samples(csv_path);
  if (!samples) {
    return tl::unexpected<Error>(samples.error());
  }
  QFile log(log_path);
  if (!log.open(QIODevice::ReadOnly) || log.size() > kMaxLogBytes) {
    return make_error(ErrorCode::IoFailure, "Cannot read the esmini event log or it is too large.");
  }

  SimulationTrace trace;
  trace.samples = std::move(*samples);
  QSet<QString> expected;
  QSet<QString> ambiguous;
  for (const QString& name : expected_events) {
    if (expected.contains(name) || name.contains('\n') || name.contains('\r')) {
      ambiguous.insert(name);
    }
    expected.insert(name);
  }

  const QRegularExpression timestamp(
      QStringLiteral(R"(^\[(-?\d+(?:\.\d+)?)\] \[(?:info|warn)\] (.*)$)"));
  const QString start = QStringLiteral(" standbyState -> startTransition -> runningState");
  const QString end = QStringLiteral(" runningState -> endTransition -> completeState");
  const QString collision = QStringLiteral("Collision between ");
  QSet<QString> started;
  QSet<QString> active;
  while (!log.atEnd()) {
    const QByteArray line = log.readLine(kMaxLineBytes);
    if (line.size() >= kMaxLineBytes - 1 || line.isEmpty()) {
      return make_error(ErrorCode::InvalidDocument, "Invalid esmini event log line.");
    }
    const auto match = timestamp.match(QString::fromUtf8(line).trimmed());
    if (!match.hasMatch()) {
      continue;
    }
    const double time = match.capturedView(1).toDouble();
    const QString message = match.captured(2);
    if (message.startsWith(collision) && !message.endsWith(QStringLiteral(" dissolved"))) {
      trace.events.push_back(
          {time, SimulationEventKind::Collision, message.sliced(collision.size())});
    } else if (message.endsWith(start)) {
      const QString name = message.chopped(start.size());
      if (expected.contains(name) && !ambiguous.contains(name)) {
        started.insert(name);
        active.insert(name);
        trace.events.push_back({time, SimulationEventKind::Started, name});
      }
    } else if (message.endsWith(end)) {
      const QString name = message.chopped(end.size());
      if (active.remove(name)) {
        trace.events.push_back({time, SimulationEventKind::Completed, name});
      }
    }
  }
  for (const QString& name : expected_events) {
    if (ambiguous.contains(name)) {
      if (!trace.ambiguous_events.contains(name)) {
        trace.ambiguous_events.push_back(name);
      }
    } else if (!started.contains(name)) {
      trace.unstarted_events.push_back(name);
    } else if (active.contains(name)) {
      trace.incomplete_events.push_back(name);
    }
  }
  return trace;
}

} // namespace roadmaker::editor
