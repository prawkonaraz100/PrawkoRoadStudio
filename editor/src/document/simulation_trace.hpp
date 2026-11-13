// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "roadmaker/error.hpp"

#include <QByteArray>
#include <QStringList>
#include <QVector>

#include <array>

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

struct SimulationActorPose {
  QString actor_name;
  std::array<double, 3> position{};
  double heading = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
};

/// Time-indexed actor transforms shared by playback and frame capture.
class SimulationPlayback {
public:
  SimulationPlayback(const SimulationTrace& trace, double timestep);

  [[nodiscard]] double duration() const { return duration_; }
  [[nodiscard]] QVector<SimulationActorPose> poses_at(double time) const;

private:
  struct Keyframe {
    double time;
    std::array<double, 3> position;
    double heading;
    double pitch;
    double roll;
  };
  struct Track {
    QString name;
    QVector<Keyframe> frames;
  };

  QVector<Track> tracks_;
  double duration_ = 0.0;
  double timestep_;
};

/// Reads extended dat2csv output and esmini's storyboard transitions.
[[nodiscard]] Expected<SimulationTrace> read_simulation_trace(const QString& csv_path,
                                                              const QString& log_path,
                                                              const QStringList& expected_events);

} // namespace roadmaker::editor
