// SPDX-License-Identifier: Apache-2.0
// Opt-in hardware regression and timing, no user scene or settings writes.
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <QSurfaceFormat>
#include <QDebug>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include "render/gl_functions.hpp"
#include "render/gl_renderer.hpp"
#include "render/scene_builder.hpp"
#include "viewport/camera.hpp"
using namespace roadmaker::editor;
namespace {
void require(bool ok,const char* reason) { if(!ok) throw std::runtime_error(reason); }
void* resolve(const char* name) {
  return reinterpret_cast<void*>(QOpenGLContext::currentContext()->getProcAddress(name));
}
double p95(std::vector<double> values) {
  std::sort(values.begin(),values.end()); return values[values.size()*95/100];
}
}
int main(int argc,char** argv) {
  QApplication app(argc,argv);
  if(argc!=2) return 2;
  const QDir output(QString::fromLocal8Bit(argv[1]));
  try {
    require(!QFile::exists(output.filePath("result.json")),"Refuse to overwrite proof");
    require(QDir().mkpath(output.path()),"Cannot create output directory");
    QSurfaceFormat format;
    format.setVersion(3,3); format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24); format.setSwapInterval(0);
    QOpenGLContext context; context.setFormat(format);
    require(context.create(),"Cannot create GL context");
    QOffscreenSurface surface; surface.setFormat(context.format()); surface.create();
    require(context.makeCurrent(&surface),"Cannot activate GL context");
    auto* f=QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(&context);
    require(f && f->initializeOpenGLFunctions() && gl::load_functions(resolve),"GL loader failed");
    const QString gpu=reinterpret_cast<const char*>(f->glGetString(GL_RENDERER));
    require(!gpu.contains("llvmpipe",Qt::CaseInsensitive) &&
            !gpu.contains("Microsoft",Qt::CaseInsensitive),"Software GL is not a hardware benchmark");
    constexpr int width=1280,height=720;
    QOpenGLFramebufferObjectFormat fbformat;
    fbformat.setAttachment(QOpenGLFramebufferObject::Depth);
    QOpenGLFramebufferObject fb(width,height,fbformat);
    require(fb.isValid() && fb.bind(),"Framebuffer failed");
    QJsonArray measurements;
    QImage reference,reference_moved,reference_shared;
    for(int variant=0;variant<3;++variant) {
      GLRenderer::Tuning tuning;
      tuning.cache_instance_uploads=variant!=0;
      tuning.stabilize_sun_shadows=variant==2;
      tuning.max_anisotropy=variant==2 ? 4.0F : 1.0F;
      GLRenderer renderer(tuning);
      require(renderer.init(),"Renderer init failed");
      Environment env=textured_lighting(); env.shadows_enabled=true;
      env.street_lights.count=4; env.street_lights.intensity=1;
      env.street_lights.cast_shadows=true;
      for(int i=0;i<4;++i) env.street_lights.lamps[i].position={float(i*12-18),0,6};
      renderer.set_environment(env);
      renderer.set_ground(true,-0.05F);
      TextureData texture;
      texture.width=texture.height=256; texture.rgba.resize(256*256*4);
      for(int y=0;y<256;++y) for(int x=0;x<256;++x) {
        const auto at=std::size_t(y*256+x)*4;
        const unsigned char v=((x/4+y/4)%2) ? 175 : 60;
        texture.rgba[at]=texture.rgba[at+1]=texture.rgba[at+2]=v;
        texture.rgba[at+3]=255;
      }
      const auto tex=renderer.upload(texture);
      auto floor=actor_box_mesh();
      // Box top UVs and horizontal scaling exercise grazing-angle filtering.
      floor.uvs.resize(floor.positions.size()/3*2);
      for(std::size_t i=0;i<floor.positions.size()/3;++i) {
        floor.uvs[2*i]=floor.positions[3*i]*40;
        floor.uvs[2*i+1]=floor.positions[3*i+1]*40;
      }
      InstanceData ground;
      ground.model[0]=100; ground.model[5]=100; ground.model[10]=0.05F;
      ground.model[14]=-0.025F;
      std::vector<DrawItem> draw;
      Material ground_mat; ground_mat.base_color=tex; ground_mat.roughness_value=0.8F;
      draw.push_back({.mesh=renderer.upload(floor),.material=ground_mat,
                      .instances=std::span<const InstanceData>(&ground,1)});
      std::vector<InstanceData> instances(1000);
      for(std::size_t i=0;i<instances.size();++i) {
        instances[i].model[12]=float(i%40)*2.0F-40;
        instances[i].model[13]=float(i/40)*2.0F-25;
        instances[i].model[14]=0.7F;
        instances[i].model[0]=instances[i].model[5]=0.35F;
      }
      // Distinct mesh parts share transforms, as imported GLB batches do.
      for(int part=0;part<8;++part) {
        auto box=actor_box_mesh();
        for(std::size_t i=2;i<box.positions.size();i+=3) box.positions[i]+=float(part)*0.13F;
        draw.push_back({.mesh=renderer.upload(box),.instances=instances});
      }
      OrbitCamera camera; camera.set_pose({0,0,0},0.6F,0.12F,24.0F);
      const auto matrices=camera.matrices(float(width)/height);
      for(int i=0;i<15;++i) renderer.render(draw,matrices,width,height);
      f->glFinish(); renderer.reset_statistics();
      std::vector<double> gpu_ms,cpu_ms;
      GLuint query=0; f->glGenQueries(1,&query);
      for(int i=0;i<90;++i) {
        QElapsedTimer timer; timer.start();
        f->glBeginQuery(GL_TIME_ELAPSED,query);
        renderer.render(draw,matrices,width,height);
        f->glEndQuery(GL_TIME_ELAPSED);
        cpu_ms.push_back(double(timer.nsecsElapsed())/1e6);
        GLuint64 ns=0; f->glGetQueryObjectui64v(query,GL_QUERY_RESULT,&ns);
        gpu_ms.push_back(double(ns)/1e6);
      }
      f->glDeleteQueries(1,&query);
      const auto stats=renderer.statistics();
      require(variant==0 ? stats.instance_uploads>0 : stats.instance_uploads==0,
              "Static instance upload regression");
      const QImage image=fb.toImage();
      require(image.save(output.filePath(QString("variant-%1.png").arg(variant))),"Save failed");
      if(variant==0) reference=image;
      if(variant==1) require(image==reference,"Caching changed pixels");
      fb.bind(); renderer.reset_statistics();
      for(auto& instance:instances) instance.model[12]+=3.75F;
      renderer.render(draw,matrices,width,height);
      f->glFinish();
      const QImage moved=fb.toImage();
      require(moved!=image,"In-place movement did not reach the GPU");
      require(variant==0 || renderer.statistics().instance_uploads==8,
              "Changed instances must upload once per mesh across all shadow passes");
      if(variant==0) reference_moved=moved;
      if(variant==1) require(moved==reference_moved,"Cached animation changed pixels");
      // Another draw-item with the SAME mesh but a different span must update.
      fb.bind();
      InstanceData alternate=instances[0]; alternate.model[12]=-1; alternate.model[13]=0;
      draw.push_back({.mesh=draw[1].mesh,.instances=std::span<const InstanceData>(&alternate,1)});
      renderer.render(draw,matrices,width,height);
      f->glFinish();
      const QImage shared=fb.toImage();
      if(variant==0) reference_shared=shared;
      if(variant==1) require(shared==reference_shared,"Shared mesh drew stale transforms");
      fb.bind();
      require(f->glGetError()==GL_NO_ERROR,"OpenGL error");
      measurements.append(QJsonObject{{"variant",variant},{"cache",tuning.cache_instance_uploads},
          {"anisotropy",renderer.texture_anisotropy()},{"gpu_p95_ms",p95(gpu_ms)},
          {"cpu_submit_p95_ms",p95(cpu_ms)},
          {"static_uploads_90_frames",double(stats.instance_uploads)},
          {"static_uploaded_bytes_90_frames",double(stats.instance_bytes)}});
      qInfo()<<"VARIANT"<<variant<<"GPU p95"<<p95(gpu_ms)<<"CPU p95"<<p95(cpu_ms)
             <<"uploads"<<stats.instance_uploads<<"anisotropy"<<renderer.texture_anisotropy();
      renderer.shutdown(); fb.bind();
    }
    QFile file(output.filePath("result.json"));
    require(file.open(QIODevice::WriteOnly),"Cannot save JSON");
    const QJsonObject result{{"gpu",gpu},{"width",width},{"height",height},
      {"instances",1000},{"parts",8},{"samples",90},{"warmup",15},
      {"scope","synthetic scene, real OpenGL, no compositor; GPU elapsed queries and CPU submission"},
      {"pixel_identical_cache",true},{"in_place_animation_pass",true},{"measurements",measurements}};
    file.write(QJsonDocument(result).toJson());
    qInfo()<<"RENDER_QUALITY_HARDWARE_PASS";
    return 0;
  } catch(const std::exception& e) { qCritical()<<e.what();return 1; }
}
