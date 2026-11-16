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

#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "render/renderer.hpp"
#include "render/render_quality.hpp"

namespace roadmaker::editor {

/// OpenGL 3.3 core implementation of Renderer. The ONLY file pair (with
/// gl_functions) that touches GL.
class GLRenderer final : public Renderer {
public:
  struct Tuning {
    bool cache_instance_uploads = true;
    bool stabilize_sun_shadows = true;
    float max_anisotropy = 4.0F;
  };
  struct Statistics {
    std::uint64_t instance_uploads = 0;
    std::uint64_t instance_bytes = 0;
  };
  GLRenderer() = default;
  explicit GLRenderer(Tuning tuning) : tuning_(tuning) {}
  [[nodiscard]] Statistics statistics() const { return statistics_; }
  [[nodiscard]] float texture_anisotropy() const { return texture_anisotropy_; }
  void reset_statistics() { statistics_ = {}; }
  GLRenderer(const GLRenderer&) = delete;
  GLRenderer& operator=(const GLRenderer&) = delete;
  GLRenderer(GLRenderer&&) = delete;
  GLRenderer& operator=(GLRenderer&&) = delete;
  ~GLRenderer() override;

  bool init() override;
  void shutdown() override;

  RenderMeshHandle upload(const RenderMeshData& data) override;
  void remove(RenderMeshHandle handle) override;
  void clear_meshes() override;

  TextureHandle upload(const TextureData& data) override;
  void remove(TextureHandle handle) override;

  void set_environment(const Environment& env) override;
  void set_ground(bool enabled, float base_z) override;
  void set_backdrop(const BackdropColors& colors) override;

  void render(const std::vector<DrawItem>& items,
              const CameraMatrices& camera,
              int width,
              int height) override;

private:
  struct GpuMesh {
    std::uint32_t vao = 0;
    std::uint32_t vbo = 0;
    std::uint32_t ebo = 0;
    // Per-instance transform buffer (attribs 3-6), created lazily on the first
    // instanced draw of this mesh; 0 = never instanced. Deleted in destroy().
    std::uint32_t instance_vbo = 0;
    InstanceBufferCache instance_cache;
    std::int32_t index_count = 0;
    std::array<float, 4> color{};
    float roughness = 1.0F;
    int alpha_mode = 0;
    float alpha_cutoff = 0.5F;
    float transmission = 0.0F;
    float ior = 1.5F;
    std::array<float,3> center{}; // local mesh bounds center for glass sorting
    PrimitiveKind kind = PrimitiveKind::Triangles;
  };

  void destroy(GpuMesh& mesh);
  void prepare_instances(GpuMesh& mesh, std::span<const InstanceData> instances);
  Tuning tuning_{};
  Statistics statistics_{};
  float texture_anisotropy_ = 1.0F;
  void draw_backdrop(const CameraMatrices& camera);
  void draw_ground(const CameraMatrices& camera);
  bool init_shadow_resources();
  void release_shadow_resources();
  void draw_shadow_map(const std::vector<DrawItem>& items,
                       const CameraMatrices& camera, int width, int height);
  void draw_depth_casters(const std::vector<DrawItem>& items);
  void draw_street_shadow_maps(const std::vector<DrawItem>& items, int width, int height);
  void set_street_shadow_uniforms(bool ground);
  bool ensure_planar_resources(int width,int height);
  void release_planar_resources();
  void draw_planar_reflection(const std::vector<DrawItem>& items,
                              const CameraMatrices& camera,int width,int height);

