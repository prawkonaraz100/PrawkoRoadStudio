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

#pragma once

// Renderer abstraction. NO OpenGL types or calls in this header — GL lives
// exclusively in editor/src/render/gl_*.  A future bgfx/WebGPU backend
// replaces GLRenderer without touching viewport or panel code.

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace roadmaker::editor {

struct RenderMeshHandle {
  std::uint32_t id = 0;

  [[nodiscard]] bool valid() const { return id != 0; }
};

/// Opaque handle to an uploaded texture (mirrors RenderMeshHandle). id 0 =
/// "no texture"; a Material with an invalid base_color renders flat.
struct TextureHandle {
  std::uint32_t id = 0;

  [[nodiscard]] bool valid() const { return id != 0; }
};

/// How a texture samples outside [0,1]. Repeat is the M2 default (tiled road
/// surfaces); ClampToEdge is for atlas-like single images (sign faces) whose
/// baked margin must not wrap and bleed at the edges.
enum class TextureWrap {
  Repeat,
  ClampToEdge,
};

/// CPU-side texture upload, RGBA8 row-major (4 bytes/texel, width*height
/// texels). The render boundary stays GL-free: no sampler/format enum here,
/// the GL backend picks the internal format.
struct TextureData {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> rgba;
  /// Wrap mode. Default Repeat keeps existing (tiled surface) behaviour byte
  /// for byte; sign faces upload ClampToEdge.
  TextureWrap wrap = TextureWrap::Repeat;
  bool srgb = false; // true for glTF sRGB baseColor; old material cache unchanged
};

/// A material references an optional base-color texture plus scalar factors.
/// Defaults reproduce the M2 flat look: no texture, white tint (the per-mesh
/// RenderMeshData::color remains the flat fallback when base_color is invalid,
/// so a default-constructed Material never changes existing output). Resolved
/// to a shader uniform set by the GL backend — no GL here.
struct Material {
  TextureHandle base_color;                          ///< invalid → use RenderMeshData::color
  TextureHandle normal;                              ///< invalid → geometric normal
  TextureHandle roughness;                           ///< invalid → roughness_value scalar
  TextureHandle metallic_roughness; ///< glTF G=roughness, B=metallic, linear data
  std::array<float, 4> tint{1.0F, 1.0F, 1.0F, 1.0F}; ///< multiplies the sample
  float uv_scale = 0.25F;                            ///< texels per meter: 0.25 = 4 m tile
  /// Scalar roughness fallback/modulator. DEFAULT 1.0 is fully rough: the sun
  /// specular lobe is scaled by (1 - roughness), so a default-constructed
  /// Material adds ZERO specular and reproduces the pre-PBR output exactly.
  float roughness_value = 1.0F;
  float normal_strength = 1.0F; ///< perturbation scale for the normal map
  float metallic_value = 0.0F; ///< glTF metallicFactor, 0 for legacy
  bool gltf_pbr = false; ///< opt-in GGX BRDF, legacy shader unchanged
  // Diffuse sky/road fill on near-vertical concrete: zero for every default
  // material and almost zero on horizontal sidewalk tops.
  float vertical_fill = 0.0F;
  bool unlit = false;           ///< markings/sky bypass lighting
  bool asphalt = false;        ///< asphalt responds to wet road reflections/snow cover
};

/// Per-instance transform for prop instancing (many props share one mesh).
/// Column-major 4x4, kernel frame (Z-up, meters).
struct InstanceData {
  std::array<float, 16> model{1.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              1.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              1.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              0.0F,
                              1.0F};
};

enum class PrimitiveKind {
  Triangles,
  Lines,
  /// One vertex per point, sized in the vertex shader. Added for lidar clouds
  /// (p7-s3, #243) — the first primitive here that is neither a surface nor an
  /// outline.
  Points,
};

/// CPU-side mesh data ready for upload. Kernel frame (Z-up, meters),
/// float32 as this is a render boundary.
struct RenderMeshData {
  std::vector<float> positions; // xyz triplets
  std::vector<float> normals;   // xyz triplets (empty for Lines)
  std::vector<float> uvs;       // uv pairs (u=s, v=t in meters); empty = untextured
  std::vector<std::uint32_t> indices;
  std::array<float, 4> color{0.8F, 0.8F, 0.8F, 1.0F};
  PrimitiveKind kind = PrimitiveKind::Triangles;
  float roughness = 1.0F; // imported glTF factor; default preserves legacy props
  int base_color_texture = -1; // index into imported PropModel::textures
  bool gltf_pbr = false;
  float metallic = 0.0F;
  int metallic_roughness_texture = -1;
  int normal_texture = -1;
  float normal_strength = 1.0F;
  int alpha_mode = 0; // glTF 0 OPAQUE, 1 MASK, 2 BLEND
  float alpha_cutoff = 0.5F;
  float transmission = 0.0F;
  float ior = 1.5F;
};

/// Per-item feedback state. Drives the accent emphasis in the mesh shader:
/// None renders the base color, Hover a subtle accent brighten, Selected a
/// stronger accent tint. The accent color itself is a theme token handed to
/// the renderer via BackdropColors::highlight (render/ stays Qt-free).
enum class HighlightState {
  None,
  Hover,
  Selected,
};

struct DrawItem {
  RenderMeshHandle mesh;
  HighlightState state = HighlightState::None;
  Material material{};                     ///< defaults reproduce the flat look
  std::span<const InstanceData> instances; ///< empty = one draw at identity
};

/// Right-handed, Z-up camera matrices (column-major 4x4).
struct CameraMatrices {
  std::array<float, 16> view{};
  std::array<float, 16> projection{};
  std::array<float, 3> eye{}; // world-space camera position (backdrop fade)
};

/// Viewport backdrop colors — sky gradient, ground grid, origin axes. Plain
/// floats because this is the theme→renderer boundary (render/ stays
/// Qt-free); values come from the active Theme (editor/src/theme/).
struct BackdropColors {
  std::array<float, 3> sky_top{0.137F, 0.153F, 0.180F};
  std::array<float, 3> sky_horizon{0.227F, 0.255F, 0.286F};
  std::array<float, 4> grid_major{0.239F, 0.263F, 0.294F, 0.85F};
  std::array<float, 4> grid_minor{0.149F, 0.169F, 0.192F, 0.55F};
  std::array<float, 3> axis_x{0.75F, 0.35F, 0.32F};
  std::array<float, 3> axis_y{0.35F, 0.65F, 0.36F};
  /// Selection/hover emphasis color (the theme `accent` token). Lives here
  /// because BackdropColors is the single theme→renderer float channel, wired
  /// to re-push on theme changes; the mesh shader mixes toward it per
  /// DrawItem::state.
  std::array<float, 3> highlight{0.961F, 0.651F, 0.137F};
};

/// Scene lighting + sky description (all plain floats — theme/render boundary
/// stays Qt- and GL-free). Hemisphere ambient (sky/ground lerp by normal.z) +
/// one directional sun; R4 shadow maps are enabled only for Textured mode. The DEFAULT values are
/// the daytime "Textured" preset (the new default render mode); `sober_lighting`
/// below reproduces the flat M2 look for the Sober toggle.
struct Environment {
  std::array<float, 3> sun_dir{0.35F, 0.25F, 0.90F}; ///< world, Z-up (to light)
  std::array<float, 3> sun_color{1.0F, 0.97F, 0.90F};
  std::array<float, 3> sky_color{0.5F, 0.7F, 1.0F};      ///< hemisphere up
  std::array<float, 3> ground_color{0.4F, 0.38F, 0.35F}; ///< hemisphere down
  float sun_intensity = 1.0F;
  float ambient = 0.35F;
  // Weather/seasons are shared across all GL cameras, including three mirror FBOs.
  // Zero values preserve the existing daylight image exactly.
  float fog_density = 0.0F;
  std::array<float, 3> fog_color{0.67F, 0.71F, 0.75F};
  float wetness = 0.0F;
  float snow_cover = 0.0F;
  float season = 0.0F; // 0 summer, 1 autumn, 2 winter (ground shader palette)
  bool shadows_enabled = false; // legacy + Sober unchanged; Textured enables R4
  float shadow_bounce = 0.0F; // R4b2: opt-in sky bounce ONLY in sunlight shadow
  // World-space pair of cone spotlights following the selected esmini driver.
  // Zero strength means NO lights for legacy, day, Sober and static view.
  std::array<std::array<float, 3>, 2> headlight_positions{};
  std::array<float, 3> headlight_forward{1.0F, 0.0F, 0.0F};
  float headlight_intensity = 0.0F;
};

/// S3.3: shared time-of-day presets for all cameras, including mirror FBOs
/// and exported video frames. Day is the legacy environment (unchanged).
enum class TimeOfDay { Day, Dawn, Dusk, Night };

[[nodiscard]] constexpr std::string_view time_of_day_key(TimeOfDay phase) {
  switch (phase) {
  case TimeOfDay::Dawn: return "dawn";
  case TimeOfDay::Dusk: return "dusk";
  case TimeOfDay::Night: return "night";
  case TimeOfDay::Day: break;
  }
  return "day";
}

/// The daytime "Textured" render mode: hemisphere sky/ground + a warm sun.
[[nodiscard]] inline Environment textured_lighting() {
  Environment env{};
  env.shadows_enabled = true;
  env.shadow_bounce = 0.11F; // low-energy reflected skylight, never erase shadows
  return env;
}

[[nodiscard]] inline Environment time_of_day_lighting(TimeOfDay phase) {
  Environment env = textured_lighting();
  switch (phase) {
  case TimeOfDay::Dawn:
    env.sun_dir = {0.90F, 0.25F, 0.16F};
    env.sun_color = {1.0F, 0.59F, 0.30F};
    env.sky_color = {0.32F, 0.43F, 0.65F};
    env.sun_intensity = 0.52F;
    env.ambient = 0.24F;
    env.shadow_bounce = 0.07F;
    break;
  case TimeOfDay::Dusk:
    env.sun_dir = {-0.9F, 0.1F, 0.10F};
    env.sun_color = {1.0F, 0.44F, 0.23F};
    env.sky_color = {0.22F, 0.29F, 0.52F};
    env.sun_intensity = 0.32F;
    env.ambient = 0.16F;
    env.shadow_bounce = 0.05F;
    break;
  case TimeOfDay::Night:
    env.sun_dir = {0.35F, 0.25F, 0.90F};
    env.sun_color = {0.28F, 0.38F, 0.62F};
    env.sky_color = {0.12F, 0.20F, 0.35F};
    env.ground_color = {0.12F, 0.16F, 0.22F};
    env.sun_intensity = 0.035F;
    env.ambient = 0.15F;
    env.shadow_bounce = 0.0F; // no directional shadow pass at night
    break;
  case TimeOfDay::Day: break;
  }
  return env;
}

/// Change only the sky of an existing themed backdrop. The editor's Sober
/// mode and the default day retain their previous colors unchanged.
[[nodiscard]] inline BackdropColors time_of_day_backdrop(BackdropColors backdrop,
                                                         TimeOfDay phase) {
  switch (phase) {
  case TimeOfDay::Dawn:
    backdrop.sky_top = {0.18F, 0.31F, 0.48F};
    backdrop.sky_horizon = {0.76F, 0.39F, 0.24F};
    break;
  case TimeOfDay::Dusk:
    backdrop.sky_top = {0.075F, 0.13F, 0.28F};
    backdrop.sky_horizon = {0.62F, 0.28F, 0.24F};
    break;
  case TimeOfDay::Night:
    backdrop.sky_top = {0.008F, 0.016F, 0.041F};
    backdrop.sky_horizon = {0.024F, 0.039F, 0.079F};
    break;
  case TimeOfDay::Day: break;
  }
  return backdrop;
}

/// S3.3 scene-local weather and seasonal environment. Defaults are neutral
/// to protect old scenes and Sober renders.
enum class WeatherPreset { Clear, Rain, Fog, Snow };
enum class SeasonPreset { Summer, Autumn, Winter };

[[nodiscard]] constexpr std::string_view weather_key(WeatherPreset preset) {
  switch (preset) {
  case WeatherPreset::Rain: return "rain";
  case WeatherPreset::Fog: return "fog";
  case WeatherPreset::Snow: return "snow";
  case WeatherPreset::Clear: break;
  }
  return "clear";
}

[[nodiscard]] constexpr std::string_view season_key(SeasonPreset season) {
  switch (season) {
  case SeasonPreset::Autumn: return "autumn";
  case SeasonPreset::Winter: return "winter";
  case SeasonPreset::Summer: break;
  }
  return "summer";
}

[[nodiscard]] inline Environment conditioned_lighting(TimeOfDay time,
                                                       WeatherPreset weather,
                                                       SeasonPreset season) {
  Environment env = time_of_day_lighting(time);
  if (season == SeasonPreset::Autumn) {
    env.season = 1.0F;
    env.sun_intensity *= 0.90F;
  } else if (season == SeasonPreset::Winter) {
    env.season = 2.0F;
    env.snow_cover = 0.18F;
    env.sun_intensity *= 0.75F;
  }
  switch (weather) {
  case WeatherPreset::Rain:
    env.wetness = 0.85F;
    env.fog_density = 0.005F;
    env.fog_color = {0.40F, 0.48F, 0.55F};
    env.sun_intensity *= 0.28F;
    env.ambient *= 0.75F;
    break;
  case WeatherPreset::Fog:
    env.fog_density = 0.031F; // approximately 60m of severe visual limitation
    env.fog_color = {0.65F, 0.69F, 0.71F};
    env.sun_intensity *= 0.48F;
    break;
  case WeatherPreset::Snow:
    env.snow_cover = 0.77F;
    env.fog_density = 0.012F;
    env.fog_color = {0.76F, 0.80F, 0.84F};
    env.sun_intensity *= 0.50F;
    break;
  case WeatherPreset::Clear: break;
  }
  return env;
}

[[nodiscard]] inline BackdropColors conditioned_backdrop(BackdropColors backdrop,
                                                          TimeOfDay time,
                                                          WeatherPreset weather) {
  backdrop = time_of_day_backdrop(backdrop, time);
  if (weather == WeatherPreset::Fog) {
    backdrop.sky_top = {0.57F, 0.62F, 0.66F};
    backdrop.sky_horizon = {0.65F, 0.69F, 0.71F};
  } else if (weather == WeatherPreset::Rain) {
    backdrop.sky_top = {0.12F, 0.17F, 0.23F};
    backdrop.sky_horizon = {0.31F, 0.38F, 0.44F};
  } else if (weather == WeatherPreset::Snow) {
    backdrop.sky_top = {0.40F, 0.49F, 0.58F};
    backdrop.sky_horizon = {0.70F, 0.77F, 0.83F};
  }
  return backdrop;
}

/// The "Sober" render mode — reproduces the M2 flat look exactly: a white,
/// normal-independent ambient of 0.35 plus a white directional term of
/// 0.65*lambert along the original hardcoded light direction, i.e. the old
/// `0.35 + 0.65*lambert` grey shading. Kept as the packaging/CI smoke path.
[[nodiscard]] inline Environment sober_lighting() {
  Environment env;
  env.sun_dir = {0.35F, 0.25F, 0.90F};
  env.sun_color = {1.0F, 1.0F, 1.0F};
  env.sky_color = {1.0F, 1.0F, 1.0F};    // sky == ground → normal-independent
  env.ground_color = {1.0F, 1.0F, 1.0F}; // ambient term, so no hemisphere tint
  env.sun_intensity = 0.65F;
  env.ambient = 0.35F;
  return env;
}

class Renderer {
public:
  virtual ~Renderer() = default;

  virtual bool init() = 0;
  virtual void shutdown() = 0;

  virtual RenderMeshHandle upload(const RenderMeshData& data) = 0;
  virtual void remove(RenderMeshHandle handle) = 0;
  virtual void clear_meshes() = 0;

  /// Uploads an RGBA8 texture and returns an opaque handle; TextureData may be
  /// discarded after the call. Safe only after init().
  virtual TextureHandle upload(const TextureData& data) = 0;
  virtual void remove(TextureHandle handle) = 0;

  /// Sets the scene lighting/sky environment. Safe to call before init(); the
  /// frame after the call uses the new environment.
  virtual void set_environment(const Environment& env) = 0;

  /// Enables the procedural grass ground plane at world height `base_z` (meters,
  /// Z-up), drawn opaque with depth so the road network occludes it. Disabled =
  /// no ground (the Sober render mode). Safe to call before init(); it only
  /// stores state. `base_z` should sit just under the network so coplanar road
  /// surfaces don't z-fight (see scene_builder `ground_base_z`).
  virtual void set_ground(bool enabled, float base_z) = 0;

  /// Sets the sky/grid/axes colors. Safe to call before init(); the frame
  /// after the call uses the new colors.
  virtual void set_backdrop(const BackdropColors& colors) = 0;

  /// Renders one frame into the currently bound framebuffer region.
  /// `width`/`height` are FRAMEBUFFER pixels (HiDPI-safe), not window units.
  virtual void render(const std::vector<DrawItem>& items,
                      const CameraMatrices& camera,
                      int width,
                      int height) = 0;
};

} // namespace roadmaker::editor
