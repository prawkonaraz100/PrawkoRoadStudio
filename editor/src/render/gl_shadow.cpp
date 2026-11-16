// PrawkoRoadStudio R4: real GL3.3 directional depth shadows, Z-up, instance-aware.
// Mirrors and driver view reuse the same renderer; preserve the caller's Qt FBO.
#include "render/gl_renderer.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "render/gl_functions.hpp"

namespace roadmaker::editor {
namespace {

constexpr const char* kDepthVS = R"(#version 330 core
layout(location=0) in vec3 in_position;
layout(location=2) in vec2 in_uv;
layout(location=3) in vec4 i0;
layout(location=4) in vec4 i1;
layout(location=5) in vec4 i2;
layout(location=6) in vec4 i3;
uniform mat4 u_light;
uniform mat4 u_model;
uniform int u_instancing;
out vec2 v_uv;
void main() {
  mat4 model = u_instancing == 1 ? mat4(i0,i1,i2,i3) : u_model;
  gl_Position = u_light * model * vec4(in_position,1.0);
  v_uv = in_uv;
}
)";
constexpr const char* kDepthFS = R"(#version 330 core
in vec2 v_uv;
uniform int u_mode;
uniform float u_cutoff;
uniform int u_has_color_tex;
uniform sampler2D u_color_tex;
uniform float u_uv_scale;
uniform vec4 u_tint;
uniform vec4 u_color;
void main() {
  if (u_mode == 1) {
    float alpha = u_has_color_tex == 1
        ? texture(u_color_tex, v_uv*u_uv_scale).a*u_tint.a : u_color.a;
    if (alpha < u_cutoff) discard;
  }
}
)";

gl::GLuint compile_shadow_shader(gl::GLenum type, const char* text) {
  const gl::GLuint id=gl::CreateShader(type);
  gl::ShaderSource(id,1,&text,nullptr);
  gl::CompileShader(id);
  gl::GLint ok=0;
  gl::GetShaderiv(id,gl::kCompileStatus,&ok);
  if(ok==0){
    std::array<char,1024> log{};
    gl::GetShaderInfoLog(id,static_cast<gl::GLsizei>(log.size()),nullptr,log.data());
    spdlog::error("R4 shadow shader failed: {}",log.data());
    gl::DeleteShader(id);
    return 0;
  }
  return id;
}
gl::GLuint make_shadow_program() {
  const gl::GLuint vs=compile_shadow_shader(gl::kVertexShader,kDepthVS);
  const gl::GLuint fs=compile_shadow_shader(gl::kFragmentShader,kDepthFS);
  if(vs==0 || fs==0){
    if(vs!=0) gl::DeleteShader(vs);
    if(fs!=0) gl::DeleteShader(fs);
    return 0;
  }
  const auto program=gl::CreateProgram();
  gl::AttachShader(program,vs);
  gl::AttachShader(program,fs);
  gl::LinkProgram(program);
  gl::DeleteShader(vs);
  gl::DeleteShader(fs);
  gl::GLint ok=0;
  gl::GetProgramiv(program,gl::kLinkStatus,&ok);
  if(ok==0){
    std::array<char,1024> log{};
    gl::GetProgramInfoLog(program,static_cast<gl::GLsizei>(log.size()),nullptr,log.data());
    spdlog::error("R4 shadow program failed: {}",log.data());
    gl::DeleteProgram(program);
    return 0;
  }
  return program;
}

