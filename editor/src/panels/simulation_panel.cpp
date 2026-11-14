// SPDX-License-Identifier: Apache-2.0

#include "panels/simulation_panel.hpp"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyleOptionSlider>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "app/settings.hpp"
#include "document/document.hpp"
#include "document/esmini_preview.hpp"

namespace roadmaker::editor {
namespace {

int timeline_ms(double seconds) {
  return static_cast<int>(std::lround(
      std::clamp(seconds * 1000.0, 0.0, static_cast<double>(std::numeric_limits<int>::max()))));
}

class TimelineSlider : public QSlider {
public:
  explicit TimelineSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {
    setContentsMargins(0, 0, 0, 6);
  }

  void set_markers(QVector<int> markers) {
    markers_ = std::move(markers);
    update();
  }

protected:
  void mousePressEvent(QMouseEvent* event) override {
    QStyleOptionSlider option;
    initStyleOption(&option);
    const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option,
                                                 QStyle::SC_SliderHandle, this);
    if (event->button() != Qt::LeftButton || handle.contains(event->pos())) {
      QSlider::mousePressEvent(event);
      return;
    }
    const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option,
                                                 QStyle::SC_SliderGroove, this);
    const int x = event->pos().x() - groove.x() - handle.width() / 2;
    const int span = std::max(1, groove.width() - handle.width());
    setValue(QStyle::sliderValueFromPosition(minimum(), maximum(), x, span, option.upsideDown));
    event->accept();
  }

  void paintEvent(QPaintEvent* event) override {
    QSlider::paintEvent(event);
    if (!isEnabled() || markers_.isEmpty()) {
      return;
    }
    QStyleOptionSlider option;
    initStyleOption(&option);
    const QRect groove = style()->subControlRect(QStyle::CC_Slider, &option,
                                                QStyle::SC_SliderGroove, this);
    const QRect handle = style()->subControlRect(QStyle::CC_Slider, &option,
                                                QStyle::SC_SliderHandle, this);
    QPainter painter(this);
    painter.setPen(palette().color(QPalette::Text));
    for (const int marker : markers_) {
      const int x = groove.x() + handle.width() / 2 +
                    QStyle::sliderPositionFromValue(minimum(), maximum(), marker,
                                                    groove.width() - handle.width(),
                                                    option.upsideDown);
      painter.drawLine(x, height() - 5, x, height() - 1);
    }
  }

private:
  QVector<int> markers_;
};

QString event_label(const SimulationEvent& event) {
  QString label;
  switch (event.kind) {
  case SimulationEventKind::Started:
    label = SimulationPanel::tr("Started");
    break;
  case SimulationEventKind::Completed:
    label = SimulationPanel::tr("Completed");
    break;
  case SimulationEventKind::Collision:
    label = SimulationPanel::tr("Collision");
    break;
  }
  return SimulationPanel::tr("%1 s · %2 · %3").arg(event.time, 0, 'f', 2).arg(label, event.name);
}

} // namespace

