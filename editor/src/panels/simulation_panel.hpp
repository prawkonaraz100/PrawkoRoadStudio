// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QWidget>

#include "document/esmini_simulation.hpp"

class QLabel;
class QPlainTextEdit;
class QPushButton;

namespace roadmaker::editor {

class Document;
class Settings;

class SimulationPanel : public QWidget {
  Q_OBJECT

public:
  SimulationPanel(Document& document, Settings& settings, QWidget* parent = nullptr);

  void run();
  void cancel();

  [[nodiscard]] const std::optional<SimulationRun>& result() const { return simulation_.result(); }

private:
  void update_controls();
  void mark_scene_changed();
  void show_result(const QString& error);

  Document& document_;
  Settings& settings_;
  EsminiSimulation simulation_;
  QLabel* status_;
  QPlainTextEdit* details_;
  QPushButton* run_;
  QPushButton* cancel_;
  bool scene_changed_ = false;
};

} // namespace roadmaker::editor
