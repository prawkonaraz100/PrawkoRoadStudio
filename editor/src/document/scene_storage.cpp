// SPDX-License-Identifier: Apache-2.0

#include "document/scene_storage.hpp"

#include "roadmaker/osc/writer.hpp"
#include "roadmaker/xodr/writer.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QString>
#include <QTemporaryDir>
#include <QUuid>
#include <algorithm>
#include <system_error>
#include <vector>

#include "document/document.hpp"
#include "document/scene_sidecar.hpp"

namespace roadmaker::editor::scene_storage {
namespace {

using Path = std::filesystem::path;

QString qpath(const Path& path) {
  return QString::fromStdU16String(path.u16string());
}

std::string utf8(const Path& path) {
  return qpath(path).toStdString();
}

Expected<void> io_error(const char* reason, const Path& path) {
  return make_error(ErrorCode::IoFailure, reason, utf8(path));
}

Path with_suffix(Path path, const char* suffix) {
  path += suffix;
  return path;
}

Path journal_for(const Path& scene) {
  Path name(".");
  name += scene.filename();
  return scene.parent_path() / with_suffix(name, ".prawko-save");
}

Path backup_for(const Path& journal, const Path& name) {
  return journal / "previous" / name;
}

Expected<void> rename_file(const Path& from, const Path& to) {
  std::error_code error;
  std::filesystem::rename(from, to, error);
  if (error) {
    return make_error(ErrorCode::IoFailure, "Could not replace file: " + error.message(), utf8(to));
  }
  return {};
}

Expected<void> restore_file(const Path& backup, const Path& destination) {
  QFile original(qpath(backup));
  QSaveFile restored(qpath(destination));
  if (!original.open(QIODevice::ReadOnly) || !restored.open(QIODevice::WriteOnly)) {
    return io_error("Could not restore the previous file", destination);
  }
  while (!original.atEnd()) {
    const QByteArray chunk = original.read(128 * 1024);
    if (chunk.isEmpty() || restored.write(chunk) != chunk.size()) {
      return io_error("Could not restore the previous file", destination);
    }
  }
  if (!restored.commit()) {
    return io_error("Could not restore the previous file", destination);
  }
  return {};
}

std::vector<Path> member_paths(const Path& scene) {
  // Publish the map last, after the files referenced by it are in place.
  return {terrain_path(scene),
          scenario_path(scene),
          scene_sidecar::path_for(scene),
          scene,
          recovery_status_path(scene)};
}

Expected<void> retire_journal(const Path& journal) {
  // Remove the active journal name in one step, before deleting its contents.
  const QString retired =
      qpath(journal) + QStringLiteral(".done-") + QUuid::createUuid().toString(QUuid::Id128);
  if (auto moved = rename_file(journal, Path(retired.toStdU16String())); !moved) {
    return moved;
  }
  QDir(retired).removeRecursively();
  return {};
}

Expected<void> rollback(const Path& scene, const Path& journal) {
  const QFileInfo folder(qpath(journal));
  if (!folder.isDir() || folder.isSymLink()) {
    return io_error("The interrupted save journal is not a regular directory", journal);
  }
  QFile manifest(qpath(journal / "members.json"));
  if (!manifest.open(QIODevice::ReadOnly)) {
    return io_error("Could not read the interrupted save journal", journal);
  }
  const QJsonDocument listed = QJsonDocument::fromJson(manifest.readAll());
  manifest.close();
  if (!listed.isArray() || listed.array().isEmpty()) {
    return io_error("The interrupted save journal is invalid", journal);
  }
  const auto allowed = member_paths(scene);
  std::vector<Path> seen;

  struct Published {
    Path path;
    bool had_original;
  };

  std::vector<Published> touched;
  for (const auto& entry : listed.array()) {
    const auto record = entry.toObject();
    const auto found = std::ranges::find_if(allowed, [&](const Path& path) {
      return record.value(QStringLiteral("name")).toString() == qpath(path.filename());
    });
    if (found == allowed.end() || std::ranges::find(seen, *found) != seen.end() ||
        !record.value(QStringLiteral("remove")).isBool() ||
        !record.value(QStringLiteral("hadOriginal")).isBool()) {
      return io_error("The interrupted save journal is invalid", journal);
    }
    const Path name = found->filename();
    seen.push_back(*found);
    const bool removed = record.value(QStringLiteral("remove")).toBool();
    const bool published = removed ? QFileInfo::exists(qpath(journal / "removed" / name))
                                   : !QFileInfo::exists(qpath(journal / name));
    if (published) {
      touched.push_back({*found, record.value(QStringLiteral("hadOriginal")).toBool()});
    }
  }
  for (auto it = touched.rbegin(); it != touched.rend(); ++it) {
    const Path& destination = it->path;
    const Path name = destination.filename();
    const Path backup = backup_for(journal, name);
    if (it->had_original) {
      if (auto restored = restore_file(backup, destination); !restored) {
        return restored;
      }
    } else {
      std::error_code error;
      const QFileInfo info(qpath(destination));
      if (info.isSymLink() || (info.exists() && !info.isFile())) {
        return io_error("Could not remove the interrupted save's new file", destination);
      }
      std::filesystem::remove(destination, error);
      if (error) {
        return io_error("Could not remove the interrupted save's new file", destination);
      }
    }
  }
  return retire_journal(journal);
}

} // namespace

Path scenario_path(Path scene) {
  return scene.replace_extension(".xosc");
}

Path terrain_path(const Path& scene) {
  return scene.parent_path() / with_suffix(scene.stem(), ".terrain.asc");
}

Path recovery_status_path(Path scene) {
  return scene.replace_extension(".json");
}

namespace {

Expected<void>
write_snapshot(const Document& document, const SceneState& state, const Path& scene) {
  const std::string name = utf8(scene.stem());
  WriterOptions options;
  options.terrain_sidecar_name = utf8(terrain_path(scene).filename());
  if (document.network().terrain().empty() && !document.network().terrain().sidecar.empty()) {
    return io_error("The scene refers to a missing terrain grid", scene);
  }
  if (auto written = roadmaker::save_xodr(document.network(), scene, name, options); !written) {
    return written;
  }
  if (document.has_scenario()) {
    osc::Scenario scenario = document.scenario();
    auto& logic = scenario.road_network.logic_file;
    if (!logic) {
      logic.emplace();
    }
    logic->filepath = utf8(scene.filename());
    if (auto written = osc::save_xosc(scenario, scenario_path(scene)); !written) {
      return written;
    }
  }
  return scene_sidecar::save(scene_sidecar::path_for(scene), state);
}

} // namespace

Expected<void> save(const Document& document,
                    const SceneState& state,
                    const Path& scene,
                    const QByteArray* recovery_status) {
  if (qpath(scene.extension()).compare(QStringLiteral(".xodr"), Qt::CaseInsensitive) != 0) {
    return io_error("Choose an OpenDRIVE file for the scene", scene);
  }
  const Path journal = journal_for(scene);
  QLockFile lock(qpath(with_suffix(journal, ".lock")));
  lock.setStaleLockTime(0);
  if (!lock.tryLock(0)) {
    return io_error("Could not acquire the scene save lock", scene);
  }
  if (const QFileInfo info(qpath(journal)); info.exists() || info.isSymLink()) {
    if (auto restored = rollback(scene, journal); !restored) {
      return restored;
    }
  }
  QTemporaryDir staging(qpath(scene.parent_path() / ".prawko-stage-XXXXXX"));
  if (!staging.isValid()) {
    return io_error("Could not create a scene staging folder", scene);
  }
  const Path staged(staging.path().toStdU16String());
  if (auto written = write_snapshot(document, state, staged / scene.filename()); !written) {
    return written;
  }
  if (recovery_status != nullptr) {
    QSaveFile status(qpath(staged / recovery_status_path(scene).filename()));
    if (!status.open(QIODevice::WriteOnly) ||
        status.write(*recovery_status) != recovery_status->size() || !status.commit()) {
      return io_error("Could not write the recovery status", scene);
    }
  }
  const QDir stage_dir(staging.path());
  if (!stage_dir.mkdir(QStringLiteral("previous")) || !stage_dir.mkdir(QStringLiteral("removed"))) {
    return io_error("Could not create a previous-version folder", scene);
  }
  QJsonArray names;

  struct Member {
    Path destination;
    bool remove;
  };

  std::vector<Member> destinations;
  for (const Path& destination : member_paths(scene)) {
    const Path name = destination.filename();
    const bool replacement = QFileInfo::exists(qpath(staged / name));
    const QFileInfo info(qpath(destination));
    const bool remove = !replacement && recovery_status != nullptr && info.exists() &&
                        (destination == scenario_path(scene) || destination == terrain_path(scene));
    if (!replacement && !remove) {
      if (destination == scenario_path(scene) && (info.exists() || info.isSymLink())) {
        return io_error("This destination already has a scenario. Open the complete scene or "
                        "choose another filename",
                        destination);
      }
      continue;
    }
    if (info.isSymLink() || (info.exists() && !info.isFile())) {
      return io_error("A scene destination is not a regular file", destination);
    }
    if (info.exists() && !QFile::copy(qpath(destination), qpath(backup_for(staged, name)))) {
      return io_error("Could not keep a copy of the previous file", destination);
    }
    names.append(QJsonObject{{QStringLiteral("name"), qpath(name)},
                             {QStringLiteral("remove"), remove},
                             {QStringLiteral("hadOriginal"), info.exists()}});
    destinations.push_back({destination, remove});
  }
  const QByteArray bytes = QJsonDocument(names).toJson(QJsonDocument::Compact);
  QSaveFile manifest(qpath(staged / "members.json"));
  if (!manifest.open(QIODevice::WriteOnly) || manifest.write(bytes) != bytes.size() ||
      !manifest.commit()) {
    return io_error("Could not write the scene save journal", scene);
  }
  if (auto started = rename_file(staged, journal); !started) {
    return started;
  }
  staging.setAutoRemove(false);
  const auto abort = [&](const Error& original) -> Expected<void> {
    if (auto restored = rollback(scene, journal); !restored) {
      return make_error(original.code,
                        original.message + ". Previous files remain in the save journal: " +
                            restored.error().message,
                        utf8(journal));
    }
    return tl::unexpected(original);
  };
  for (const auto& [destination, remove] : destinations) {
    const Path name = destination.filename();
    const auto published = remove ? rename_file(destination, journal / "removed" / name)
                                  : rename_file(journal / name, destination);
    if (!published) {
      return abort(published.error());
    }
  }
  if (auto committed = retire_journal(journal); !committed) {
    return abort(committed.error());
  }
  return {};
}

Expected<std::unique_ptr<QLockFile>> lock_for_read(const Path& scene) {
  const Path journal = journal_for(scene);
  const QFileInfo scene_directory(qpath(scene.parent_path().empty() ? Path(".") : scene.parent_path()));
  if (!scene_directory.exists()) {
    return std::unique_ptr<QLockFile>{}; // Let the reader report the missing scene.
  }
  auto lock = std::make_unique<QLockFile>(qpath(with_suffix(journal, ".lock")));
  lock->setStaleLockTime(0);
  if (!lock->tryLock(0)) {
    const QFileInfo info(qpath(journal));
    // Read-only folders still support opening an ordinary scene.
    if (lock->error() == QLockFile::PermissionError && !info.exists() && !info.isSymLink()) {
      return std::unique_ptr<QLockFile>{};
    }
    return make_error(ErrorCode::IoFailure, "Could not acquire the scene read lock", utf8(scene));
  }
  const QFileInfo current(qpath(journal));
  if (current.exists() || current.isSymLink()) {
    if (auto restored = rollback(scene, journal); !restored) {
      return tl::unexpected(restored.error());
    }
  }
  return lock;
}

Expected<void> recover_interrupted_save(const Path& scene) {
  const QFileInfo info(qpath(journal_for(scene)));
  if (info.exists() || info.isSymLink()) {
    if (auto lock = lock_for_read(scene); !lock) {
      return tl::unexpected(lock.error());
    }
  }
  return {};
}

} // namespace roadmaker::editor::scene_storage
