// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "roadmaker/error.hpp"

#include <filesystem>
#include <memory>

class QByteArray;
class QLockFile;

namespace roadmaker::editor {

class Document;
struct SceneState;

namespace scene_storage {

[[nodiscard]] std::filesystem::path scenario_path(std::filesystem::path scene);
[[nodiscard]] std::filesystem::path terrain_path(const std::filesystem::path& scene);
[[nodiscard]] std::filesystem::path recovery_status_path(std::filesystem::path scene);

/// Publishes a scene with rollback copies for each destination. An interrupted
/// publication is rolled back before the next editor open or save.
[[nodiscard]] Expected<void> save(const Document& document,
                                  const SceneState& state,
                                  const std::filesystem::path& scene,
                                  const QByteArray* recovery_status = nullptr);
[[nodiscard]] Expected<void> recover_interrupted_save(const std::filesystem::path& scene);
[[nodiscard]] Expected<std::unique_ptr<QLockFile>>
lock_for_read(const std::filesystem::path& scene);

} // namespace scene_storage
} // namespace roadmaker::editor
