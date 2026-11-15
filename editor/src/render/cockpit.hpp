/*
 * Copyright 2026 PrawkoRoadStudio
 * Licensed under the Apache License, Version 2.0.
 */
#pragma once

// Authored in Blender, loaded as immutable camera-relative render meshes.
// The cockpit does NOT enter OpenDRIVE, scenery selection or the project
// library: it belongs solely to first-person camera composition.

#include <array>
#include <optional>
#include <vector>

#include "render/renderer.hpp"
#include "viewport/camera.hpp"

namespace roadmaker::editor {

struct CockpitMeshes {
  std::vector<RenderMeshData> shell;
  std::vector<RenderMeshData> wheel;
  std::vector<RenderMeshData> wiper;
  [[nodiscard]] bool valid() const {
    return !shell.empty() && !wheel.empty() && !wiper.empty();
  }
};

/// Reads three original, Blender-generated GLBs embedded in the Qt resource
/// binary. Inverts the prop importer's normalisation from bundled metadata,
/// yielding exact Blender eye-relative coordinates in metres.
[[nodiscard]] std::optional<CockpitMeshes> load_cockpit_meshes();

/// Local axes are X right, Y forward, Z up. The resulting column-major model
/// matrix follows the vehicle's pitch/roll, without altering the external camera.
[[nodiscard]] InstanceData cockpit_pose(const DriverCamera& driver,
                                        const std::array<float, 3>& anchor = {},
                                        float rotation = 0.0F,
                                        bool flip_x = false);

/// A deterministic, playback-time windshield wiper phase; exports do not
/// accidentally depend on the speed at which FFmpeg encodes the frames.
[[nodiscard]] float cockpit_wiper_sweep(double time_seconds, bool running);

} // namespace roadmaker::editor