SimulationPanel::SimulationPanel(Document& document, Settings& settings, QWidget* parent)
    : QWidget(parent), document_(document), settings_(settings), status_(new QLabel(this)),
      details_(new QPlainTextEdit(this)), run_(new QPushButton(tr("Run Simulation"), this)),
      cancel_(new QPushButton(tr("Cancel"), this)), play_(new QPushButton(tr("&Play"), this)),
      restart_(new QPushButton(tr("From Start"), this)),
      edit_(new QPushButton(tr("Wróć do edycji"), this)),
      export_video_(new QPushButton(tr("Export MP4…"), this)),
      timeline_(new TimelineSlider(this)), time_label_(new QLabel(this)),
      events_(new QComboBox(this)), camera_(new QComboBox(this)) {
  auto* buttons = new QHBoxLayout;
  buttons->addWidget(run_);
  buttons->addWidget(cancel_);
  buttons->addStretch();
  auto* camera_label = new QLabel(tr("&Camera"), this);
  camera_label->setBuddy(camera_);
  buttons->addWidget(camera_label);
  buttons->addWidget(camera_);
  buttons->addWidget(export_video_);
  auto* layout = new QVBoxLayout(this);
  layout->addLayout(buttons);
  auto* transport = new QHBoxLayout;
  transport->addWidget(play_);
  transport->addWidget(restart_);
  transport->addWidget(timeline_, 1);
  transport->addWidget(time_label_);
  transport->addWidget(edit_);
  status_->setTextFormat(Qt::PlainText);
  status_->setWordWrap(true);
  status_->setObjectName(QStringLiteral("simulation_status"));
  details_->setReadOnly(true);
  details_->setObjectName(QStringLiteral("simulation_details"));
  details_->setAccessibleName(tr("Simulation report"));
  layout->addWidget(status_);
  layout->addLayout(transport);
  layout->addWidget(events_);
  layout->addWidget(details_, 1);

  play_->setObjectName(QStringLiteral("simulation_play"));
  restart_->setObjectName(QStringLiteral("simulation_restart"));
  edit_->setObjectName(QStringLiteral("simulation_edit"));
  export_video_->setObjectName(QStringLiteral("simulation_export_video"));
  export_video_->setEnabled(false);
  timeline_->setObjectName(QStringLiteral("simulation_timeline"));
  timeline_->setAccessibleName(tr("Simulation time"));
  timeline_->setSingleStep(50);
  timeline_->setPageStep(1000);
  time_label_->setObjectName(QStringLiteral("simulation_time"));
  events_->setObjectName(QStringLiteral("simulation_events"));
  events_->setAccessibleName(tr("Jump to simulation event"));
  events_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  events_->setMinimumContentsLength(22);
  camera_->setObjectName(QStringLiteral("simulation_camera"));
  camera_->setAccessibleName(tr("Simulation camera"));
  camera_->setToolTip(tr("Choose the view used for playback and video export"));
  camera_->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
  camera_->setMinimumContentsLength(18);
  timer_.setInterval(16);
  timer_.setTimerType(Qt::PreciseTimer);

  status_->setText(tr("Open a scenario with actors to simulate."));
  connect(run_, &QPushButton::clicked, this, [this] { run(); });
  connect(cancel_, &QPushButton::clicked, this, &SimulationPanel::cancel);
  connect(play_, &QPushButton::clicked, this, &SimulationPanel::toggle_playback);
  connect(restart_, &QPushButton::clicked, this, [this] { seek(0); });
  connect(edit_, &QPushButton::clicked, this, &SimulationPanel::leave_preview);
  connect(export_video_, &QPushButton::clicked, this, &SimulationPanel::video_export_requested);
  connect(timeline_, &QSlider::valueChanged, this, &SimulationPanel::seek);
  connect(timeline_, &QSlider::sliderPressed, this, [this] { seek(timeline_->value()); });
  connect(events_, &QComboBox::activated, this, [this](int index) {
    if (index > 0) {
      seek(events_->itemData(index).toInt());
      events_->setCurrentIndex(0);
    }
  });
  connect(camera_, &QComboBox::currentIndexChanged, this, [this](int) {
    driver_actor_ = camera_->currentData().toString();
    document_.undo_stack()->resetClean();
    if (playback_) {
      pause();
      present_frame();
    }
  });
  connect(camera_, &QComboBox::activated, this, [this](int) {
    if (playback_ && !preview_active_) {
      present_frame();
    }
  });
  connect(&timer_, &QTimer::timeout, this, [this] {
    time_ms_ = static_cast<int>(std::min<qint64>(play_from_ms_ + clock_.elapsed(),
                                                 timeline_->maximum()));
    if (time_ms_ == timeline_->maximum()) {
      pause();
    }
    present_frame();
  });
  connect(&document_, &Document::loaded, this, [this] {
    driver_actor_ = document_.scene_state().driver_view_actor;
    mark_scene_changed();
    refresh_camera_choices();
  });
  connect(&document_, &Document::mesh_changed, this, &SimulationPanel::mark_scene_changed);
  connect(&document_, &Document::scenario_changed, this, [this] {
    mark_scene_changed();
    refresh_camera_choices();
  });
  connect(document_.undo_stack(), &QUndoStack::indexChanged, this, [this](int) {
    mark_scene_changed();
  });
  connect(&simulation_, &EsminiSimulation::progress, this, [this](const QString& message) {
    status_->setText(message);
    update_controls();
  });
  connect(&simulation_, &EsminiSimulation::finished, this, &SimulationPanel::show_result);
  reset_playback();
  driver_actor_ = document_.scene_state().driver_view_actor;
  refresh_camera_choices();
  update_controls();
}

void SimulationPanel::run(bool play_when_ready) {
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
  const auto started = simulation_.start(document_, engine);
  if (!started) {
    scene_changed_ = scene_was_changed;
    status_->setText(QString::fromStdString(started.error().message));
    return;
  }
  play_when_ready_ = play_when_ready;
  reset_playback();
  details_->clear();
  update_controls();
  settings_.set_esmini_path(engine);
}

void SimulationPanel::cancel() {
  simulation_.cancel();
}

bool SimulationPanel::can_run() const {
  return !simulation_.running() && !document_.scenario().entities.scenario_objects.empty();
}

