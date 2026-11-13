// S2.2h -- opt-in hardware-visible QOpenGLWidget frameSwapped benchmark.
// Uses the actual main window, GL renderer, 4x MSAA, overlay path, Qt event
// loop, and Alt+LMB orbit. Runs only against a copy of a saved scene.
#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QTimer>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

#include "app/main_window.hpp"
#include "app/settings.hpp"
#include "viewport/viewport_widget.hpp"

using roadmaker::editor::MainWindow;
using roadmaker::editor::ViewportWidget;
namespace {
double percentile(std::vector<double> a, double q) {
  if(a.empty())return -1.0;
  std::sort(a.begin(),a.end());
  const double f=q*(a.size()-1);
  const auto lo=std::size_t(f);
  return a[lo]+(a[std::min(lo+1,a.size()-1)]-a[lo])*(f-lo);
}
QJsonObject measure(const std::vector<double>& v) {
  double mean=0; int jank=0;
  for(double x:v){mean+=x;if(x>33.33)++jank;}
  mean=v.empty()? -1.0:mean/v.size();
  return {{"samples",int(v.size())},{"mean_ms",mean},
          {"p50_ms",percentile(v,.5)},{"p95_ms",percentile(v,.95)},
          {"p99_ms",percentile(v,.99)},{"worst_ms",percentile(v,1)},
          {"effective_fps",mean>0?1000.0/mean:0.0},
          {"frames_over_33ms",jank}};
}
}
int main(int argc,char** argv) {
  if(argc<4){qCritical()<<"Usage: prawko_qt_fps_probe SCENE OUT.json x|t|y";return 2;}
  QSurfaceFormat format;
  format.setVersion(3,3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSamples(4);
  format.setDepthBufferSize(24);
  QSurfaceFormat::setDefaultFormat(format);
  QApplication app(argc,argv);
  roadmaker::editor::Settings::set_read_only(true);
  MainWindow window(nullptr, false);
  window.resize(1600,900);
  window.show();
  const std::filesystem::path src=std::filesystem::path(argv[1]);
  window.load_file(src);
  auto* viewport=window.viewport();
  viewport->set_textured_rendering(true);
  viewport->set_camera_preset("orbit");
  viewport->set_hints_enabled(true);
  viewport->set_hint(QStringLiteral("PrawkoRoadStudio FPS test — viewport and Qt overlays"));
  if(!viewport->isVisible()){qCritical()<<"Not visible";return 3;}

  QElapsedTimer clock;
  clock.start();
  std::vector<double> intervals;
  qint64 last_swap=-1;
  int frame_count=0;
  int tick=0;
  const QPointF base(viewport->width()*0.5,viewport->height()*0.5);
  auto sendMouse=[&](QEvent::Type type,const QPointF& pos,Qt::MouseButton button,
                     Qt::MouseButtons buttons){
    QMouseEvent event(type,pos,viewport->mapToGlobal(pos.toPoint()),button,buttons,Qt::AltModifier);
    QApplication::sendEvent(viewport,&event);
  };
  QJsonObject result{{"scene",QString::fromLocal8Bit(argv[3])},
                     {"measurement","visible real QOpenGLWidget frameSwapped cadence, Qt events/overlays/compositor"},
                     {"resolution",QString("%1x%2").arg(window.width()).arg(window.height())},
                     {"qt_timer_interval_ms",16},{"warmup_frames",30},{"sampled_frames",180}};
  QJsonArray tests;
  int mode=0;
  QObject::connect(viewport,&QOpenGLWidget::frameSwapped,&app,[&]{
    const qint64 now=clock.nsecsElapsed();
    if(last_swap>=0 && frame_count>=30) {
      intervals.push_back(double(now-last_swap)/1.0e6);
    }
    last_swap=now;
    ++frame_count;
  });
  QTimer driver;
  driver.setTimerType(Qt::PreciseTimer);
  driver.setInterval(16);
  QObject::connect(&driver,&QTimer::timeout,&app,[&]{
    ++tick;
    if(mode==1){
      // The same Alt+LMB navigation gesture as a real pointer orbit.
      const QPointF pos=base+QPointF(40.0*std::sin(tick*0.055),14.0*std::sin(tick*0.034));
      sendMouse(QEvent::MouseMove,pos,Qt::NoButton,Qt::LeftButton);
    }
    viewport->update();
    if(frame_count>=210 || tick>=260){
      if(frame_count<100){
        qCritical()<<"Not enough onscreen frameSwapped signals"<<frame_count;
        driver.stop();app.exit(7);return;
      }
      if(mode==1)sendMouse(QEvent::MouseButtonRelease,base,Qt::LeftButton,Qt::NoButton);
      QJsonObject metric=measure(intervals);
      metric["mode"]=mode==0?"steady_refresh":"alt_lmb_orbit";
      metric["frame_swapped_count"]=frame_count;
      metric["timer_ticks"]=tick;
      tests.append(metric);
      qInfo().noquote()<<QString::fromLocal8Bit(argv[3])
         <<metric["mode"].toString()
         <<"frameSwapped"<<frame_count<<"p50"<<metric["p50_ms"].toDouble()
         <<"p95"<<metric["p95_ms"].toDouble()<<"FPS"<<metric["effective_fps"].toDouble();
      mode++;
      if(mode==2){
        driver.stop();
        result["modes"]=tests;
        QFile output(QString::fromLocal8Bit(argv[2]));
        if(!output.open(QIODevice::WriteOnly)){
          qCritical()<<"Could not write result"<<output.fileName();
          app.exit(5);return;
        }
        output.write(QJsonDocument(result).toJson(QJsonDocument::Indented));
        output.close();
        qInfo()<<"QT_FPS_PROBE_PASS"<<output.fileName();
        app.quit();
        return;
      }
      intervals.clear();
      last_swap=-1;frame_count=0;tick=0;
      sendMouse(QEvent::MouseButtonPress,base,Qt::LeftButton,Qt::LeftButton);
    }
  });
  QTimer::singleShot(300,&app,[&]{
    if(!viewport->context()){qCritical()<<"No GL context";app.exit(6);return;}
    const char* gpu=reinterpret_cast<const char*>(
      viewport->context()->functions()->glGetString(GL_RENDERER));
    result["gl_renderer"]=QString::fromLatin1(gpu ? gpu:"unknown");
    if(!result["gl_renderer"].toString().contains("NVIDIA",Qt::CaseInsensitive)){
      qCritical()<<"Hardware renderer expected"<<result["gl_renderer"].toString();
      app.exit(8);return;
    }
    result["msaa_samples"]=viewport->format().samples();
    driver.start();
  });
  return app.exec();
}
