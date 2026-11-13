/*
 * Copyright 2026 Robomous
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "document/autosave.hpp"

#include <spdlog/spdlog.h>

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QStandardPaths>
#include <QUndoStack>
#include <algorithm>
#include <system_error>
#include <utility>

#include "document/document.hpp"
#include "document/scene_sidecar.hpp"
#include "document/scene_storage.hpp"

namespace roadmaker::editor {
namespace {

constexpr int kSidecarVersion = 2;

std::filesystem::path
member_path(const std::filesystem::path& dir, const QString& session, const char* extension) {
  return dir / std::filesystem::path((session + QLatin1String(extension)).toStdU16String());
}

} // namespace

AutosaveManager::AutosaveManager(Document& document,
                                 std::filesystem::path recovery_dir,
                                 QString session_id,
                                 Clock clock,
                                 QObject* parent)
    : QObject(parent), document_(document), dir_(std::move(recovery_dir)),
      session_(std::move(session_id)),
      clock_(clock ? std::move(clock) : [] { return QDateTime::currentMSecsSinceEpoch(); }),
      last_autosave_ms_(clock_()) {
  connect(document_.undo_stack(),
          &QUndoStack::indexChanged,
          this,
          &AutosaveManager::on_command_committed);
  connect(&document_, &Document::saved, this, &AutosaveManager::clear_recovery);
  connect(&document_, &Document::loaded, this, &AutosaveManager::clear_recovery);
  // A recovery copy around junction regeneration: the hairiest lifetime zone
  // dogfooding found (hardening §4.6).
  connect(&document_, &Document::regeneration_checkpoint, this, [this] {
    if (auto written = autosave_now(); !written.has_value()) {
      spdlog::warn("regeneration-checkpoint autosave failed: {}", written.error().message);
    }
  });
}

AutosaveManager::~AutosaveManager() = default;

void AutosaveManager::set_enabled(bool enabled) {
  enabled_ = enabled;
  if (!enabled_) {
    clear_recovery();
  }
}

void AutosaveManager::on_command_committed() {
  if (!document_.is_dirty()) {
    // Undo back to the clean state, or a stack clear on load/reset —
    // nothing worth protecting.
    commands_since_autosave_ = 0;
    return;
  }
  if (++commands_since_autosave_ < kCommandsPerAutosave) {
    return;
  }
  if (auto written = autosave_now(); !written.has_value()) {
    spdlog::warn("autosave failed: {}", written.error().message);
  }
}

void AutosaveManager::maybe_autosave() {
  if (!enabled_) {
    return;
  }
  if (!document_.is_dirty() || document_.preview_active()) {
    return;
  }
  if (clock_() - last_autosave_ms_ < kIntervalMs) {
    return;
  }
  if (auto written = autosave_now(); !written.has_value()) {
    spdlog::warn("autosave failed: {}", written.error().message);
  }
}

Expected<void> AutosaveManager::autosave_now() {
  if (!enabled_) {
    return {};
  }
  if (!document_.is_dirty() || document_.preview_active()) {
    return {};
  }
  std::error_code ec;
  std::filesystem::create_directories(dir_, ec);
  if (ec) {
    return make_error(
        ErrorCode::IoFailure, "cannot create recovery directory: " + ec.message(), dir_.string());
  }
  if (!session_lock_) {
    auto lock = std::make_unique<QLockFile>(
        QString::fromStdU16String(member_path(dir_, session_, ".session.lock").u16string()));
    lock->setStaleLockTime(0);
    if (!lock->tryLock(0)) {
      return make_error(ErrorCode::IoFailure, "Could not lock this recovery session.");
    }
    session_lock_ = std::move(lock);
  }

  const QString original = document_.file_path();
  const qint64 now = clock_();
  const QJsonObject sidecar{
      {QStringLiteral("version"), kSidecarVersion},
      {QStringLiteral("originalPath"), original},
      {QStringLiteral("dirty"), true},
      {QStringLiteral("saveToken"), save_token_ + 1},
      {QStringLiteral("writtenMs"), now},
      {QStringLiteral("hasScenario"), document_.has_scenario()},
  };
  const QByteArray status = QJsonDocument(sidecar).toJson(QJsonDocument::Compact);
  if (auto written = scene_storage::save(document_, document_.current_scene_state(),
                                         xodr_path(), &status);
      !written) {
    return written;
  }
  ++save_token_;
  last_autosave_ms_ = now;
  commands_since_autosave_ = 0;
  return {};
}

void AutosaveManager::clear_recovery() {
  discard(RecoverySet{.xodr = xodr_path(), .sidecar = sidecar_path(),
                      .scene_state = scene_state_path(), .session = session_});
  session_lock_.reset();
  commands_since_autosave_ = 0;
}

std::filesystem::path AutosaveManager::xodr_path() const {
  return member_path(dir_, session_, ".xodr");
}

std::filesystem::path AutosaveManager::sidecar_path() const {
  return member_path(dir_, session_, ".json");
}

std::filesystem::path AutosaveManager::scene_state_path() const {
  // Derived from the recovery copy, not spelled out here, so it stays whatever
  // Document::load() will look for beside it.
  return scene_sidecar::path_for(xodr_path());
}

std::vector<RecoverySet>
AutosaveManager::pending_recoveries(const std::filesystem::path& recovery_dir,
                                    const QString& current_session) {
  std::vector<RecoverySet> sets;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(recovery_dir, ec)) {
    if (entry.path().extension() != ".json") {
      continue;
    }
    // <session>.rmscene.json is a scene sidecar, not a status record (#325).
    // Its stem ends in ".rmscene", so it would otherwise be scanned as a
    // session of that name — today only the missing-.xodr check below saves us,
    // and that is an accident, not a rule.
    if (entry.path().stem().extension() == ".rmscene") {
      continue;
    }
    const QString session = QString::fromStdU16String(entry.path().stem().u16string());
    if (session == current_session) {
      continue;
    }
    QLockFile owner(QString::fromStdU16String(
        member_path(recovery_dir, session, ".session.lock").u16string()));
    owner.setStaleLockTime(0);
    if (!owner.tryLock(0)) {
      continue; // Another editor still owns this unsaved scene.
    }
    const auto xodr = member_path(recovery_dir, session, ".xodr");
    if (auto recovered = scene_storage::recover_interrupted_save(xodr); !recovered) {
      spdlog::warn("recovery save remains pending: {}", recovered.error().message);
      continue;
    }
    QFile file(QString::fromStdU16String(entry.path().u16string()));
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    const QJsonDocument sidecar = QJsonDocument::fromJson(file.readAll());
    if (!sidecar.isObject()) {
      continue;
    }
    const QJsonObject object = sidecar.object();
    const int version = object.value(QStringLiteral("version")).toInt(1);
    if (version < 1 || version > kSidecarVersion ||
        (version == kSidecarVersion &&
         !object.value(QStringLiteral("hasScenario")).isBool())) {
      continue;
    }
    RecoverySet set{
        .xodr = xodr,
        .sidecar = entry.path(),
        .scene_state = scene_sidecar::path_for(xodr),
        .session = session,
        .original_path = object.value(QStringLiteral("originalPath")).toString(),
        .dirty = object.value(QStringLiteral("dirty")).toBool(false),
        .save_token = object.value(QStringLiteral("saveToken")).toVariant().toLongLong(),
        .written_ms = object.value(QStringLiteral("writtenMs")).toVariant().toLongLong(),
        .version = version,
        .has_scenario = object.value(QStringLiteral("hasScenario")).toBool(),
    };
    std::error_code exists_ec;
    if (!std::filesystem::exists(set.xodr, exists_ec)) {
      continue; // sidecar without its recovery copy — nothing to offer
    }
    sets.push_back(std::move(set));
  }
  std::ranges::sort(
      sets, [](const RecoverySet& a, const RecoverySet& b) { return a.written_ms > b.written_ms; });
  return sets;
}

bool AutosaveManager::should_recover(const RecoverySet& set) {
  return set.dirty;
}

void AutosaveManager::discard(const RecoverySet& set) {
  if (!scene_storage::recover_interrupted_save(set.xodr)) {
    return;
  }
  std::error_code ec;
  std::filesystem::remove(scene_storage::scenario_path(set.xodr), ec);
  std::filesystem::remove(scene_storage::terrain_path(set.xodr), ec);
  std::filesystem::remove(set.xodr, ec);
  std::filesystem::remove(set.sidecar, ec);
  std::filesystem::remove(set.scene_state, ec);
}

std::filesystem::path AutosaveManager::default_recovery_dir() {
  const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return std::filesystem::path(base.toStdU16String()) / "recovery";
}

} // namespace roadmaker::editor