void SimulationPanel::update_controls() {
  const bool available_to_run = can_run();
  if (run_->isEnabledTo(this) != available_to_run) {
    run_->setEnabled(available_to_run);
    emit run_availability_changed(available_to_run);
  }
  cancel_->setEnabled(simulation_.running());
  const bool available = playback_.has_value();
  play_->setEnabled(available && timeline_->maximum() > 0);
  restart_->setEnabled(available);
  edit_->setEnabled(preview_active_);
  timeline_->setEnabled(available && timeline_->maximum() > 0);
  events_->setEnabled(available && events_->count() > 1);
  camera_->setEnabled(camera_->count() > 1 && !simulation_.running());
  const bool video_available = available && timeline_->maximum() > 0;
  if (export_video_->isEnabledTo(this) != video_available) {
    export_video_->setEnabled(video_available);
    emit video_availability_changed(video_available);
  }
}

void SimulationPanel::mark_scene_changed() {
  if (playback_) {
    reset_playback();
  }
  scene_changed_ = true;
  update_controls();
  if (simulation_.result()) {
    status_->setText(tr("This result is from an earlier version of the scene."));
  } else if (!simulation_.running()) {
    status_->setText(run_->isEnabled() ? tr("Ready to run the scenario.")
                                       : tr("Place an actor before running the scenario."));
  }
}

void SimulationPanel::toggle_playback() {
  if (timer_.isActive()) {
    pause();
    return;
  }
  if (!playback_ || timeline_->maximum() == 0) {
    return;
  }
  if (time_ms_ == timeline_->maximum()) {
    time_ms_ = 0;
  }
  play_from_ms_ = time_ms_;
  clock_.start();
  timer_.start();
  play_->setText(tr("&Pause"));
  present_frame();
}

void SimulationPanel::pause() {
  timer_.stop();
  play_->setText(tr("&Play"));
}

void SimulationPanel::seek(int milliseconds) {
  if (playback_) {
    pause();
    time_ms_ = std::clamp(milliseconds, 0, timeline_->maximum());
    present_frame();
  }
}

void SimulationPanel::present_frame() {
  const QSignalBlocker block(timeline_);
  timeline_->setValue(time_ms_);
  const double time = time_ms_ == timeline_->maximum() ? playback_->duration() : time_ms_ / 1000.0;
  time_label_->setText(tr("%1 / %2 s").arg(time, 0, 'f', 2).arg(playback_->duration(), 0, 'f', 2));
  preview_active_ = true;
  update_controls();
  emit preview_frame(playback_->poses_at(time), driver_actor_);
}

void SimulationPanel::refresh_camera_choices() {
  const QSignalBlocker block(camera_);
  camera_->clear();
  camera_->addItem(tr("Editor camera"), QString{});
  for (const auto& object : document_.scenario().entities.scenario_objects) {
    if (std::holds_alternative<osc::Vehicle>(object.entity_object)) {
      const QString name = QString::fromStdString(object.name);
      camera_->addItem(tr("Driver view — %1").arg(name), name);
    }
  }
  const int index = camera_->findData(driver_actor_);
  if (index < 0) {
    driver_actor_.clear();
  }
  camera_->setCurrentIndex(std::max(0, index));
  update_controls();
}

void SimulationPanel::leave_preview() {
  pause();
  if (preview_active_) {
    preview_active_ = false;
    update_controls();
    emit preview_closed();
  }
}

void SimulationPanel::reset_playback() {
  leave_preview();
  playback_.reset();
  time_ms_ = 0;
  const QSignalBlocker block(timeline_);
  timeline_->setRange(0, 0);
  time_label_->setText(tr("0.00 / 0.00 s"));
  static_cast<TimelineSlider*>(timeline_)->set_markers({});
  events_->clear();
  events_->addItem(tr("Jump to event…"));
}

void SimulationPanel::show_result(const QString& error) {
  const bool autoplay = play_when_ready_;
  play_when_ready_ = false;
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
  QVector<int> markers;
  for (const auto& event : trace.events) {
    const QString label = event_label(event);
    report << label;
    if (!scene_changed_) {
      const int marker = timeline_ms(event.time);
      markers.push_back(marker);
      events_->addItem(label, marker);
    }
  }
  details_->setPlainText(report.join('\n'));
  if (!scene_changed_) {
    playback_.emplace(trace, run.timestep);
    timeline_->setRange(0, timeline_ms(playback_->duration()));
    static_cast<TimelineSlider*>(timeline_)->set_markers(std::move(markers));
    if (isVisible()) {
      if (autoplay && timeline_->maximum() > 0) {
        toggle_playback();
      } else {
        seek(0);
      }
    } else {
      time_label_->setText(tr("%1 / %2 s").arg(0.0, 0, 'f', 2).arg(playback_->duration(), 0, 'f', 2));
      update_controls();
    }
  }
}

} // namespace roadmaker::editor