  std::unordered_map<std::uint32_t, GpuMesh> meshes_;
  std::uint32_t next_id_ = 1;
  // GL texture names keyed by TextureHandle::id (id space disjoint from meshes).
  std::unordered_map<std::uint32_t, std::uint32_t> textures_;
  std::uint32_t next_tex_id_ = 1;
  Environment environment_{};
  std::uint32_t program_ = 0;
  std::int32_t u_view_ = -1;
  std::int32_t u_projection_ = -1;
  std::int32_t u_model_ = -1;
  std::int32_t u_use_instancing_ = -1;
  std::int32_t u_color_ = -1;
  std::int32_t u_highlight_ = -1;
  std::int32_t u_accent_ = -1;
  std::int32_t u_lit_ = -1;
  std::int32_t u_has_texture_ = -1;
  std::int32_t u_base_color_ = -1;
  std::int32_t u_uv_scale_ = -1;
  std::int32_t u_tint_ = -1;
  // PBR-lite material uniforms (normal + roughness maps, per-draw specular).
  std::int32_t u_has_normal_ = -1;
  std::int32_t u_normal_ = -1;
  std::int32_t u_normal_strength_ = -1;
  std::int32_t u_has_roughness_ = -1;
  std::int32_t u_roughness_map_ = -1;
  std::int32_t u_roughness_ = -1;
  std::int32_t u_gltf_pbr_ = -1;
  std::int32_t u_metallic_ = -1;
  std::int32_t u_has_metallic_roughness_ = -1;
  std::int32_t u_metallic_roughness_ = -1;
  std::int32_t u_alpha_mode_ = -1;
  std::int32_t u_alpha_cutoff_ = -1;
  std::int32_t u_transmission_ = -1;
  std::int32_t u_ior_ = -1;
  std::int32_t u_vertical_fill_ = -1;
  std::int32_t u_camera_pos_ = -1;
  // Environment lighting uniforms (driven by set_environment).
  std::int32_t u_sun_dir_ = -1;
  std::int32_t u_sun_color_ = -1;
  std::int32_t u_sun_intensity_ = -1;
  std::int32_t u_sky_light_ = -1;
  std::int32_t u_ground_light_ = -1;
  std::int32_t u_ambient_ = -1;
  std::int32_t u_fog_density_ = -1;
  std::int32_t u_fog_color_ = -1;
  std::int32_t u_wetness_ = -1;
  std::int32_t u_pbr_wetness_ = -1;
  std::int32_t u_emissive_ = -1;
  std::int32_t u_water_surface_ = -1;
  std::int32_t u_planar_enabled_ = -1;
  std::int32_t u_planar_texture_ = -1;
  std::int32_t u_planar_vp_ = -1;
  std::int32_t u_planar_secondary_texture_ = -1;
  std::int32_t u_planar_secondary_vp_ = -1;
  std::int32_t u_planar_primary_z_ = -1;
  std::int32_t u_planar_secondary_z_ = -1;
  std::int32_t u_planar_secondary_active_ = -1;
  std::int32_t u_planar_clip_z_ = -1;
  std::int32_t u_snow_cover_ = -1;
  std::int32_t u_asphalt_ = -1;
  std::int32_t u_shadow_enabled_ = -1;
  std::int32_t u_shadow_light_matrix_ = -1;
  std::int32_t u_shadow_map_ = -1;
  std::int32_t u_shadow_texel_ = -1;
  std::int32_t u_shadow_bounce_ = -1;
  std::int32_t u_headlight_left_ = -1;
  std::int32_t u_headlight_right_ = -1;
  std::int32_t u_headlight_forward_ = -1;
  std::int32_t u_headlight_intensity_ = -1;
  std::int32_t u_traffic_headlight_left_ = -1;
  std::int32_t u_traffic_headlight_right_ = -1;
  std::int32_t u_traffic_headlight_forward_ = -1;
  std::int32_t u_traffic_headlight_intensity_ = -1;
  std::int32_t u_street_count_ = -1;
  std::int32_t u_street_intensity_ = -1;
  std::int32_t u_street_shadows_enabled_ = -1;
  std::array<std::int32_t,kMaxStreetLamps> u_street_shadow_matrix_{};
  std::array<std::int32_t,kMaxStreetLamps> u_street_shadow_sampler_{};
  std::array<std::int32_t,kMaxStreetLamps> u_street_pos_{};
  std::array<std::int32_t,kMaxStreetLamps> u_street_dir_{};
  std::uint32_t shadow_program_ = 0;
  std::uint32_t shadow_fbo_ = 0;
  std::uint32_t shadow_depth_ = 0;
  bool shadow_ready_ = false;
  bool shadow_active_ = false;
  static constexpr int kShadowSize = 1024;
  static constexpr int kStreetShadowSize = 512;
  std::array<std::uint32_t,kMaxStreetLamps> street_shadow_fbo_{};
  std::array<std::uint32_t,kMaxStreetLamps> street_shadow_depth_{};
  std::array<std::array<float,16>,kMaxStreetLamps> street_shadow_matrix_{};
  bool street_shadow_ready_ = false;
  int street_shadow_count_ = 0;
  std::uint32_t planar_fbo_ = 0;
  std::uint32_t planar_color_ = 0;
  std::uint32_t planar_depth_ = 0;
  std::uint32_t planar_second_fbo_ = 0;
  std::uint32_t planar_second_color_ = 0;
  std::uint32_t planar_second_depth_ = 0;
  int planar_width_ = 0;
  int planar_height_ = 0;
  bool planar_ready_ = false;
  bool planar_active_ = false;
  bool planar_secondary_active_ = false;
  bool planar_rendering_ = false;
  std::array<float,16> planar_view_projection_{};
  std::array<float,16> planar_second_view_projection_{};
  float planar_surface_height_ = 0.0F;  // clip height of CURRENT mirror pass
  float planar_first_height_ = 0.0F;
  float planar_second_height_ = 0.0F;
  std::array<float,16> shadow_light_matrix_{};
  std::int32_t u_depth_light_ = -1;
  std::int32_t u_depth_model_ = -1;
  std::int32_t u_depth_instancing_ = -1;
  std::int32_t u_depth_mode_ = -1;
  std::int32_t u_depth_cutoff_ = -1;
  std::int32_t u_depth_has_tex_ = -1;
  std::int32_t u_depth_tex_ = -1;
  std::int32_t u_depth_uv_scale_ = -1;
  std::int32_t u_depth_tint_ = -1;
  std::int32_t u_depth_color_ = -1;