using V3=std::array<float,3>;
V3 sub(V3 a,V3 b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
V3 cross(V3 a,V3 b){return {a[1]*b[2]-a[2]*b[1],
                                 a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
float dot(V3 a,V3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V3 unit(V3 v){
  const float len=std::sqrt(std::max(1e-12F,dot(v,v)));
  return {v[0]/len,v[1]/len,v[2]/len};
}
using M4=std::array<float,16>;
M4 multiply(M4 a,M4 b){
  M4 out{};
  for(int c=0;c<4;++c)
    for(int r=0;r<4;++r)
      for(int k=0;k<4;++k)
        out[c*4+r]+=a[k*4+r]*b[c*4+k];
  return out;
}
M4 light_view(V3 light,V3 target){
  const V3 f=unit(sub(target,light));
  const V3 side=unit(cross(f,{0,1,0}));
  const V3 up=cross(side,f);
  return {side[0],up[0],-f[0],0,
          side[1],up[1],-f[1],0,
          side[2],up[2],-f[2],0,
          -dot(side,light),-dot(up,light),dot(f,light),1};
}
M4 light_projection(){
  constexpr float range=90.0F;
  constexpr float near=1.0F;
  constexpr float far=300.0F;
  return {1/range,0,0,0, 0,1/range,0,0,
          0,0,-2/(far-near),0, 0,0,-(far+near)/(far-near),1};
}
constexpr M4 kIdentity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
// 144-degree downward perspective covers the bounded R6b2f light cone.
// OpenGL right-handed depth range [-1,1], Z-up world / camera forward -Z.
M4 street_projection() {
  constexpr float near=0.35F;
  constexpr float far=24.0F;
  const float f=1.0F/std::tan(72.0F*3.14159265358979323846F/180.0F);
  return {f,0,0,0, 0,f,0,0,
          0,0,-(far+near)/(far-near),-1,
          0,0,-2.0F*far*near/(far-near),0};
}

} // namespace

bool GLRenderer::init_shadow_resources() {
  shadow_program_=make_shadow_program();
  if(shadow_program_==0) return false;
  u_depth_light_=gl::GetUniformLocation(shadow_program_,"u_light");
  u_depth_model_=gl::GetUniformLocation(shadow_program_,"u_model");
  u_depth_instancing_=gl::GetUniformLocation(shadow_program_,"u_instancing");
  u_depth_mode_=gl::GetUniformLocation(shadow_program_,"u_mode");
  u_depth_cutoff_=gl::GetUniformLocation(shadow_program_,"u_cutoff");
  u_depth_has_tex_=gl::GetUniformLocation(shadow_program_,"u_has_color_tex");
  u_depth_tex_=gl::GetUniformLocation(shadow_program_,"u_color_tex");
  u_depth_uv_scale_=gl::GetUniformLocation(shadow_program_,"u_uv_scale");
  u_depth_tint_=gl::GetUniformLocation(shadow_program_,"u_tint");
  u_depth_color_=gl::GetUniformLocation(shadow_program_,"u_color");

  gl::GLint former_fbo=0;
  gl::GetIntegerv(gl::kFramebufferBinding,&former_fbo);
  gl::GenTextures(1,&shadow_depth_);
  gl::ActiveTexture(gl::kTexture4);
  gl::BindTexture(gl::kTexture2D,shadow_depth_);
  gl::TexImage2D(gl::kTexture2D,0,gl::kDepthComponent24,kShadowSize,kShadowSize,
                 0,gl::kDepthComponent,gl::kUnsignedInt,nullptr);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureMinFilter,gl::kLinear);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureMagFilter,gl::kLinear);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureWrapS,gl::kClampToEdge);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureWrapT,gl::kClampToEdge);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureCompareMode,gl::kCompareRefToTexture);
  gl::TexParameteri(gl::kTexture2D,gl::kTextureCompareFunc,gl::kLequal);
  gl::BindTexture(gl::kTexture2D,0);
  gl::ActiveTexture(gl::kTexture0);
  gl::GenFramebuffers(1,&shadow_fbo_);
  gl::BindFramebuffer(gl::kFramebuffer,shadow_fbo_);
  gl::FramebufferTexture2D(gl::kFramebuffer,gl::kDepthAttachment,
                           gl::kTexture2D,shadow_depth_,0);
  gl::DrawBuffer(gl::kNone);
  gl::ReadBuffer(gl::kNone);
  const auto completeness=gl::CheckFramebufferStatus(gl::kFramebuffer);
  gl::BindFramebuffer(gl::kFramebuffer,static_cast<gl::GLuint>(former_fbo));
  if(completeness!=gl::kFramebufferComplete){
    spdlog::error("R4 depth-only framebuffer incomplete {}",completeness);
    release_shadow_resources();
    return false;
  }
  // R6b2f: four extra depth-only targets, lazily used ONLY if streetlights
  // opted into shadows. Failure does not corrupt the existing directional map.
  street_shadow_ready_=true;
  gl::GetIntegerv(gl::kFramebufferBinding,&former_fbo);
  for (std::size_t i=0; i<kMaxStreetLamps; ++i) {
    gl::GenTextures(1,&street_shadow_depth_[i]);
    gl::ActiveTexture(gl::kTexture0+static_cast<gl::GLenum>(5+i));
    gl::BindTexture(gl::kTexture2D,street_shadow_depth_[i]);
    gl::TexImage2D(gl::kTexture2D,0,gl::kDepthComponent24,
                   kStreetShadowSize,kStreetShadowSize,0,
                   gl::kDepthComponent,gl::kUnsignedInt,nullptr);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureMinFilter,gl::kLinear);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureMagFilter,gl::kLinear);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureWrapS,gl::kClampToEdge);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureWrapT,gl::kClampToEdge);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureCompareMode,gl::kCompareRefToTexture);
    gl::TexParameteri(gl::kTexture2D,gl::kTextureCompareFunc,gl::kLequal);
    gl::GenFramebuffers(1,&street_shadow_fbo_[i]);
    gl::BindFramebuffer(gl::kFramebuffer,street_shadow_fbo_[i]);
    gl::FramebufferTexture2D(gl::kFramebuffer,gl::kDepthAttachment,
                             gl::kTexture2D,street_shadow_depth_[i],0);
    gl::DrawBuffer(gl::kNone);
    gl::ReadBuffer(gl::kNone);
    if (gl::CheckFramebufferStatus(gl::kFramebuffer)!=gl::kFramebufferComplete) {
      spdlog::error("R6b2f streetlight shadow depth FBO {} incomplete",i);
      street_shadow_ready_=false;
      break;
    }
  }
  gl::BindFramebuffer(gl::kFramebuffer,static_cast<gl::GLuint>(former_fbo));
  gl::ActiveTexture(gl::kTexture0);
  gl::BindTexture(gl::kTexture2D,0);
  if(street_shadow_ready_) spdlog::info("R6b2f shadow maps READY: {} at {}x{}",
                                        kMaxStreetLamps,kStreetShadowSize,kStreetShadowSize);
  spdlog::info("R4 depth shadow map READY: {}x{}",kShadowSize,kShadowSize);
  return true;
}

