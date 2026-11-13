// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

#include "document/esmini_simulation.hpp"

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QSlider;

namespace roadmaker::editor {

class Document;
class Settings;

class SimulationPanel : public QWidget {
  Q_OBJECT

public:
  SimulationPanel(Document& document, Settings& settings, QWidget* parent = nullptr);

  void run();
  void cancel();
  void leave_preview();

  [[nodiscard]] const std::optional<SimulationRun>& result() const { return simulation_.result(); }

signals:
  void preview_frame(const QVector<SimulationActorPose>& poses);
  void preview_closed();

private:
  void update_controls();
  void mark_scene_changed();
  void show_result(const QString& error);
  void toggle_playback();
  void pause();
  void seek(int milliseconds);
  void present_frame();
  void reset_playback();

  Document& document_;
  Settings& settings_;
  EsminiSimulation simulation_;
  QLabel* status_;
  QPlainTextEdit* details_;
  QPushButton* run_;
  QPushButton* cancel_;
  QPushButton* play_;
  QPushButton* restart_;
  QPushButton* edit_;
  QSlider* timeline_;
  QLabel* time_label_;
  QComboBox* events_;
  std::optional<SimulationPlayback> playback_;
  QTimer timer_;
  QElapsedTimer clock_;
  int play_from_ms_ = 0;
  int time_ms_ = 0;
  bool preview_active_ = false;
  bool scene_changed_ = false;
};

} // namespace roadmaker::editor
