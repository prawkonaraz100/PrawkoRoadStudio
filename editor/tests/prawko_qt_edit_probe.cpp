// S2.2j: visible Qt viewport with the real Document node-drag preview,
// incremental junction remesh, OpenGL mesh upload and frameSwapped cadence.
// Never saves any scenes: all geometry edits live in RAM and are cancelled.
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QTimer>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>
#include "document/document.hpp"
#include "document/selection_model.hpp"
#include "tools/tool_manager.hpp"
#include "tools/node_drag.hpp"
#include "viewport/viewport_widget.hpp"
#include "roadmaker/road/network.hpp"

using namespace roadmaker;
using namespace roadmaker::editor;
namespace {
double pct(std::vector<double> v, double p) {
  if(v.empty()) return -1.0;
  std::sort(v.begin(),v.end());
  const double f=p*(v.size()-1), l=std::floor(f);
  const auto i=static_cast<std::size_t>(l);
  return v[i]+(v[std::min(i+1,v.size()-1)]-v[i])*(f-l);
}
QJsonObject stats(const std::vector<double>& v) {
  double total=0.0;int long_frame=0;
  for(double x:v){total+=x;if(x>33.33) ++long_frame;}
  const double mean=v.empty()?-1.0:total/v.size();
  return {{"samples",int(v.size())},{"mean_ms",mean},
          {"p50_ms",pct(v,.5)},{"p95_ms",pct(v,.95)},
          {"p99_ms",pct(v,.99)},{"worst_ms",pct(v,1.0)},
          {"effective_fps",mean>0?1000.0/mean:0.0},
          {"frames_over_33ms",long_frame}};
}
}
int main(int argc,char** argv) {
  if(argc<4){qCritical()<<"Usage: prawko_qt_edit_probe SCENE OUT.json NAME";return 2;}
  QSurfaceFormat fmt;
  fmt.setVersion(3,3);fmt.setProfile(QSurfaceFormat::CoreProfile);
  fmt.setDepthBufferSize(24);fmt.setSamples(4);
  QSurfaceFormat::setDefaultFormat(fmt);
  QApplication app(argc,argv);
  Document doc;
  SelectionModel selection(doc);
  ToolManager tools;
  ViewportWidget viewport(doc,selection,tools);
  viewport.resize(1600,900);
  viewport.show();
  const auto loaded=doc.load(std::filesystem::path(argv[1]));
  if(!loaded.has_value()){qCritical()<<"Scene load failed";return 3;}
  viewport.set_textured_rendering(true);
  viewport.set_camera_preset("orbit");
  viewport.set_hints_enabled(true);
  viewport.set_hint("PrawkoRoadStudio edit/remesh FPS probe (in memory)");
  RoadId candidate;
  Waypoint original{};
  std::size_t waypoint=0;
  doc.network().for_each_road([&](RoadId id,const Road& road){
    if(candidate.is_valid() || !road.authoring_waypoints ||
       road.authoring_waypoints->size()<2 || road.junction.is_valid()) return;
    candidate=id;
    waypoint=road.authoring_waypoints->size()-1;
    original=road.authoring_waypoints->at(waypoint);
  });
  if(!candidate.is_valid()){qCritical()<<"No authored non-connector road";return 4;}
  NodeDragState drag{.road=candidate,.index=waypoint,
                     .original=original,.current=original};
  edit::SnapOptions snaps;
  snaps.radius=0.0;snaps.grid.reset();snaps.endpoints=false;snaps.tangent=false;
  int mesh_changes=0;
  QObject::connect(&doc,&Document::mesh_changed,&app,
                   [&](const std::vector<RoadId>&){++mesh_changes;});
  QElapsedTimer clock;clock.start();
  QElapsedTimer operation_timer;
  std::vector<double> frame_times,edit_cpu_ms;
  QJsonArray results;
  qint64 last_swap=-1;
  int mode=0, frames=0,ticks=0,edits_attempted=0,preview_successes=0;
  QObject::connect(&viewport,&QOpenGLWidget::frameSwapped,&app,[&]{
    const qint64 now=clock.nsecsElapsed();
    if(last_swap>=0 && frames>=25)frame_times.push_back(double(now-last_swap)/1e6);
    last_swap=now;++frames;
  });
  QTimer driver;
  driver.setTimerType(Qt::PreciseTimer);driver.setInterval(16);
  QJsonObject summary{{"scene",QString::fromLocal8Bit(argv[3])},
                      {"measurement","visible QOpenGLWidget + real Document preview + junction mesh upload"},
                      {"window","1600x900"},{"timer_ms",16}};
  QObject::connect(&driver,&QTimer::timeout,&app,[&]{
    ++ticks;
    if(mode==1){
      ++edits_attempted;
      operation_timer.start();
      const double phase=ticks*0.047;
      // Same production node drag path as EditNodesTool::mouse_move:
      // begin/update_preview -> remesh -> mesh_changed -> GPU upload in paintGL.
      update_node_drag(doc,drag,snaps,
                       Waypoint{.x=original.x+0.12*std::sin(phase),
                                .y=original.y+0.12*std::cos(phase)});
      edit_cpu_ms.push_back(operation_timer.nsecsElapsed()/1.0e6);
      if(doc.preview_active())++preview_successes;
    }
    viewport.update();
    if(frames>=205 || ticks>=280){
      if(frames<90){qCritical()<<"Too few real frames"<<frames;app.exit(5);return;}
      QJsonObject item=stats(frame_times);
      item["mode"]=mode==0?"baseline_refresh":"node_drag_live_remesh";
      item["swap_signals"]=frames;
      item["timer_ticks"]=ticks;
      item["edit_attempts"]=edits_attempted;
      item["preview_active_count"]=preview_successes;
      item["mesh_changed_signals"]=mesh_changes;
      if(mode==1)item["edit_cpu_ms"]=stats(edit_cpu_ms);
      results.append(item);
      qInfo().noquote()<<item["mode"].toString()<<"FPS"<<item["effective_fps"].toDouble()
       <<"p95"<<item["p95_ms"].toDouble()<<"edit_P95"
       <<(mode==1?pct(edit_cpu_ms,.95):0.0)<<"mesh_signals"<<mesh_changes;
      if(mode==1){
        driver.stop();
        const bool real_preview=doc.preview_active() && mesh_changes>90 &&
                                preview_successes>90 && edits_attempted>90;
        doc.cancel_preview();
        summary["modes"]=results;
        summary["real_edit_verified"]=real_preview;
        QFile out(QString::fromLocal8Bit(argv[2]));
        if(!out.open(QIODevice::WriteOnly)){app.exit(6);return;}
        out.write(QJsonDocument(summary).toJson(QJsonDocument::Indented));out.close();
        qInfo()<<"EDIT_FPS_PROBE"<<(real_preview?"PASS":"FAIL")<<out.fileName();
        app.exit(real_preview?0:7);return;
      }
      mode=1;ticks=0;frames=0;last_swap=-1;frame_times.clear();
    }
  });
  QTimer::singleShot(300,&app,[&]{
    if(!viewport.context()){qCritical()<<"No context";app.exit(8);return;}
    const char* renderer=reinterpret_cast<const char*>(
       viewport.context()->functions()->glGetString(GL_RENDERER));
    summary["gl_renderer"]=QString::fromLatin1(renderer?renderer:"");
    viewport.makeCurrent();
    GLint samples=0, sample_buffers=0;
    viewport.context()->functions()->glGetIntegerv(0x80A9,&samples);
    viewport.context()->functions()->glGetIntegerv(0x80A8,&sample_buffers);
    summary["draw_fbo_samples"]=samples;
    summary["draw_fbo_sample_buffers"]=sample_buffers;
    viewport.doneCurrent();
    driver.start();
  });
  return app.exec();
}