void GLRenderer::release_shadow_resources() {

  for (std::size_t i=0;i<kMaxStreetLamps;++i){
    if(street_shadow_fbo_[i]!=0) gl::DeleteFramebuffers(1,&street_shadow_fbo_[i]);
    if(street_shadow_depth_[i]!=0) gl::DeleteTextures(1,&street_shadow_depth_[i]);
    street_shadow_fbo_[i]=street_shadow_depth_[i]=0;
  }
  street_shadow_ready_=false;
  street_shadow_count_=0;
  if(shadow_fbo_!=0) gl::DeleteFramebuffers(1,&shadow_fbo_);
  if(shadow_depth_!=0) gl::DeleteTextures(1,&shadow_depth_);
  if(shadow_program_!=0) gl::DeleteProgram(shadow_program_);
  shadow_fbo_=shadow_depth_=shadow_program_=0;
  shadow_ready_=shadow_active_=false;
}

void GLRenderer::draw_depth_casters(const std::vector<DrawItem>& items) {
  for(const auto& item:items){
    const auto found=meshes_.find(item.mesh.id);
    if(found==meshes_.end()) continue;
    GpuMesh& mesh=found->second;
    if(mesh.kind!=PrimitiveKind::Triangles || item.material.unlit ||
       mesh.alpha_mode==2 || (mesh.alpha_mode==0 && mesh.color[3]<0.999F)) continue;
    const auto texit=textures_.find(item.material.base_color.id);
    const std::uint32_t tex=texit==textures_.end()?0:texit->second;
    gl::Uniform1i(u_depth_has_tex_,tex!=0?1:0);
    gl::Uniform1i(u_depth_mode_,mesh.alpha_mode);
    gl::Uniform1f(u_depth_cutoff_,mesh.alpha_cutoff);
    gl::Uniform1f(u_depth_uv_scale_,item.material.uv_scale);
    gl::Uniform4f(u_depth_tint_,item.material.tint[0],item.material.tint[1],
                  item.material.tint[2],item.material.tint[3]);
    gl::Uniform4f(u_depth_color_,mesh.color[0],mesh.color[1],mesh.color[2],mesh.color[3]);
    gl::ActiveTexture(gl::kTexture0);
    gl::BindTexture(gl::kTexture2D,tex);
    gl::BindVertexArray(mesh.vao);
    if(item.instances.empty()){
      gl::Uniform1i(u_depth_instancing_,0);
      gl::UniformMatrix4fv(u_depth_model_,1,0,kIdentity.data());
      gl::DrawElements(gl::kTriangles,mesh.index_count,gl::kUnsignedInt,nullptr);
    } else {
      prepare_instances(mesh, item.instances);
      gl::Uniform1i(u_depth_instancing_,1);
      gl::DrawElementsInstanced(gl::kTriangles,mesh.index_count,
                               gl::kUnsignedInt,nullptr,
                               static_cast<gl::GLsizei>(item.instances.size()));
    }
  }
}

