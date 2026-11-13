/*
 * PrawkoRoadStudio S2.2g / temporary reproducible OpenGL GPU profiler.
 * Measures native GLRenderer (same source used by the Qt viewport) under a
 * real hardware OpenGL 3.3 core context. Offscreen: no swap/compositor or
 * QPainter HUD; GPU timer query measures GL commands, not CPU meshing.
 * No scene file or installed app is changed.
 */
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <QSurfaceFormat>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "document/document.hpp"
#include "roadmaker/mesh/mesh_builder.hpp"
#include "render/gl_functions.hpp"
#include "render/gl_renderer.hpp"
#include "render/material_catalog.hpp"
#include "render/scene_builder.hpp"
#include "viewport/camera.hpp"

using namespace roadmaker;
using namespace roadmaker::editor;
namespace {
void* resolve_gl(const char* name) {
  auto* ctx = QOpenGLContext::currentContext();
  return ctx ? reinterpret_cast<void*>(ctx->getProcAddress(name)) : nullptr;
}
double percentile(std::vector<double> v, double p) {
  if (v.empty()) return -1.0;
  std::sort(v.begin(), v.end());
  const double position = p * double(v.size() - 1);
  const auto lo = static_cast<std::size_t>(position);
  const auto hi = std::min(v.size()-1, lo+1);
  return v[lo] + (v[hi]-v[lo])*(position-lo);
}
QJsonObject stats(const std::vector<double>& v) {
  return {{ "count", int(v.size()) }, { "p50_ms", percentile(v,0.50) },
          { "p95_ms", percentile(v,0.95) }, { "min_ms", percentile(v,0) },
          { "max_ms", percentile(v,1) }};
}
struct Pose {
  const char* name;
  std::array<float,3> target;
  float yaw, pitch, distance;
};
// In the very same OpenDRIVE XY frame as the viewport. Eye height:
// target.z + distance*sin(pitch); driver profile is around 1.5 m.
const std::array<Pose,5> poses{{
  {"top", {0,0,0.1f},0.75f,1.51f,30.0f},
  {"oblique", {2,-2,0.2f},0.72f,0.66f,19.0f},
  {"driver", {5,-5,0.80f},-2.35f,0.15f,5.0f},
  {"opposite", {5,-5,0.80f},0.75f,0.15f,5.0f},
  {"macro", {4.5f,-4.5f,0.34f},2.30f,0.12f,2.2f}
}};
}
int main(int argc, char** argv) {
  QApplication app(argc,argv);
  Q_INIT_RESOURCE(resources); // static Qt resource object in editor library
  if (argc < 4) {
    qCritical() << "Usage: prawko_gpu_probe <scene.xodr> <out-dir> <label>";
    return 2;
  }
  const QString scene_path = QString::fromLocal8Bit(argv[1]);
  const QString output_dir = QString::fromLocal8Bit(argv[2]);
  const QString label = QString::fromLocal8Bit(argv[3]);
  if (!QDir().mkpath(output_dir)) return 3;
  QSurfaceFormat fmt;
  fmt.setVersion(3,3);
  fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setDepthBufferSize(24);
  fmt.setStencilBufferSize(8);
  fmt.setSwapInterval(0);
  QOpenGLContext context;
  context.setFormat(fmt);
  if (!context.create()) { qCritical() << "Cannot create OpenGL context"; return 4; }
  QOffscreenSurface surface;
  surface.setFormat(context.format());
  surface.create();
  if (!surface.isValid() || !context.makeCurrent(&surface)) {
    qCritical() << "Cannot make offscreen GL current";return 5;
  }
  auto* functions = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(&context);
  if (!functions || !functions->initializeOpenGLFunctions() || !gl::load_functions(&resolve_gl)) {
    qCritical() << "GL 3.3 core functions unavailable"; return 6;
  }
  const QString vendor = reinterpret_cast<const char*>(functions->glGetString(0x1F00));
  const QString gpu = reinterpret_cast<const char*>(functions->glGetString(0x1F01));
  const QString version = reinterpret_cast<const char*>(functions->glGetString(0x1F02));
  qInfo().noquote() << "HARDWARE" << vendor << gpu << version;
  if (gpu.contains("Microsoft",Qt::CaseInsensitive) || gpu.contains("llvmpipe",Qt::CaseInsensitive)) {
    qCritical() << "Software GL detected — refusing hardware performance claims";return 7;
  }
  QOpenGLFramebufferObjectFormat fbformat;
  fbformat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
  fbformat.setSamples(0);
  constexpr int width = 1280, height = 720;
  QOpenGLFramebufferObject framebuffer(width,height,fbformat);
  if (!framebuffer.isValid() || !framebuffer.bind()) return 8;

  Document document;
  if (!document.load(scene_path.toStdWString())) {
    qCritical() << "Scene load failed" << scene_path; return 9;
  }
  // Pick the centre of the longest REAL corner curb rather than assume X/T/Y
  // use the same quadrant. This keeps driver and macro views on the feature.
  std::array<float,2> focus{5.0f,-5.0f};
  std::size_t longest_curb=0;
  for (const auto& floor : document.mesh().junction_floors) {
    for (const auto& part : floor.details) {
      if (part.name.find("corner curb") == std::string::npos ||
          part.indices.size() <= longest_curb || part.positions.empty()) continue;
      longest_curb=part.indices.size();
      double sx=0,sy=0;
      for (std::size_t v=0;v+2<part.positions.size();v+=3) {
        sx+=part.positions[v];sy+=part.positions[v+1];
      }
      focus={float(sx/(part.positions.size()/3)),
             float(sy/(part.positions.size()/3))};
    }
  }
  auto chosen_poses=poses;
  chosen_poses[2].target={focus[0],focus[1],0.80f};
  chosen_poses[3].target={focus[0],focus[1],0.80f};
  chosen_poses[4].target={focus[0],focus[1],0.34f};
  QJsonObject result{{"scene",label},{"gl_vendor",vendor},{"gl_renderer",gpu},
                     {"gl_version",version},{"width",width},{"height",height},
                     {"samples_per_pose",65},{"warmup_per_pose",15},
                     {"measurement_scope","GLRenderer offscreen with PBR textures/props; no QPainter, swap or compositor"},
                     {"focus_x_m",focus[0]},{"focus_y_m",focus[1]}};
  QJsonArray cases;
  MaterialCatalog catalog;
  for (const bool lod : {false,true}) {
    MeshOptions options;
    options.raised_sidewalks = true;
    options.curb_render_lod = lod;
    QElapsedTimer clock;
    clock.start();
    const NetworkMesh mesh = build_network_mesh(document.network(),options);
    const double cpu_mesh_ms = clock.nsecsElapsed()/1.0e6;
    const Scene scene = build_scene(mesh,&document.network());
    GLRenderer renderer;
    if (!renderer.init()) return 10;
    renderer.set_environment(textured_lighting());
    renderer.set_ground(true,ground_base_z(scene.bounds));
    std::map<std::string,TextureHandle> textures;
    auto tex = [&](const std::string& path)->TextureHandle {
      if (path.empty()) return {};
      if (auto it = textures.find(path);it!=textures.end())return it->second;
      QImage im(QString::fromStdString(path));
      if (im.isNull()) { qWarning() << "missing texture" << path.c_str();return {}; }
      im=im.convertToFormat(QImage::Format_RGBA8888);
      TextureData t;
      t.width=im.width();t.height=im.height();
      t.rgba.resize(std::size_t(t.width)*std::size_t(t.height)*4);
      for(int y=0;y<t.height;++y)std::memcpy(t.rgba.data()+std::size_t(y)*t.width*4,
                                               im.constScanLine(y),std::size_t(t.width)*4);
      TextureHandle handle=renderer.upload(t);
      textures[path]=handle;
      return handle;
    };
    // Upload materials as real GUI does: same resource paths, same UV scale,
    // same OpenGL sampler and shader.
    // Prime textures outside the *mesh-only* upload timer; otherwise the
    // first baseline is slower just because the OS caches JPEG/PNG files.
    QElapsedTimer texture_clock;
    texture_clock.start();
    for (const auto& item : scene.items) {
      if (item.surface != SurfaceKind::Asphalt &&
          item.surface != SurfaceKind::Concrete) continue;
      const char* fallback=item.surface==SurfaceKind::Asphalt ? "asphalt":"concrete";
      const MaterialDef* m=item.material.empty()?nullptr:catalog.find_material(item.material);
      if(!m)m=catalog.find_material(fallback);
      if(m){tex(m->albedo);tex(m->normal);tex(m->roughness);}
    }
    functions->glFinish();
    const double texture_upload_sync_ms=texture_clock.nsecsElapsed()/1.0e6;
    // Warmed, repeated full-mesh uploads. Each trial destroys the previous
    // VAO/VBO/EBO buffers; GL finish is included, and textures remain cached.
    std::vector<double> repeated_mesh_upload_ms;
    for(int trial=0;trial<12;++trial) {
      QElapsedTimer upload_clock;
      upload_clock.start();
      for(const auto& item:scene.items)renderer.upload(item.data);
      for(const auto& batch:scene.prop_batches)
        for(const auto& part:batch.parts)renderer.upload(part);
      for(const auto& face:scene.sign_faces)renderer.upload(face.data);
      functions->glFinish();
      if(trial>=2)repeated_mesh_upload_ms.push_back(upload_clock.nsecsElapsed()/1.0e6);
      renderer.clear_meshes();
    }
    std::vector<RenderMeshHandle> owned_meshes;
    std::vector<DrawItem> draw;
    draw.reserve(scene.items.size()+64);
    clock.restart();
    std::size_t raw_mesh_bytes = 0, mesh_count = 0, triangle_count = 0;
    for (const auto& item : scene.items) {
      RenderMeshHandle h=renderer.upload(item.data);
      owned_meshes.push_back(h);
      ++mesh_count;
      raw_mesh_bytes += item.data.positions.size()*sizeof(float) +
                        item.data.normals.size()*sizeof(float) +
                        item.data.uvs.size()*sizeof(float) +
                        item.data.indices.size()*sizeof(std::uint32_t);
      triangle_count+=item.data.indices.size()/3;
      Material mat;
      if(item.surface != SurfaceKind::Paint &&
         (item.surface == SurfaceKind::Asphalt || item.surface == SurfaceKind::Concrete)) {
        const char* fallback = item.surface == SurfaceKind::Asphalt ? "asphalt" : "concrete";
        const MaterialDef* m = item.material.empty()? nullptr : catalog.find_material(item.material);
        if (!m) m=catalog.find_material(fallback);
        if(m) {
          mat.base_color=tex(m->albedo);
          mat.normal=tex(m->normal);
          mat.roughness=tex(m->roughness);
          mat.uv_scale=m->uv_scale;mat.tint=m->tint;
          mat.roughness_value=m->roughness_value;
          mat.normal_strength=m->normal_strength;
        }
      }
      if(item.surface==SurfaceKind::Paint)mat.unlit=true;
      draw.push_back(DrawItem{.mesh=h,.material=mat});
    }
    std::vector<std::vector<InstanceData>> instances;
    instances.reserve(scene.prop_batches.size());
    for(const auto& batch:scene.prop_batches) {
      auto& list=instances.emplace_back();
      for(const auto& inst:batch.instances)list.push_back(inst.transform);
      for(const auto& part:batch.parts) {
        auto handle=renderer.upload(part);
        owned_meshes.push_back(handle);
        ++mesh_count;
        raw_mesh_bytes += (part.positions.size()+part.normals.size()+part.uvs.size())*4 +
                          part.indices.size()*4;
        triangle_count += part.indices.size()/3;
        draw.push_back(DrawItem{.mesh=handle,.instances=std::span<const InstanceData>(list)});
      }
    }
    for(const auto& sign:scene.sign_faces) {
      auto handle=renderer.upload(sign.data);
      owned_meshes.push_back(handle);
      ++mesh_count;
      draw.push_back(DrawItem{.mesh=handle});
    }
    functions->glFinish(); // includes GL uploads AND textures; not pure GPU DMA timing
    const double upload_sync_ms = clock.nsecsElapsed()/1.0e6;

    QJsonObject item_case{{"lod",lod},{"cpu_mesh_ms",cpu_mesh_ms},
                          {"mesh_upload_sync_ms",upload_sync_ms},
                          {"texture_decode_upload_sync_ms",texture_upload_sync_ms},
                          {"repeated_mesh_upload_sync_ms",stats(repeated_mesh_upload_ms)},
                          {"mesh_bytes_cpu",double(raw_mesh_bytes)},
                          {"mesh_count",int(mesh_count)},{"triangles",double(triangle_count)},
                          {"texture_count",int(textures.size())},
                          {"draw_item_count",int(draw.size())}};
    QJsonArray camera_results;
    for(const auto& pose:chosen_poses) {
      OrbitCamera cam;
      cam.set_pose(pose.target,pose.yaw,pose.pitch,pose.distance);
      const CameraMatrices camera=cam.matrices(float(width)/float(height));
      std::vector<double> gpu_ms, submit_ms, full_sync_ms;
      std::uint32_t query = 0;
      functions->glGenQueries(1,&query);
      for(int run=0;run<80;++run) {
        framebuffer.bind();
        QElapsedTimer timer;
        timer.start();
        functions->glBeginQuery(0x88BF,query); // GL_TIME_ELAPSED (OpenGL 3.3)
        renderer.render(draw,camera,width,height);
        functions->glEndQuery(0x88BF);
        const double submit = timer.nsecsElapsed()/1.0e6;
        // Query result read is a GPU completion fence. Not the display swap.
        std::uint64_t gpu_ns = 0;
        functions->glGetQueryObjectui64v(query,0x8866,&gpu_ns); // GL_QUERY_RESULT
        const double sync = timer.nsecsElapsed()/1.0e6;
        if(run>=15) {
          gpu_ms.push_back(double(gpu_ns)/1.0e6);
          submit_ms.push_back(submit);
          full_sync_ms.push_back(sync);
        }
      }
      functions->glDeleteQueries(1,&query);
      framebuffer.bind();
      renderer.render(draw,camera,width,height);
      functions->glFinish();
      const QImage image=framebuffer.toImage(); // Qt flips GL bottom-up pixels
      const QString filename=QString("%1-%2-%3.png").arg(label,lod?"lod":"base",pose.name);
      if (!image.save(QDir(output_dir).filePath(filename))) {
        qCritical() << "Screenshot failed" << filename;
        return 11;
      }
      QJsonObject per_camera{{"pose",pose.name},{"gpu_draw_ms",stats(gpu_ms)},
                             {"cpu_submit_ms",stats(submit_ms)},
                             {"cpu_until_gpu_done_ms",stats(full_sync_ms)},
                             {"screenshot",filename},
                             {"eye_height_m",cam.matrices(float(width)/height).eye[2]}};
      camera_results.append(per_camera);
      qInfo().noquote() << label << (lod?"LOD":"BASE") << pose.name
        << "GPU p50" << percentile(gpu_ms,.50) << "p95" << percentile(gpu_ms,.95)
        << "CPU submit p50" << percentile(submit_ms,.50);
    }
    item_case["views"]=camera_results;
    cases.append(item_case);
    renderer.shutdown();
  }
  result["cases"]=cases;
  const QByteArray json=QJsonDocument(result).toJson(QJsonDocument::Indented);
  QFile file(QDir(output_dir).filePath(label+"-gpu-benchmark.json"));
  if(!file.open(QIODevice::WriteOnly)||file.write(json)!=json.size())return 12;
  file.close();
  qInfo().noquote() << "GPU_PROBE_PASS" << label << file.fileName();
  framebuffer.release();
  context.doneCurrent();
  return 0;
}
