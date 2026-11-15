/*
 * Copyright 2026 PrawkoRoadStudio
 * Licensed under the Apache License, Version 2.0.
 */

#include "render/cockpit.hpp"

#include "roadmaker/assets/prop_import.hpp"
#include "render/scene_builder.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryFile>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <spdlog/spdlog.h>

static void init_cockpit_resources() {
  Q_INIT_RESOURCE(resources);
}

namespace roadmaker::editor {
namespace {

[[nodiscard]] std::vector<RenderMeshData> load_part(const char* key,
                                                     const QJsonObject& layouts) {
  const auto part_layout = layouts.value(QLatin1String(key)).toObject();
  const auto xy = part_layout.value(QStringLiteral("center_xy")).toArray();
  const auto bottom = part_layout.value(QStringLiteral("min_z"));
  if (xy.size() != 2 || !xy[0].isDouble() || !xy[1].isDouble() || !bottom.isDouble()) {
    spdlog::error("cockpit: malformed Blender coordinates for {}", key);
    return {};
  }
  const std::array<float,3> offset{static_cast<float>(xy[0].toDouble()),
                                   static_cast<float>(xy[1].toDouble()),
                                   static_cast<float>(bottom.toDouble())};

  QFile bundled(QStringLiteral(":/cockpit/%1.glb").arg(QLatin1String(key)));
  if (!bundled.open(QIODevice::ReadOnly)) {
    spdlog::error("cockpit: missing bundled GLB {}", key);
    return {};
  }
  // The already-established prop reader accepts files. Keep the bundled model
  // fully self-contained, and release the temporary file after parsing.
  QTemporaryFile temp(QDir::tempPath() + QStringLiteral("/prawko-cockpit-XXXXXX.glb"));
  if (!temp.open() || temp.write(bundled.readAll()) < 1 || !temp.flush()) {
    spdlog::error("cockpit: failed extracting bundled GLB {}", key);
    return {};
  }
  temp.close(); // Windows: release the write handle before tinygltf opens the path.
  const auto imported =
      props::import_prop_model(std::filesystem::path(temp.fileName().toStdWString()),
                               std::string("prawko-cockpit-") + key);
  if (!imported) {
    spdlog::error("cockpit: cannot import {}: {}", key, imported.error().message);
    return {};
  }
  std::vector<RenderMeshData> meshes;
  meshes.reserve(imported->model.parts.size());
  for (const props::PropPart& part : imported->model.parts) {
    if (part.positions.empty() || part.indices.empty()) continue;
    RenderMeshData mesh = to_render_data(
        part.positions, part.normals, part.indices,
        {part.color[0], part.color[1], part.color[2], 1.0F});
    // import_prop_model deliberately moves each model's base to Z=0 and centers
    // XY. Restore authored offsets so all three independent GLBs share the
    // same origin: the driver's camera eye (Blender model contract).
    for (std::size_t i = 0; i + 2 < mesh.positions.size(); i += 3) {
      mesh.positions[i] += offset[0];
      mesh.positions[i + 1] += offset[1];
      mesh.positions[i + 2] += offset[2];
    }
    meshes.push_back(std::move(mesh));
  }
  return meshes;
}

} // namespace

std::optional<CockpitMeshes> load_cockpit_meshes() {
  static std::once_flag resource_once;
  std::call_once(resource_once, init_cockpit_resources);
  QFile layout(QStringLiteral(":/cockpit/layout.json"));
  if (!layout.open(QIODevice::ReadOnly)) return std::nullopt;
  const QJsonDocument doc = QJsonDocument::fromJson(layout.readAll());
  if (!doc.isObject()) return std::nullopt;
  CockpitMeshes meshes;
  meshes.shell = load_part("shell", doc.object());
  meshes.wheel = load_part("wheel", doc.object());
  meshes.wiper = load_part("wiper", doc.object());
  if (!meshes.valid()) return std::nullopt;
  return meshes;
}

InstanceData cockpit_pose(const DriverCamera& driver, const std::array<float,3>& anchor,
                          float rotation, bool flip_x) {
  const float c = std::cos(rotation);
  const float s = std::sin(rotation);
  const float flip = flip_x ? -1.0F : 1.0F;
  InstanceData instance;
  auto& m = instance.model;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    // Rotation around camera-forward, plus optional lateral flip for the
    // passenger-side wiper. The three camera axes remain orthogonal at all yaw,
    // pitch and roll values.
    m[axis] = flip * c * driver.right[axis] + flip * s * driver.up[axis];
    m[4 + axis] = driver.forward[axis];
    m[8 + axis] = -s * driver.right[axis] + c * driver.up[axis];
    m[12 + axis] = driver.eye[axis] +
                   anchor[0] * driver.right[axis] +
                   anchor[1] * driver.forward[axis] +
                   anchor[2] * driver.up[axis];
  }
  return instance;
}

float cockpit_wiper_sweep(double time_seconds, bool running) {
  if (!running || !std::isfinite(time_seconds)) return 0.0F;
  constexpr double cycle_hz = 0.72; // two blades sweep back and forth at ~43 cycles/min
  constexpr double tau = 2.0 * std::numbers::pi;
  const double phase = tau * cycle_hz * std::max(0.0, time_seconds);
  return static_cast<float>(1.08 * (0.5 - 0.5 * std::cos(phase)));
}

} // namespace roadmaker::editor