void GLRenderer::draw_shadow_map(const std::vector<DrawItem>& items,
                                 const CameraMatrices& camera,int width,int height) {
  shadow_active_=false;
  if(!shadow_ready_ || !environment_.shadows_enabled ||
     environment_.sun_intensity<0.10F) return;
  const V3 sun=unit(environment_.sun_dir);
  // A bounded light volume follows each camera, including the driver and all
  // three mirror FBO views; no assumption that the scene lives near (0,0).
  const V3 target{camera.eye[0]-camera.view[2]*24.0F,
                  camera.eye[1]-camera.view[6]*24.0F,
                  ground_base_z_};
  const V3 light{target[0]+sun[0]*110.0F,
                 target[1]+sun[1]*110.0F,
                 target[2]+sun[2]*110.0F};
  shadow_light_matrix_=multiply(light_projection(),light_view(light,target));
  if (tuning_.stabilize_sun_shadows)
    shadow_light_matrix_ = stabilize_shadow_projection(shadow_light_matrix_, kShadowSize);

  gl::GLint original_fbo=0;
  gl::GetIntegerv(gl::kFramebufferBinding,&original_fbo);
  gl::BindFramebuffer(gl::kFramebuffer,shadow_fbo_);
  gl::Viewport(0,0,kShadowSize,kShadowSize);
  gl::Enable(gl::kDepthTest);
  gl::DepthFunc(gl::kLess);
  gl::DepthMask(1);
  gl::Disable(gl::kBlend);
  gl::Clear(gl::kDepthBufferBit);
  gl::Enable(gl::kPolygonOffsetFill);
  gl::PolygonOffset(1.2F,2.0F);
  gl::UseProgram(shadow_program_);
  gl::UniformMatrix4fv(u_depth_light_,1,0,shadow_light_matrix_.data());
  gl::Uniform1i(u_depth_tex_,0);

  draw_depth_casters(items);
  gl::BindVertexArray(0);
  gl::UseProgram(0);
  gl::Disable(gl::kPolygonOffsetFill);
  // Qt uses a nonzero default FBO, and mirrors use other FBOs: never bind 0.
  gl::BindFramebuffer(gl::kFramebuffer,static_cast<gl::GLuint>(original_fbo));
  gl::Viewport(0,0,width,height);
  shadow_active_=true;
}
void GLRenderer::draw_street_shadow_maps(const std::vector<DrawItem>& items,
                                         int width,int height) {
  street_shadow_count_=0;
  const auto& streets=environment_.street_lights;
  if (!shadow_ready_ || !street_shadow_ready_ ||
      !streets.cast_shadows || streets.count<=0 || streets.intensity<=0.0F)
    return;
  gl::GLint original_fbo=0;
  gl::GetIntegerv(gl::kFramebufferBinding,&original_fbo);
  const int count=std::min(streets.count,static_cast<int>(kMaxStreetLamps));
  for(int i=0;i<count;++i) {
    const auto& lamp=streets.lamps[static_cast<std::size_t>(i)];
    const V3 start=lamp.position;
    const V3 finish{start[0]+6.0F*lamp.direction[0],
                    start[1]+6.0F*lamp.direction[1],
                    start[2]+6.0F*lamp.direction[2]};
    street_shadow_matrix_[static_cast<std::size_t>(i)]=
       multiply(street_projection(),light_view(start,finish));
    gl::BindFramebuffer(gl::kFramebuffer,street_shadow_fbo_[static_cast<std::size_t>(i)]);
    gl::Viewport(0,0,kStreetShadowSize,kStreetShadowSize);
    gl::Enable(gl::kDepthTest);
    gl::DepthFunc(gl::kLess);
    gl::DepthMask(1);
    gl::Disable(gl::kBlend);
    gl::Clear(gl::kDepthBufferBit);
    gl::Enable(gl::kPolygonOffsetFill);
    gl::PolygonOffset(1.15F,2.1F);
    gl::UseProgram(shadow_program_);
    gl::UniformMatrix4fv(u_depth_light_,1,0,
            street_shadow_matrix_[static_cast<std::size_t>(i)].data());
    gl::Uniform1i(u_depth_tex_,0);
    draw_depth_casters(items);
    gl::BindVertexArray(0);
    gl::UseProgram(0);
    gl::Disable(gl::kPolygonOffsetFill);
    ++street_shadow_count_;
  }
  gl::BindFramebuffer(gl::kFramebuffer,static_cast<gl::GLuint>(original_fbo));
  gl::Viewport(0,0,width,height);
}

} // namespace roadmaker::editor