  // Backdrop passes (sky gradient + ground grid), both attributeless draws
  // through one shared empty VAO (core profile requires a bound VAO).
  BackdropColors backdrop_{};
  std::uint32_t empty_vao_ = 0;
  std::uint32_t sky_program_ = 0;
  std::int32_t u_sky_top_ = -1;
  std::int32_t u_sky_horizon_ = -1;
  std::uint32_t grid_program_ = 0;
  std::int32_t u_grid_view_ = -1;
  std::int32_t u_grid_projection_ = -1;
  std::int32_t u_grid_eye_ = -1;
  std::int32_t u_grid_major_ = -1;
  std::int32_t u_grid_minor_ = -1;
  std::int32_t u_grid_axis_x_ = -1;
  std::int32_t u_grid_axis_y_ = -1;

  // Procedural grass ground plane (textured mode only), at ground_base_z_.
  bool ground_enabled_ = false;
  float ground_base_z_ = 0.0F;
  std::uint32_t ground_program_ = 0;
  std::int32_t u_ground_view_ = -1;
  std::int32_t u_ground_projection_ = -1;
  std::int32_t u_ground_eye_ = -1;
  std::int32_t u_ground_base_z_ = -1;
  std::int32_t u_ground_horizon_ = -1;
  std::int32_t u_ground_sun_dir_ = -1;
  std::int32_t u_ground_sun_color_ = -1;
  std::int32_t u_ground_sun_intensity_ = -1;
  std::int32_t u_ground_sky_ = -1;
  std::int32_t u_ground_ambient_ = -1;
  std::int32_t u_ground_season_ = -1;
  std::int32_t u_ground_snow_ = -1;
  std::int32_t u_ground_fog_ = -1;
  std::int32_t u_ground_fog_color_ = -1;
  std::int32_t u_ground_shadow_enabled_ = -1;
  std::int32_t u_ground_shadow_light_matrix_ = -1;
  std::int32_t u_ground_shadow_map_ = -1;
  std::int32_t u_ground_shadow_texel_ = -1;
  std::int32_t u_ground_shadow_bounce_ = -1;
  std::int32_t u_ground_headlight_left_ = -1;
  std::int32_t u_ground_headlight_right_ = -1;
  std::int32_t u_ground_headlight_forward_ = -1;
  std::int32_t u_ground_headlight_intensity_ = -1;
  std::int32_t u_ground_traffic_headlight_left_ = -1;
  std::int32_t u_ground_traffic_headlight_right_ = -1;
  std::int32_t u_ground_traffic_headlight_forward_ = -1;
  std::int32_t u_ground_traffic_headlight_intensity_ = -1;
  std::int32_t u_ground_street_count_ = -1;
  std::int32_t u_ground_street_intensity_ = -1;
  std::int32_t u_ground_street_shadows_enabled_ = -1;
  std::array<std::int32_t,kMaxStreetLamps> u_ground_street_shadow_matrix_{};
  std::array<std::int32_t,kMaxStreetLamps> u_ground_street_shadow_sampler_{};
  std::array<std::int32_t,kMaxStreetLamps> u_ground_street_pos_{};
  std::array<std::int32_t,kMaxStreetLamps> u_ground_street_dir_{};

  bool ready_ = false;
};

} // namespace roadmaker::editor
