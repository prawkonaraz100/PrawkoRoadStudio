// SPDX-License-Identifier: Apache-2.0

#include "panels/simulation_panel.hpp"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QVBoxLayout>

#include "app/settings.hpp"
#include "document/document.hpp"
#include "document/esmini_preview.hpp"

namespace roadmaker::editor {

SimulationPanel::SimulationPanel(Document& document, Settings& settings, QWidget* parent)
    : QWidget(parent), document_(document), settings_(settings), status_(new QLabel(this)),
      details_(new QPlainTextEdit(this)), run_(new QPushButton(tr("Run Simulation"), this)),
      cancel_(new QPushButton(tr("Cancel"), this)) {
  auto* buttons = new QHBoxLayout;
  buttons->addWidget(run_);
  buttons->addWidget(cancel_);
  buttons->addStretch();
  auto* layout = new QVBoxLayout(this);
  layout->addLayout(buttons);
  status_->setTextFormat(Qt::PlainText);
  status_->setWordWrap(true);
  status_->setObjectName(QStringLiteral("simulation_status"));
  details_->setReadOnly(true);
  details_->setObjectName(QStringLiteral("simulation_details"));
  details_->setAccessibleName(tr("Simulation report"));
  layout->addWidget(status_);
  layout->addWidget(details_, 1);

  status_->setText(tr("Open a scenario with actors to simulate."));
  connect(run_, &QPushButton::clicked, this, &SimulationPanel::run);
  connect(cancel_, &QPushButton::clicked, this, &SimulationPanel::cancel);
  connect(&document_, &Document::loaded, this, &SimulationPanel::mark_scene_changed);
  connect(&document_, &Document::scenario_changed, this, &SimulationPanel::mark_scene_changed);
  connect(document_.undo_stack(), &QUndoStack::indexChanged, this, [this](int) {
    mark_scene_changed();
  });
  connect(&simulation_, &EsminiSimulation::progress, this, [this](const QString& message) {
    status_->setText(message);
    update_controls();
  });
  connect(&simulation_, &EsminiSimulation::finished, this, &SimulationPanel::show_result);
  update_controls();
}

void SimulationPanel::run() {
  if (simulation_.running()) {
    return;
  }
  if (document_.scenario().entities.scenario_objects.empty()) {
    status_->setText(tr("Place an actor before running the scenario."));
    return;
  }
  QString engine = resolve_esmini(settings_.esmini_path());
  if (engine.isEmpty() || EsminiSimulation::find_converter(engine).isEmpty()) {
    status_->setText(tr("Select esmini from a release folder that also contains dat2csv."));
    engine = QFileDialog::getOpenFileName(this, tr("Locate the esmini executable"), engine);
    if (engine.isEmpty()) {
      return;
    }
  }

  const bool scene_was_changed = scene_changed_;
  scene_changed_ = false;
  details_->clear();
  const auto started = simulation_.start(document_, engine);
  if (!started) {
    scene_changed_ = scene_was_changed;
    status_->setText(QString::fromStdString(started.error().message));
    return;
  }
  settings_.set_esmini_path(engine);
}

void SimulationPanel::cancel() {
  simulation_.cancel();
}

void SimulationPanel::update_controls() {
  run_->setEnabled(!simulation_.running() &&
                   !document_.scenario().entities.scenario_objects.empty());
  cancel_->setEnabled(simulation_.running());
}

void SimulationPanel::mark_scene_changed() {
  update_controls();
  scene_changed_ = true;
  if (simulation_.result()) {
    status_->setText(tr("This result is from an earlier version of the scene."));
  } else if (!simulation_.running()) {
    status_->setText(run_->isEnabled() ? tr("Ready to run the scenario.")
                                       : tr("Place an actor before running the scenario."));
  }
}

void SimulationPanel::show_result(const QString& error) {
  update_controls();
  if (!error.isEmpty()) {
    status_->setText(error.section('\n', 0, 0));
    details_->setPlainText(error.section('\n', 1));
    return;
  }
  const SimulationRun& run = *simulation_.result();
  const SimulationTrace& trace = run.trace;
  const bool missing = !trace.unstarted_events.isEmpty() || !trace.incomplete_events.isEmpty() ||
                       !trace.ambiguous_events.isEmpty();
  const QString outcome =
      missing ? tr("Simulation ended with unverified events.") : tr("Simulation finished.");
  status_->setText(scene_changed_ ? outcome + tr(" The scene has changed since it started.")
                                  : outcome);

  QSet<int> actors;
  for (const auto& sample : trace.samples) {
    actors.insert(sample.actor_id);
  }
  QStringList report{
      tr("Scene: %1").arg(run.source_scene.isEmpty() ? tr("Unsaved scene") : run.source_scene),
      tr("%1 s · %2 actors · %3 samples")
          .arg(trace.samples.back().time, 0, 'f', 2)
          .arg(actors.size())
          .arg(trace.samples.size()),
      tr("esmini %1 · step %2 s · seed %3")
          .arg(run.engine_version.isEmpty() ? tr("(version unavailable)") : run.engine_version)
          .arg(run.timestep)
          .arg(run.seed),
      tr("Snapshot SHA-256: %1").arg(run.snapshot_sha256),
  };
  if (!trace.unstarted_events.isEmpty()) {
    report << tr("Events that did not start: %1").arg(trace.unstarted_events.join(", "));
  }
  if (!trace.incomplete_events.isEmpty()) {
    report << tr("Events that did not complete: %1").arg(trace.incomplete_events.join(", "));
  }
  if (!trace.ambiguous_events.isEmpty()) {
    report << tr("Events with ambiguous names: %1").arg(trace.ambiguous_events.join(", "));
  }
  for (const auto& event : trace.events) {
    QString label;
    switch (event.kind) {
    case SimulationEventKind::Started:
      label = tr("Started");
      break;
    case SimulationEventKind::Completed:
      label = tr("Completed");
      break;
    case SimulationEventKind::Collision:
      label = tr("Collision");
      break;
    }
    report << tr("%1 s · %2 · %3").arg(event.time, 0, 'f', 2).arg(label, event.name);
  }
  details_->setPlainText(report.join('\n'));
}

} // namespace roadmaker::editor
