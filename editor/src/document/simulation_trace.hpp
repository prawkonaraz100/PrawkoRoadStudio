// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "roadmaker/error.hpp"

#include <QByteArray>
#include <QStringList>
#include <QVector>

namespace roadmaker::editor {

struct SimulationSample {
  double time = 0.0;
  int actor_id = 0;
  QString actor_name;
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
  double heading = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
  QString road_id;
  int lane_id = 0;
};

enum class SimulationEventKind { Started, Completed, Collision };

struct SimulationEvent {
  double time = 0.0;
  SimulationEventKind kind = SimulationEventKind::Started;
  QString name;
};

struct SimulationTrace {
  QVector<SimulationSample> samples;
  QVector<SimulationEvent> events;
  QStringList unstarted_events;
  QStringList incomplete_events;
  QStringList ambiguous_events;
};

/// Reads extended dat2csv output and esmini's storyboard transitions.
[[nodiscard]] Expected<SimulationTrace> read_simulation_trace(const QString& csv_path,
                                                              const QString& log_path,
                                                              const QStringList& expected_events);

} // namespace roadmaker::editor
