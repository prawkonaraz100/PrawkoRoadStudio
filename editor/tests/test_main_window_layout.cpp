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

// Structural guards for the chrome layout (issue #332): nothing whose width
// depends on the active tool may live in a toolbar, or the tool buttons shift
// under the cursor on every tool switch. The per-tool instruction belongs to
// the status bar's left section, where it elides instead of resizing the bar.
//
// The window is constructed but NEVER shown: ViewportWidget's GL init is
// fatal-on-failure and only runs on realize, so offscreen tests must stay
// construction-only (this is why --screenshot mode probes GL first).

#include "roadmaker/xodr/reader.hpp"
#include "roadmaker/xodr/writer.hpp"

#include <gtest/gtest.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>
#include <QWidgetAction>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>

#include "app/elided_label.hpp"
#include "app/main_window.hpp"
#include "app/settings.hpp"
#include "app/shortcut_registry.hpp"
#include "app/welcome_widget.hpp"
#include "document/asset_import.hpp"
#include "document/library_list_model.hpp"
#include "document/project.hpp"
#include "document/scene_sidecar.hpp"
#include "panels/editor2d_host.hpp"
#include "panels/library_panel.hpp"
#include "viewport/projection.hpp"
#include "viewport/viewport_widget.hpp"

namespace roadmaker::editor {
namespace {

class MainWindowLayoutTest : public ::testing::Test {
protected:
  MainWindow window_{nullptr, /*restore_saved_layout=*/false};
};

TEST_F(MainWindowLayoutTest, StudioLibraryOpensTheNewRoadTemplatesWithoutUserSetup) {
  window_.open_studio_library();
  auto* dock = window_.findChild<QDockWidget*>(QStringLiteral("dock.library"));
  auto* category = window_.findChild<QComboBox*>(QStringLiteral("library_category"));
  ASSERT_NE(dock, nullptr);
  ASSERT_NE(category, nullptr);
  EXPECT_FALSE(dock->isHidden()); // window stays un-realized in headless CI
  EXPECT_EQ(category->currentData().toString(), QStringLiteral("Prawko | Gotowe drogi"));
}

TEST_F(MainWindowLayoutTest, ToolbarsHoldNoVariableWidthLabels) {
  const auto bars = window_.findChildren<QToolBar*>();
  ASSERT_FALSE(bars.isEmpty());
  for (const QToolBar* bar : bars) {
    for (const QLabel* label : bar->findChildren<QLabel*>()) {
      // The one allowed label is the fixed "Template:" caption, which is shown
      // and hidden wholesale with the dropdown it labels.
      EXPECT_EQ(label->objectName(), QStringLiteral("toolOptionCaption"))
          << "toolbar " << bar->objectName().toStdString() << " holds label '"
          << label->text().toStdString() << "'";
    }
  }
}

TEST_F(MainWindowLayoutTest, ScenarioModeShowsRouteAndRunOnTheOnlyPrimaryToolbar) {
  auto* mode = window_.findChild<QAction*>(QStringLiteral("scenario_mode"));
  auto* main = window_.findChild<QToolBar*>(QStringLiteral("toolbar.main"));
  auto* run = window_.findChild<QAction*>(QStringLiteral("run_scenario_simulation"));
  auto* more = window_.findChild<QMenu*>(QStringLiteral("menu.advanced_tools"));
  auto* storyboard_dock = window_.findChild<QDockWidget*>(QStringLiteral("dock.editor2d"));
  auto* editor = window_.findChild<Editor2DHost*>();
  auto* actor_kind = window_.findChild<QComboBox*>(QStringLiteral("actor_kind"));
  auto* options = window_.findChild<QToolBar*>(QStringLiteral("toolbar.options"));
  ASSERT_NE(mode, nullptr);
  ASSERT_NE(main, nullptr);
  ASSERT_NE(run, nullptr);
  ASSERT_NE(more, nullptr);
  ASSERT_NE(storyboard_dock, nullptr);
  ASSERT_NE(editor, nullptr);
  ASSERT_NE(actor_kind, nullptr);
  ASSERT_NE(options, nullptr);
  QWidgetAction* actor_kind_action = nullptr;
  for (QAction* action : options->actions()) {
    auto* widget_action = qobject_cast<QWidgetAction*>(action);
    if (widget_action && widget_action->defaultWidget() == actor_kind) {
      actor_kind_action = widget_action;
      break;
    }
  }
  ASSERT_NE(actor_kind_action, nullptr);
  ASSERT_TRUE(main->actions().contains(mode));
  EXPECT_EQ(main->actions().indexOf(run), main->actions().indexOf(mode) + 1);
  EXPECT_FALSE(run->isEnabled());

  auto* actor = static_cast<QAction*>(nullptr);
  for (QMenu* category : more->findChildren<QMenu*>()) {
    for (QAction* action : category->actions()) {
      if (action->iconText() == QStringLiteral("Actor"))
        actor = action;
    }
  }
  ASSERT_NE(actor, nullptr);
  // Do not depend on order of unrelated core controls.
  QAction* road_action = nullptr;
  QAction* route_action = nullptr;
  for (QAction* action : main->actions()) {
    if (action->iconText() == QStringLiteral("Droga"))
      road_action = action;
    if (action->iconText() == QStringLiteral("Trasa"))
      route_action = action;
  }
  ASSERT_NE(road_action, nullptr);
  ASSERT_NE(route_action, nullptr);
  EXPECT_TRUE(road_action->isVisible());
  EXPECT_FALSE(route_action->isVisible());
  EXPECT_FALSE(actor->isVisible());

  mode->trigger();
  EXPECT_TRUE(mode->isChecked());
  EXPECT_FALSE(road_action->isVisible());
  EXPECT_TRUE(route_action->isVisible());
  EXPECT_TRUE(route_action->isEnabled());
  EXPECT_TRUE(actor->isVisible());
  EXPECT_TRUE(actor->isEnabled());
  auto* category = window_.findChild<QComboBox*>(QStringLiteral("library_category"));
  ASSERT_NE(category, nullptr);
  EXPECT_EQ(category->currentData().toString(), QStringLiteral("Uczestnicy ruchu"));

  auto* scenario_menu = window_.menuBar()->findChild<QMenu*>(QStringLiteral("scenario_menu"));
  ASSERT_NE(scenario_menu, nullptr);
  const auto scenario_actions = scenario_menu->actions();
  const auto found = std::ranges::find_if(scenario_actions, [](const QAction* action) {
    return action->text() == QStringLiteral("Storyboard…");
  });
  ASSERT_NE(found, scenario_actions.end());
  (*found)->trigger();
  EXPECT_FALSE(storyboard_dock->isHidden());
  EXPECT_EQ(editor->current_title(), QStringLiteral("Storyboard"));

  actor->trigger();
  EXPECT_TRUE(actor_kind_action->isVisible());
  EXPECT_GE(actor_kind->count(), 2);

  mode->trigger();
  EXPECT_FALSE(mode->isChecked());
  EXPECT_TRUE(road_action->isVisible());
  EXPECT_FALSE(route_action->isVisible());
  EXPECT_FALSE(actor->isVisible());
  EXPECT_FALSE(actor_kind_action->isVisible());
}

TEST_F(MainWindowLayoutTest, LibraryMimeDropAndViewportClicksWriteBothKindsOfRoute) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto scene = std::filesystem::path(directory.path().toStdU16String()) / "practice.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "prawko_crosswalk_clear.xodr",
                             scene);
  const auto state_path = directory.filePath(QStringLiteral("practice.rmscene.json"));
  {
    std::ofstream state(std::filesystem::path(state_path.toStdU16String()));
    state
        << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  window_.load_file(scene);
  auto* run = window_.findChild<QAction*>(QStringLiteral("run_scenario_simulation"));
  ASSERT_NE(run, nullptr);
  EXPECT_FALSE(run->isEnabled());
  auto* mode = window_.findChild<QAction*>(QStringLiteral("scenario_mode"));
  ASSERT_NE(mode, nullptr);
  mode->setChecked(true);
  auto* viewport = window_.viewport();
  viewport->resize(800, 500);
  viewport->set_camera_preset(QStringLiteral("ortho"));
  const auto pixel = [viewport](double x, double y) {
    const double w = viewport->width();
    const double h = viewport->height();
    const auto projected =
        project_to_screen(viewport->camera().matrices(float(w / h)), x, y, 0.0, w, h);
    return projected ? QPointF((*projected)[0], (*projected)[1]).toPoint() : QPoint(-1, -1);
  };
  const auto drop = [viewport, &pixel](const QByteArray& key, double x, double y) {
    const QPoint pos = pixel(x, y);
    ASSERT_TRUE(viewport->rect().contains(pos)) << pos.x() << ", " << pos.y();
    QMimeData mime;
    mime.setData(QString::fromLatin1(kLibraryItemMimeType), key);
    QDragEnterEvent enter(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &enter);
    ASSERT_TRUE(enter.isAccepted());
    QDropEvent event(QPointF(pos), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &event);
    ASSERT_TRUE(event.isAccepted());
  };
  auto* finish = window_.findChild<QPushButton*>(QStringLiteral("finish_actor_route"));
  ASSERT_NE(finish, nullptr);
  auto* toolbar = window_.findChild<QToolBar*>(QStringLiteral("toolbar.options"));
  ASSERT_NE(toolbar, nullptr);
  const auto actions = toolbar->actions();
  const auto finish_action = std::ranges::find_if(actions, [finish](QAction* action) {
    const auto* widget = qobject_cast<QWidgetAction*>(action);
    return widget && widget->defaultWidget() == finish;
  });
  ASSERT_NE(finish_action, actions.end());
  EXPECT_FALSE((*finish_action)->isVisible());
  drop("actor.car", 42.0, -1.75);
  ASSERT_TRUE((*finish_action)->isVisible());
  EXPECT_TRUE(run->isEnabled());
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(65.0, -1.75));
  finish->click();
  drop("actor.pedestrian", 58.0, -6.1);
  ASSERT_TRUE((*finish_action)->isVisible());
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(58.0, 5.1));
  finish->click();
  EXPECT_FALSE((*finish_action)->isVisible());
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(43.0, -1.75));
  auto* route_toolbar = window_.findChild<QToolBar*>(QStringLiteral("toolbar.main"));
  ASSERT_NE(route_toolbar, nullptr);
  const auto tools = route_toolbar->actions();
  const auto route = std::ranges::find_if(
      tools, [](QAction* action) { return action->iconText() == QStringLiteral("Trasa"); });
  ASSERT_NE(route, tools.end());
  auto* edit_route = window_.findChild<QPushButton*>(QStringLiteral("actor_edit_route_button"));
  ASSERT_NE(edit_route, nullptr);
  ASSERT_TRUE(edit_route->isEnabled());
  edit_route->click(); // same tool as the toolbar, from the selected participant's inspector
  EXPECT_TRUE((*finish_action)->isVisible());
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(80.0, -1.75));
  finish->click();
  const auto menus = window_.menuBar()->actions();
  const auto save = menus.front()->menu()->actions();
  const auto save_action = std::ranges::find_if(
      save, [](QAction* action) { return action->text() == QStringLiteral("&Save"); });
  ASSERT_NE(save_action, save.end());
  (*save_action)->trigger();
  auto xosc = scene;
  xosc.replace_extension(".xosc");
  std::ifstream stream(xosc, std::ios::binary);
  ASSERT_TRUE(stream);
  const std::string xml{std::istreambuf_iterator<char>(stream), {}};
  EXPECT_NE(xml.find("<AssignRouteAction>"), std::string::npos);
  EXPECT_NE(xml.find("<FollowTrajectoryAction>"), std::string::npos);
  const auto first_waypoint = xml.find("<Waypoint ");
  ASSERT_NE(first_waypoint, std::string::npos);
  const auto second_waypoint = xml.find("<Waypoint ", first_waypoint + 1);
  ASSERT_NE(second_waypoint, std::string::npos);
  EXPECT_NE(xml.find("<Waypoint ", second_waypoint + 1), std::string::npos);
}

// Reproduces the reported "only one road can be dropped" issue at the real
// Qt viewport boundary. In particular the second drop occurs while Create Road
// is already checked; ToolManager will NOT re-activate the same tool.
TEST_F(MainWindowLayoutTest, RepeatedRoadLibraryDropsAuthorMultipleIndependentRoads) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto scene = std::filesystem::path(directory.path().toStdU16String()) / "roads.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "prawko_crosswalk_clear.xodr",
                             scene);
  {
    std::ofstream state(std::filesystem::path(
        directory.filePath(QStringLiteral("roads.rmscene.json")).toStdU16String()));
    state
        << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  window_.load_file(scene);
  auto* viewport = window_.viewport();
  viewport->resize(800, 500);
  viewport->set_camera_preset(QStringLiteral("ortho"));
  const auto pixel = [viewport](double x, double y) {
    const double w = viewport->width();
    const double h = viewport->height();
    const auto projected =
        project_to_screen(viewport->camera().matrices(float(w / h)), x, y, 0.0, w, h);
    return projected ? QPointF((*projected)[0], (*projected)[1]).toPoint() : QPoint(-100, -100);
  };
  const auto drop = [viewport, &pixel](const QByteArray& key, double x, double y) {
    const QPoint pos = pixel(x, y);
    ASSERT_TRUE(viewport->rect().contains(pos)) << pos.x() << ", " << pos.y();
    QMimeData mime;
    mime.setData(QString::fromLatin1(kLibraryItemMimeType), key);
    QDragEnterEvent enter(pos, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &enter);
    ASSERT_TRUE(enter.isAccepted());
    QDropEvent event(QPointF(pos), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &event);
    ASSERT_TRUE(event.isAccepted());
  };
  const auto read_count = [&scene]() {
    std::ifstream stream(scene, std::ios::binary);
    const std::string xml{std::istreambuf_iterator<char>(stream), {}};
    std::size_t count = 0, pos = 0;
    while ((pos = xml.find("<road ", pos)) != std::string::npos) {
      ++count;
      pos += 6;
    }
    return count;
  };
  auto* save = window_.findChild<QAction*>(QStringLiteral("file_save"));
  if (!save) {
    const auto actions = window_.menuBar()->actions().front()->menu()->actions();
    const auto match = std::ranges::find_if(
        actions, [](const QAction* action) { return action->text() == QStringLiteral("&Save"); });
    ASSERT_NE(match, actions.end());
    save = *match;
  }
  ASSERT_NE(save, nullptr);
  auto* footer = window_.findChild<QLabel*>(QStringLiteral("status_entities"));
  ASSERT_NE(footer, nullptr);
  EXPECT_TRUE(footer->text().startsWith(QStringLiteral("1 roads"))) << footer->text().toStdString();
  EXPECT_EQ(read_count(), 1U);
  // First select the existing road to catch the accidental extend gesture.
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(50.0, 0.0));

  drop("road.prawko.urban", 28.0, 24.0);
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(78.0, 24.0));
  QTest::keyClick(viewport, Qt::Key_Return);
  EXPECT_TRUE(footer->text().startsWith(QStringLiteral("2 roads")))
      << "The status bar showed the stale 1-road count after adding road 2";
  save->trigger();
  EXPECT_EQ(read_count(), 2U) << "The first Library drop did not create another road";

  // If Scenario mode was enabled between placements, a road template must
  // return to Map mode and activate Create Road rather than silently refuse.
  auto* scenario_mode = window_.findChild<QAction*>(QStringLiteral("scenario_mode"));
  ASSERT_NE(scenario_mode, nullptr);
  scenario_mode->setChecked(true);
  ASSERT_TRUE(scenario_mode->isChecked());
  drop("road.prawko.wide", 28.0, -24.0);
  EXPECT_FALSE(scenario_mode->isChecked());
  QTest::mouseClick(viewport, Qt::LeftButton, Qt::NoModifier, pixel(78.0, -24.0));
  QTest::keyClick(viewport, Qt::Key_Return);
  EXPECT_TRUE(footer->text().startsWith(QStringLiteral("3 roads")))
      << "The status bar showed a stale road count after adding road 3";
  save->trigger();
  EXPECT_EQ(read_count(), 3U) << "A second template drop didn't add road 3";
  // The same footer must follow Undo and Redo, not only fresh additions.
  const auto menus = window_.menuBar()->actions();
  ASSERT_GE(menus.size(), 2);
  QMenu* edit_menu = menus[1]->menu();
  ASSERT_NE(edit_menu, nullptr);
  const auto edit_actions = edit_menu->actions();
  const auto undo = std::ranges::find_if(edit_actions, [](const QAction* action) {
    return action->text().startsWith(QStringLiteral("&Undo"));
  });
  const auto redo = std::ranges::find_if(edit_actions, [](const QAction* action) {
    return action->text().startsWith(QStringLiteral("&Redo"));
  });
  ASSERT_NE(undo, edit_actions.end());
  ASSERT_NE(redo, edit_actions.end());
  (*undo)->trigger();
  EXPECT_TRUE(footer->text().startsWith(QStringLiteral("2 roads")));
  (*redo)->trigger();
  EXPECT_TRUE(footer->text().startsWith(QStringLiteral("3 roads")));
}

// End-to-end mouse-boundary regression with our real GLB from Blender,
// imported into a temporary project using the PRODUCT importer, not a mock.
// Drop a Library tile onto the Qt viewport, edit pose through Properties,
// save XODR and confirm the reopened scene still owns this model.
TEST_F(MainWindowLayoutTest, BlenderBarrierLibraryDropAndPropertiesMoveRotateSave) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  // Opt-in forensic capture of this test's ACTUAL serialized editor output.
  // CI and ordinary runs still clean up the temporary project as usual.
  if (qEnvironmentVariableIsSet("PRAWKO_KEEP_BLENDER_TEST")) {
    directory.setAutoRemove(false);
  }
  const auto root = std::filesystem::path(directory.path().toStdU16String());
  ASSERT_TRUE(Project::create(root, QStringLiteral("Prawko GLB proof")).has_value());
  const auto scene = root / "bariera.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "straight_road.xodr", scene);
  {
    std::ofstream state(root / "bariera.rmscene.json");
    state
        << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  AssetImportRequest request;
  request.source =
      std::filesystem::path(RM_ASSETS_DIR) / "prawko/blender/prawko_bariera_drogowa.glb";
  request.label = QStringLiteral("Bariera ostrzegawcza Prawko");
  request.category = QStringLiteral("Prawko | Obiekty 3D");
  request.license = QStringLiteral("Model własny PrawkoNaRaz");
  auto imported = import_prop_asset(root, request);
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  LibraryManifest overlay;
  overlay.upsert(std::get<0>(*imported));
  const auto manifest = root / "assets/library/manifest.json";
  std::filesystem::create_directories(manifest.parent_path());
  ASSERT_TRUE(overlay.save(manifest).has_value());

  window_.load_file(scene);
  auto* library = window_.findChild<LibraryPanel*>();
  ASSERT_NE(library, nullptr);
  // A saved Scenario scene must not conceal project GLBs behind the traffic
  // actor category on launch; the first visible tiles must be the 3D models.
  EXPECT_EQ(library->category_combo()->currentData().toString(),
            QStringLiteral("Prawko | Obiekty 3D"));
  const auto* proxy = library->view()->model();
  ASSERT_NE(proxy, nullptr);
  const int models_category =
      library->category_combo()->findData(QStringLiteral("Prawko | Obiekty 3D"));
  ASSERT_GE(models_category, 0);
  auto* add_menu = window_.findChild<QMenu*>(QStringLiteral("menu.prawko_library"));
  ASSERT_NE(add_menu, nullptr);
  ASSERT_EQ(add_menu->actions().size(), 4);
  EXPECT_TRUE(add_menu->actions()[3]->isVisible());
  add_menu->actions()[3]->trigger();
  EXPECT_EQ(library->category_combo()->currentData().toString(),
            QStringLiteral("Prawko | Obiekty 3D"));
  library->category_combo()->setCurrentIndex(models_category);
  ASSERT_EQ(proxy->rowCount(), 1);
  const QModelIndex tile = proxy->index(0, 0);
  const QByteArray key = proxy->data(tile, LibraryListModel::KeyRole).toByteArray();
  EXPECT_EQ(key, QByteArray("prop.bariera_ostrzegawcza_prawko"));

  auto* viewport = window_.viewport();
  viewport->resize(800, 500);
  viewport->set_camera_preset(QStringLiteral("ortho"));
  const double w = viewport->width(), h = viewport->height();
  const auto projected =
      project_to_screen(viewport->camera().matrices(float(w / h)), 44.0, -5.0, 0.0, w, h);
  ASSERT_TRUE(projected.has_value());
  const QPoint pos = QPointF((*projected)[0], (*projected)[1]).toPoint();
  ASSERT_TRUE(viewport->rect().contains(pos));
  std::unique_ptr<QMimeData> mime(proxy->mimeData({tile}));
  ASSERT_NE(mime, nullptr);
  QDragEnterEvent enter(pos, Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(viewport, &enter);
  ASSERT_TRUE(enter.isAccepted());
  QDropEvent drop(QPointF(pos), Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(viewport, &drop);
  ASSERT_TRUE(drop.isAccepted());

  // The drop selected the newly created object. Heading and along-road
  // position are user-editable, not just internal fields.
  auto* heading = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_hdg_spin"));
  auto* station = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_s_spin"));
  ASSERT_NE(heading, nullptr);
  ASSERT_NE(station, nullptr);
  const double old_s = station->value();
  EXPECT_GT(old_s, 20.0);
  station->setValue(old_s + 6.0);
  QMetaObject::invokeMethod(station, "editingFinished", Qt::DirectConnection);
  heading->setValue(1.571);
  QMetaObject::invokeMethod(heading, "editingFinished", Qt::DirectConnection);

  auto* save = window_.findChild<QAction*>(QStringLiteral("file_save"));
  if (save == nullptr) {
    const auto file_actions = window_.menuBar()->actions().front()->menu()->actions();
    const auto it = std::ranges::find_if(file_actions, [](const QAction* action) {
      return action->text() == QStringLiteral("&Save");
    });
    ASSERT_NE(it, file_actions.end());
    save = *it;
  }
  save->trigger();
  const auto reloaded = load_xodr(scene);
  ASSERT_TRUE(reloaded.has_value()) << reloaded.error().message;
  int count = 0;
  reloaded->network.for_each_object([&](ObjectId, const Object& object) {
    ++count;
    EXPECT_EQ(object.name, "bariera_ostrzegawcza_prawko");
    EXPECT_NEAR(object.s, old_s + 6.0, 0.01);
    EXPECT_NEAR(object.hdg, 1.571, 0.01);
  });
  EXPECT_EQ(count, 1) << "The UI drop did not persist a real GLB object";
}

TEST_F(MainWindowLayoutTest, BlenderShelterLibraryDropMoveRotateSave) {
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  // Opt-in forensic capture of this test's ACTUAL serialized editor output.
  // CI and ordinary runs still clean up the temporary project as usual.
  if (qEnvironmentVariableIsSet("PRAWKO_KEEP_BLENDER_TEST")) {
    directory.setAutoRemove(false);
  }
  const auto root = std::filesystem::path(directory.path().toStdU16String());
  ASSERT_TRUE(Project::create(root, QStringLiteral("Prawko GLB proof")).has_value());
  const auto scene = root / "wiata.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "straight_road.xodr", scene);
  {
    std::ofstream state(root / "wiata.rmscene.json");
    state
        << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  AssetImportRequest request;
  request.source =
      std::filesystem::path(RM_ASSETS_DIR) / "prawko/blender/wiata_przystankowa/prawko_wiata_przystankowa.glb";
  request.label = QStringLiteral("Wiata przystankowa Prawko");
  request.category = QStringLiteral("Prawko | Obiekty 3D");
  request.license = QStringLiteral("Model własny PrawkoNaRaz");
  auto imported = import_prop_asset(root, request);
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  LibraryManifest overlay;
  overlay.upsert(std::get<0>(*imported));
  const auto manifest = root / "assets/library/manifest.json";
  std::filesystem::create_directories(manifest.parent_path());
  ASSERT_TRUE(overlay.save(manifest).has_value());

  window_.load_file(scene);
  auto* library = window_.findChild<LibraryPanel*>();
  ASSERT_NE(library, nullptr);
  // A saved Scenario scene must not conceal project GLBs behind the traffic
  // actor category on launch; the first visible tiles must be the 3D models.
  EXPECT_EQ(library->category_combo()->currentData().toString(),
            QStringLiteral("Prawko | Obiekty 3D"));
  const auto* proxy = library->view()->model();
  ASSERT_NE(proxy, nullptr);
  const int models_category =
      library->category_combo()->findData(QStringLiteral("Prawko | Obiekty 3D"));
  ASSERT_GE(models_category, 0);
  auto* add_menu = window_.findChild<QMenu*>(QStringLiteral("menu.prawko_library"));
  ASSERT_NE(add_menu, nullptr);
  ASSERT_EQ(add_menu->actions().size(), 4);
  EXPECT_TRUE(add_menu->actions()[3]->isVisible());
  add_menu->actions()[3]->trigger();
  EXPECT_EQ(library->category_combo()->currentData().toString(),
            QStringLiteral("Prawko | Obiekty 3D"));
  library->category_combo()->setCurrentIndex(models_category);
  ASSERT_EQ(proxy->rowCount(), 1);
  const QModelIndex tile = proxy->index(0, 0);
  const QByteArray key = proxy->data(tile, LibraryListModel::KeyRole).toByteArray();
  EXPECT_EQ(key, QByteArray("prop.wiata_przystankowa_prawko"));

  auto* viewport = window_.viewport();
  viewport->resize(800, 500);
  viewport->set_camera_preset(QStringLiteral("ortho"));
  const double w = viewport->width(), h = viewport->height();
  const auto projected =
      project_to_screen(viewport->camera().matrices(float(w / h)), 44.0, -5.0, 0.0, w, h);
  ASSERT_TRUE(projected.has_value());
  const QPoint pos = QPointF((*projected)[0], (*projected)[1]).toPoint();
  ASSERT_TRUE(viewport->rect().contains(pos));
  std::unique_ptr<QMimeData> mime(proxy->mimeData({tile}));
  ASSERT_NE(mime, nullptr);
  QDragEnterEvent enter(pos, Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(viewport, &enter);
  ASSERT_TRUE(enter.isAccepted());
  QDropEvent drop(QPointF(pos), Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(viewport, &drop);
  ASSERT_TRUE(drop.isAccepted());

  // The drop selected the newly created object. Heading and along-road
  // position are user-editable, not just internal fields.
  auto* heading = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_hdg_spin"));
  auto* station = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_s_spin"));
  ASSERT_NE(heading, nullptr);
  ASSERT_NE(station, nullptr);
  const double old_s = station->value();
  EXPECT_GT(old_s, 20.0);
  station->setValue(old_s + 6.0);
  QMetaObject::invokeMethod(station, "editingFinished", Qt::DirectConnection);
  heading->setValue(1.571);
  QMetaObject::invokeMethod(heading, "editingFinished", Qt::DirectConnection);

  auto* save = window_.findChild<QAction*>(QStringLiteral("file_save"));
  if (save == nullptr) {
    const auto file_actions = window_.menuBar()->actions().front()->menu()->actions();
    const auto it = std::ranges::find_if(file_actions, [](const QAction* action) {
      return action->text() == QStringLiteral("&Save");
    });
    ASSERT_NE(it, file_actions.end());
    save = *it;
  }
  save->trigger();
  const auto reloaded = load_xodr(scene);
  ASSERT_TRUE(reloaded.has_value()) << reloaded.error().message;
  int count = 0;
  reloaded->network.for_each_object([&](ObjectId, const Object& object) {
    ++count;
    EXPECT_EQ(object.name, "wiata_przystankowa_prawko");
    EXPECT_NEAR(object.s, old_s + 6.0, 0.01);
    EXPECT_NEAR(object.hdg, 1.571, 0.01);
  });
  EXPECT_EQ(count, 1) << "The UI drop did not persist a real GLB object";
}

TEST_F(MainWindowLayoutTest, R6BuildingGlbLibraryDropRotateSaveReload) {
  // A separate temporary project proves that the REAL authored building GLB
  // works with the same mouse-drop/editor/model as the shelter (not only CLI).
  QTemporaryDir temp;
  ASSERT_TRUE(temp.isValid());
  const auto root = std::filesystem::path(temp.path().toStdU16String());
  ASSERT_TRUE(Project::create(root, QStringLiteral("R6 kamienica proof")).has_value());
  const auto scene = root / "r6-building.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "straight_road.xodr", scene);
  {
    std::ofstream state(root / "r6-building.rmscene.json");
    state << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  AssetImportRequest request;
  request.source = std::filesystem::path(RM_ASSETS_DIR) /
      "prawko/blender/kamienica_miejska_r6/r6_kamienica_miejska.glb";
  request.label = QStringLiteral("Kamienica miejska Prawko R6");
  request.category = QStringLiteral("Prawko | Obiekty 3D");
  request.license = QStringLiteral("Autorski model PrawkoNaRaz");
  const auto imported = import_prop_asset(root, request);
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  const std::string model_id = std::get<1>(*imported).slug.toStdString();
  LibraryManifest manifest;
  manifest.upsert(std::get<0>(*imported));
  const auto manifest_file = root / "assets/library/manifest.json";
  std::filesystem::create_directories(manifest_file.parent_path());
  ASSERT_TRUE(manifest.save(manifest_file).has_value());
  window_.load_file(scene);
  auto* library = window_.findChild<LibraryPanel*>();
  ASSERT_NE(library, nullptr);
  const int cat = library->category_combo()->findData(QStringLiteral("Prawko | Obiekty 3D"));
  ASSERT_GE(cat,0);
  library->category_combo()->setCurrentIndex(cat);
  const auto* proxy = library->view()->model();
  ASSERT_EQ(proxy->rowCount(),1);
  const QModelIndex tile = proxy->index(0,0);
  auto* view = window_.viewport();
  view->resize(800,500);
  view->set_camera_preset(QStringLiteral("ortho"));
  const auto point = project_to_screen(view->camera().matrices(1.6F),44.0,-5.0,0.0,800,500);
  ASSERT_TRUE(point.has_value());
  const QPoint pos = QPointF((*point)[0],(*point)[1]).toPoint();
  ASSERT_TRUE(view->rect().contains(pos));
  std::unique_ptr<QMimeData> mime(proxy->mimeData({tile}));
  ASSERT_NE(mime,nullptr);
  QDragEnterEvent enter(pos,Qt::CopyAction,mime.get(),Qt::LeftButton,Qt::NoModifier);
  QApplication::sendEvent(view,&enter);
  ASSERT_TRUE(enter.isAccepted());
  QDropEvent drop(QPointF(pos),Qt::CopyAction,mime.get(),Qt::LeftButton,Qt::NoModifier);
  QApplication::sendEvent(view,&drop);
  ASSERT_TRUE(drop.isAccepted());
  auto* station = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_s_spin"));
  auto* heading = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_hdg_spin"));
  ASSERT_NE(station,nullptr);
  ASSERT_NE(heading,nullptr);
  const double old_s=station->value();
  station->setValue(old_s+6.0);
  QMetaObject::invokeMethod(station,"editingFinished",Qt::DirectConnection);
  heading->setValue(1.57);
  QMetaObject::invokeMethod(heading,"editingFinished",Qt::DirectConnection);
  auto* save=window_.findChild<QAction*>(QStringLiteral("file_save"));
  if(!save) {
    const auto actions=window_.menuBar()->actions().front()->menu()->actions();
    const auto match=std::ranges::find_if(actions,[](const QAction* a){return a->text()==QStringLiteral("&Save");});
    ASSERT_NE(match,actions.end());
    save=*match;
  }
  save->trigger();
  const auto parsed=load_xodr(scene);
  ASSERT_TRUE(parsed.has_value()) << parsed.error().message;
  int count=0;
  parsed->network.for_each_object([&](ObjectId,const Object& o){
    ++count;
    EXPECT_EQ(o.name,model_id);
    EXPECT_NEAR(o.s,old_s+6.0,.01);
    EXPECT_NEAR(o.hdg,1.57,.01);
  });
  EXPECT_EQ(count,1) << "R6 building must be persisted by real editor drag/drop";
}

TEST_F(MainWindowLayoutTest, R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved) {
  // Real Qt drag/drop through the SAME UI handlers as manual library placement,
  // on a disposable test project: no change to the user's five original scenes.
  QTemporaryDir tmp;
  ASSERT_TRUE(tmp.isValid());
  const auto root = std::filesystem::path(tmp.path().toStdU16String());
  ASSERT_TRUE(Project::create(root, QStringLiteral("R6b street assets GUI proof")).has_value());
  const auto scene = root / "r6b-street.xodr";
  std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "straight_road.xodr",
                             scene);
  {
    std::ofstream state(root / "r6b-street.rmscene.json");
    state << R"({"scene_version":1,"view":{"distance":60,"pitch":1.56,"projection":"orthographic","target":[50,0,0],"yaw":-1.57}})";
  }
  const std::array<const char*, 16> names{
      "r6b-chodnik-20m", "r6b-drzewo", "r6b-latarnia", "r6b2-samochod",
      "r6b2b-pas-zieleni-20m", "r6b2b-rabata-ogrodowa",
      "r6b2b-sciezka-zwirowa-12m", "r6b2c-lipa-9m",
      "r6b2c-brzoza-10m", "r6b2c-pas-fundamentowy-12m",
      "r6b2d-przejscie-plyty-12m", "r6b2d-styk-chodnik-ziemia-20m", "r6b2g-latarnia-emisyjna", "r6b2h-kaluza-nieregularna-240",
      "r6b2h-kaluza-placowa-140", "r6b2i-kaluza-przykraweznikowa-700"};
  LibraryManifest overlay;
  std::vector<QString> keys;
  std::vector<std::string> ids;
  for (const char* name : names) {
    AssetImportRequest request;
    const std::string stem(name);
    request.source = std::filesystem::path(RM_ASSETS_DIR) /
                     (stem.starts_with("r6b2i-")
                         ? "prawko/blender/kaluze_r6b2i"
                         : stem.starts_with("r6b2h-")
                         ? "prawko/blender/kaluze_r6b2h"
                         : stem.starts_with("r6b2g-")
                         ? "prawko/blender/oprawa_r6b2g"
                         : stem.starts_with("r6b2d-")
                         ? "prawko/blender/styki_r6b2d"
                         : stem.starts_with("r6b2c-")
                         ? "prawko/blender/drzewa_r6b2c"
                         : stem.starts_with("r6b2b-") ? "prawko/blender/teren_r6b2b"
                         : stem == "r6b2-samochod" ? "prawko/blender/samochod_r6b2"
                                                    : "prawko/blender/ulica_r6b") /
                     (stem + ".glb");
    request.label = QString::fromUtf8(name);
    request.category = QStringLiteral("Prawko | Obiekty 3D");
    request.license = QStringLiteral("PrawkoNaRaz original asset, Apache-2.0");
    auto imported = import_prop_asset(root, request);
    ASSERT_TRUE(imported.has_value()) << name << ": " << imported.error().message;
    keys.push_back(std::get<0>(*imported).key);
    ids.push_back(std::get<1>(*imported).slug.toStdString());
    overlay.upsert(std::get<0>(*imported));
  }
  const auto manifest = root / "assets/library/manifest.json";
  std::filesystem::create_directories(manifest.parent_path());
  ASSERT_TRUE(overlay.save(manifest).has_value());
  window_.load_file(scene);
  auto* library = window_.findChild<LibraryPanel*>();
  ASSERT_NE(library, nullptr);
  const int cat = library->category_combo()->findData(QStringLiteral("Prawko | Obiekty 3D"));
  ASSERT_GE(cat, 0);
  library->category_combo()->setCurrentIndex(cat);
  const auto* proxy = library->view()->model();
  ASSERT_EQ(proxy->rowCount(), 16);
  auto* viewport = window_.viewport();
  viewport->resize(800, 500);
  viewport->set_camera_preset(QStringLiteral("ortho"));
  const auto screen = project_to_screen(viewport->camera().matrices(1.6F),
                                        44.0, -5.0, 0.0, 800, 500);
  ASSERT_TRUE(screen.has_value());
  const QPoint where = QPointF((*screen)[0], (*screen)[1]).toPoint();
  ASSERT_TRUE(viewport->rect().contains(where));
  auto* station = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_s_spin"));
  auto* heading = window_.findChild<QDoubleSpinBox*>(QStringLiteral("object_hdg_spin"));
  ASSERT_NE(station, nullptr);
  ASSERT_NE(heading, nullptr);
  struct ExpectedObject {
    std::string model;
    double station;
    double heading;
  };
  std::vector<ExpectedObject> expected;
  for (std::size_t i = 0; i < keys.size(); ++i) {
    QModelIndex tile;
    for (int row = 0; row < proxy->rowCount(); ++row) {
      const QModelIndex candidate = proxy->index(row, 0);
      if (proxy->data(candidate, LibraryListModel::KeyRole).toString() == keys[i]) {
        tile = candidate;
        break;
      }
    }
    ASSERT_TRUE(tile.isValid()) << "Missing R6b asset in actual Qt library";
    std::unique_ptr<QMimeData> mime(proxy->mimeData({tile}));
    ASSERT_NE(mime, nullptr);
    QDragEnterEvent enter(where, Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &enter);
    ASSERT_TRUE(enter.isAccepted());
    QDropEvent drop(QPointF(where), Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(viewport, &drop);
    ASSERT_TRUE(drop.isAccepted());
    const double new_s = station->value() + 6.0 + static_cast<double>(i);
    const double new_hdg = 0.4 + static_cast<double>(i)*0.35; // within one full OpenDRIVE turn
    station->setValue(new_s);
    QMetaObject::invokeMethod(station, "editingFinished", Qt::DirectConnection);
    heading->setValue(new_hdg);
    QMetaObject::invokeMethod(heading, "editingFinished", Qt::DirectConnection);
    expected.push_back({ids[i], new_s, new_hdg});
  }
  auto* save = window_.findChild<QAction*>(QStringLiteral("file_save"));
  if (!save) {
    const auto actions = window_.menuBar()->actions().front()->menu()->actions();
    const auto match = std::ranges::find_if(actions, [](const QAction* action) {
      return action->text() == QStringLiteral("&Save");
    });
    ASSERT_NE(match, actions.end());
    save = *match;
  }
  save->trigger();
  auto loaded = load_xodr(scene);
  ASSERT_TRUE(loaded.has_value()) << loaded.error().message;
  int count = 0;
  loaded->network.for_each_object([&](ObjectId, const Object& obj) {
    ++count;
    const auto it = std::ranges::find(expected, obj.name, &ExpectedObject::model);
    ASSERT_NE(it, expected.end()) << "Unexpected object persisted in R6b test";
    EXPECT_NEAR(obj.s, it->station, 0.01);
    EXPECT_NEAR(obj.hdg, it->heading, 0.01);
  });
  EXPECT_EQ(count, 16) << "All sixteen road/landscape GLBs should drag, rotate and persist";
}

TEST_F(MainWindowLayoutTest, CuratedLibraryStaysAvailableAcrossMapAndScenarioModes) {
  auto* library = window_.findChild<LibraryPanel*>();
  ASSERT_NE(library, nullptr);
  auto* scenario_mode = window_.findChild<QAction*>(QStringLiteral("scenario_mode"));
  ASSERT_NE(scenario_mode, nullptr);
  const auto verify = [library] {
    // Changing modes may focus its relevant category; explicitly select
    // All to verify total availability rather than the current filter.
    library->category_combo()->setCurrentIndex(0);
    ASSERT_EQ(library->category_combo()->count(), 4);
    // The current category is intentionally retained across editing modes.
    // Switch to All before checking the 18-item complete catalogue.
    library->category_combo()->setCurrentIndex(0);
    ASSERT_EQ(library->view()->model()->rowCount(), 18);
    const int actor_category = library->category_combo()->findData(kScenarioActorCategory);
    ASSERT_GE(actor_category, 0);
    library->category_combo()->setCurrentIndex(actor_category);
    ASSERT_EQ(library->view()->model()->rowCount(), 6);
    for (int row = 0; row < 6; ++row) {
      EXPECT_EQ(library->view()
                    ->model()
                    ->data(library->view()->model()->index(row, 0), LibraryListModel::CategoryRole)
                    .toString(),
                kScenarioActorCategory);
    }
    library->category_combo()->setCurrentIndex(0);
    EXPECT_EQ(library->view()->model()->rowCount(), 18);
  };
  scenario_mode->setChecked(false);
  verify();
  scenario_mode->setChecked(true);
  verify();
  scenario_mode->setChecked(false);
  verify();
}

TEST_F(MainWindowLayoutTest, ImageExportIsEnabledWhenASceneOpens) {
  auto* image = window_.findChild<QAction*>(QStringLiteral("export_scene_image"));
  ASSERT_NE(image, nullptr);
  EXPECT_FALSE(image->isEnabled());
  QTemporaryDir directory;
  ASSERT_TRUE(directory.isValid());
  const auto file =
      std::filesystem::path(directory.filePath(QStringLiteral("scene.xodr")).toStdString());
  ASSERT_TRUE(roadmaker::save_xodr(RoadNetwork{}, file, "scene").has_value());
  window_.load_file(file);
  EXPECT_TRUE(image->isEnabled());
}

TEST_F(MainWindowLayoutTest, SingleCompactToolbarPresentsOnlyThePrawkoWorkflow) {
  auto* bar = window_.findChild<QToolBar*>(QStringLiteral("toolbar.main"));
  auto* add = window_.findChild<QToolButton*>(QStringLiteral("toolbar.add"));
  auto* more = window_.findChild<QToolButton*>(QStringLiteral("toolbar.more"));
  auto* library = window_.findChild<QMenu*>(QStringLiteral("menu.prawko_library"));
  auto* advanced = window_.findChild<QMenu*>(QStringLiteral("menu.advanced_tools"));
  auto* options = window_.findChild<QToolBar*>(QStringLiteral("toolbar.options"));
  ASSERT_NE(bar, nullptr);
  ASSERT_NE(add, nullptr);
  ASSERT_NE(more, nullptr);
  ASSERT_NE(library, nullptr);
  ASSERT_NE(advanced, nullptr);
  ASSERT_NE(options, nullptr);
  EXPECT_EQ(window_.findChild<QToolBar*>(QStringLiteral("toolbar.tools")), nullptr);
  EXPECT_EQ(window_.findChild<QTabBar*>(QStringLiteral("toolbar_tabs")), nullptr);
  EXPECT_EQ(window_.findChild<QToolBar*>(QStringLiteral("toolbar.tabs")), nullptr);
  EXPECT_EQ(bar->iconSize(), QSize(24, 24));
  EXPECT_LE(bar->actions().size(), 19) << "The whole legacy tool ribbon has returned";
  EXPECT_TRUE(options->isHidden()) << "Idle tools should not consume an empty toolbar row";

  const QStringList main_labels = {QStringLiteral("Nowa"),
                                   QStringLiteral("Otwórz"),
                                   QStringLiteral("Zapisz"),
                                   QStringLiteral("Zaznacz"),
                                   QStringLiteral("Przesuń"),
                                   QStringLiteral("Usuń"),
                                   QStringLiteral("Cofnij"),
                                   QStringLiteral("Droga"),
                                   QStringLiteral("Trasa"),
                                   QStringLiteral("Scenariusz"),
                                   QStringLiteral("Symulacja")};
  for (const QString& label : main_labels) {
    EXPECT_TRUE(std::ranges::any_of(bar->actions(), [&label](const QAction* a) {
      return !a->isSeparator() && a->iconText() == label;
    })) << label.toStdString();
  }
  EXPECT_EQ(add->text(), QStringLiteral("Dodaj"));
  EXPECT_EQ(more->text(), QStringLiteral("Więcej"));
  ASSERT_EQ(library->actions().size(), 4);
  EXPECT_EQ(library->actions()[0]->text(), QStringLiteral("Gotowe drogi"));
  EXPECT_EQ(library->actions()[1]->text(), QStringLiteral("Krawężniki i chodniki"));
  EXPECT_EQ(library->actions()[2]->text(), QStringLiteral("Uczestnicy ruchu"));
  EXPECT_EQ(library->actions()[3]->text(), QStringLiteral("Obiekty 3D"));
  EXPECT_FALSE(library->actions()[3]->isVisible())
      << "No imported models: do not show an empty category.";
  auto* category = window_.findChild<QComboBox*>(QStringLiteral("library_category"));
  ASSERT_NE(category, nullptr);
  for (int index = 0; index < 3; ++index) {
    library->actions()[index]->trigger();
    EXPECT_EQ(category->currentData().toString(),
              QStringList({QStringLiteral("Prawko | Gotowe drogi"),
                           QStringLiteral("Prawko | Krawężniki i chodniki"),
                           QStringLiteral("Uczestnicy ruchu")})[index]);
  }

  // Every non-primary registry tool is present exactly once in a category.
  int expected = 0;
  for (const auto& tab : shortcuts::toolbar_tabs()) {
    for (const auto& group : shortcuts::toolbar_layout(tab.tab)) {
      for (const auto id : group.ids) {
        if (id != shortcuts::Id::ToolCreateRoad && id != shortcuts::Id::ToolRoute) {
          ++expected;
        }
      }
    }
  }
  int present = 0;
  for (QMenu* submenu : advanced->findChildren<QMenu*>()) {
    for (QAction* action : submenu->actions()) {
      if (!action->isSeparator())
        ++present;
    }
  }
  EXPECT_EQ(present, expected);
  EXPECT_GE(advanced->findChildren<QMenu*>().size(), 5);
}

TEST_F(MainWindowLayoutTest, ToolOptionHintIsGone) {
  EXPECT_EQ(window_.findChild<QWidget*>(QStringLiteral("toolOptionHint")), nullptr);
}

TEST_F(MainWindowLayoutTest, ToolOptionsContentDoesNotTrackTheTool) {
  auto* caption = window_.findChild<QLabel*>(QStringLiteral("toolOptionCaption"));
  auto* instruction = window_.findChild<ElidedLabel*>(QStringLiteral("status_instruction"));
  ASSERT_NE(caption, nullptr);
  ASSERT_NE(instruction, nullptr);
  const QString text = caption->text();
  ASSERT_FALSE(text.isEmpty());

  // Every tool: the caption may only flip visibility, never content — while the
  // per-tool sentence does change, in the status bar where it belongs.
  static const QStringList kTools{QStringLiteral("select"),          QStringLiteral("move"),
                                  QStringLiteral("create-road"),     QStringLiteral("edit-nodes"),
                                  QStringLiteral("lane-profile"),    QStringLiteral("elevation"),
                                  QStringLiteral("create-junction"), QStringLiteral("split"),
                                  QStringLiteral("delete"),          QStringLiteral("lane-add"),
                                  QStringLiteral("lane-form"),       QStringLiteral("lane-carve"),
                                  QStringLiteral("crosswalk"),       QStringLiteral("markingPoint"),
                                  QStringLiteral("markingCurve"),    QStringLiteral("propPoint"),
                                  QStringLiteral("propCurve"),       QStringLiteral("propSpan"),
                                  QStringLiteral("propPolygon"),     QStringLiteral("corner"),
                                  QStringLiteral("stopline")};
  for (const QString& tool : kTools) {
    window_.activate_tool_for_capture(tool);
    EXPECT_EQ(caption->text(), text) << "tool " << tool.toStdString();
    EXPECT_FALSE(instruction->full_text().isEmpty()) << "tool " << tool.toStdString();
  }
}

TEST_F(MainWindowLayoutTest, InstructionSitsLeftOfThePermanentIndicators) {
  auto* instruction = window_.findChild<ElidedLabel*>(QStringLiteral("status_instruction"));
  ASSERT_NE(instruction, nullptr);
  EXPECT_EQ(instruction->parentWidget(), window_.statusBar());

  // QStatusBar lays out on resize (it has no QLayout), and this window is never
  // shown, so drive one resize by hand. showMessage()'s hide/show of the normal
  // section cannot be asserted headless — QStatusBar only hides items that
  // report isVisible(), which nothing in an unshown window does.
  window_.resize(1200, 800);
  QStatusBar* bar = window_.statusBar();
  bar->resize(1200, bar->sizeHint().height());
  QResizeEvent resized(bar->size(), bar->size());
  QApplication::sendEvent(bar, &resized);
  QCoreApplication::sendPostedEvents();

  ASSERT_GT(instruction->width(), 0) << "status bar never laid out";
  for (const QLabel* other : bar->findChildren<QLabel*>()) {
    if (other == instruction) {
      continue;
    }
    EXPECT_LT(instruction->x(), other->x())
        << "the instruction owns the left edge; '" << other->text().toStdString()
        << "' must sit to its right";
    EXPECT_GT(instruction->width(), other->width()) << "the instruction takes the bar's stretch";
  }
}

TEST_F(MainWindowLayoutTest, InstructionElidesInsteadOfWidening) {
  auto* instruction = window_.findChild<ElidedLabel*>(QStringLiteral("status_instruction"));
  ASSERT_NE(instruction, nullptr);
  const QString sentence =
      QStringLiteral("Click to place polygon points, double-click or Enter to close the loop, "
                     "Esc to cancel");
  instruction->set_full_text(sentence);
  EXPECT_EQ(instruction->minimumSizeHint().width(), 0)
      << "an elided label must not impose the full text as a minimum width";

  instruction->resize(60, instruction->sizeHint().height());
  EXPECT_TRUE(instruction->displayed_text().endsWith(QChar(0x2026)));
  EXPECT_EQ(instruction->full_text(), sentence) << "the source text survives eliding";

  instruction->resize(2000, instruction->sizeHint().height());
  EXPECT_EQ(instruction->displayed_text(), sentence);
}

// #333 — View ▸ Viewport Hints. The window is constructed against a CLEARED
// QSettings scope so the assertions see the shipped default, never whatever
// the developer running the suite last toggled (and nothing leaks back out).
class ViewportHintsToggleTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Same isolation as the Settings suites: a test-only QSettings scope, so a
    // toggle asserted here can never land in the developer's real RoadMaker
    // settings (nor be read from them).
    QCoreApplication::setOrganizationName(QStringLiteral("RobomousTests"));
    // Per-test app name: ctest -j runs sibling cases as concurrent processes
    // that would otherwise race on one shared settings domain.
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    QCoreApplication::setApplicationName(QStringLiteral("RoadMakerHintsTest_") +
                                         QString::fromUtf8(info->name()));
    QSettings().clear();
    window_ = std::make_unique<MainWindow>(nullptr, /*restore_saved_layout=*/false);
    // The action is reached through its binding rather than its label, so the
    // test does not re-hardcode a key letter the registry owns (the drift this
    // whole registry exists to prevent).
    for (QAction* action : window_->findChildren<QAction*>()) {
      if (action->shortcuts() == shortcuts::sequences(shortcuts::Id::ViewportHints)) {
        toggle_ = action;
        break;
      }
    }
  }

  void TearDown() override {
    window_.reset();
    QSettings().clear();
    // Back to the suite-wide test scope (qt_gtest_main.cpp), NOT the shipped
    // Robomous/RoadMaker names: restoring those would re-arm the developer's
    // real settings store for every later case in this process (#399).
    QCoreApplication::setOrganizationName(QStringLiteral("RobomousTests"));
    QCoreApplication::setApplicationName(QStringLiteral("RoadMakerEditorTests"));
  }

  std::unique_ptr<MainWindow> window_;
  QAction* toggle_ = nullptr;
};

TEST_F(ViewportHintsToggleTest, DefaultsOnAndIsACheckableViewMenuEntry) {
  ASSERT_NE(toggle_, nullptr) << "no action carries the ViewportHints binding";
  EXPECT_TRUE(toggle_->isCheckable());
  EXPECT_TRUE(toggle_->isChecked());
  EXPECT_TRUE(window_->viewport()->hints_enabled());

  const QList<QAction*> menus = window_->menuBar()->actions();
  const auto view = std::find_if(menus.begin(), menus.end(), [](const QAction* action) {
    return action->menu() != nullptr && action->text().contains(QStringLiteral("View"));
  });
  ASSERT_NE(view, menus.end());
  EXPECT_TRUE((*view)->menu()->actions().contains(toggle_));
}

TEST_F(ViewportHintsToggleTest, TogglingHidesTheCardAndKeepsTheText) {
  ASSERT_NE(toggle_, nullptr);
  ViewportWidget* viewport = window_->viewport();
  window_->activate_tool_for_capture(QStringLiteral("create-road"));
  const QString hint = viewport->hint();
  ASSERT_FALSE(hint.isEmpty()) << "the active tool must feed the corner hint";
  EXPECT_TRUE(viewport->hint_visible());

  toggle_->setChecked(false);
  EXPECT_FALSE(viewport->hints_enabled());
  EXPECT_FALSE(viewport->hint_visible());
  EXPECT_EQ(viewport->hint(), hint) << "only painting is gated; the text is kept";

  toggle_->setChecked(true);
  EXPECT_TRUE(viewport->hint_visible()) << "re-enabling shows the CURRENT tool's hint at once";
}

TEST_F(ViewportHintsToggleTest, StatusBarInstructionIsUnaffected) {
  ASSERT_NE(toggle_, nullptr);
  auto* instruction = window_->findChild<ElidedLabel*>(QStringLiteral("status_instruction"));
  ASSERT_NE(instruction, nullptr);
  window_->activate_tool_for_capture(QStringLiteral("create-road"));
  const QString sentence = instruction->full_text();
  ASSERT_FALSE(sentence.isEmpty());

  toggle_->setChecked(false);
  EXPECT_EQ(instruction->full_text(), sentence);
  window_->activate_tool_for_capture(QStringLiteral("elevation"));
  EXPECT_FALSE(instruction->full_text().isEmpty())
      << "the status bar keeps tracking the tool while the viewport hint is off";
}

TEST_F(ViewportHintsToggleTest, ChoicePersistsAcrossWindows) {
  ASSERT_NE(toggle_, nullptr);
  toggle_->setChecked(false);
  EXPECT_FALSE(Settings().viewport_hints());

  MainWindow reopened(nullptr, /*restore_saved_layout=*/false);
  EXPECT_FALSE(reopened.viewport()->hints_enabled());
}

// fmt-s1 (#325): the app-level half of Layer 2 — the per-scene render mode and
// project.json's last_scene. The window is constructed, never shown (see the
// file header); the viewport's GL never initializes offscreen, which is why
// the camera assertions go through frame_pending() rather than a rendered
// frame.
class SceneStateWindowTest : public ::testing::Test {
protected:
  void SetUp() override {
    // The same QSettings isolation the other window suites use: this case
    // reads and asserts the application render-mode default, and must never
    // see (or write) the developer's real store (#399).
    QCoreApplication::setOrganizationName(QStringLiteral("RobomousTests"));
    const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
    QCoreApplication::setApplicationName(QStringLiteral("RoadMakerSceneStateTest_") +
                                         QString::fromUtf8(info->name()));
    QSettings().clear();
    dir_ = std::make_unique<QTemporaryDir>();
    ASSERT_TRUE(dir_->isValid());
    window_ = std::make_unique<MainWindow>(nullptr, /*restore_saved_layout=*/false);
  }

  void TearDown() override {
    window_.reset();
    dir_.reset();
    QSettings().clear();
    QCoreApplication::setOrganizationName(QStringLiteral("RobomousTests"));
    QCoreApplication::setApplicationName(QStringLiteral("RoadMakerEditorTests"));
  }

  [[nodiscard]] std::filesystem::path root() const {
    return std::filesystem::path(dir_->path().toStdString());
  }

  /// A copy of the sample scene inside `root()`, optionally with a sidecar.
  std::filesystem::path make_scene(const char* name, const char* sidecar_json) {
    const std::filesystem::path scene = root() / name;
    std::filesystem::copy_file(std::filesystem::path(RM_SAMPLES_DIR) / "t_junction.xodr", scene);
    if (sidecar_json != nullptr) {
      std::ofstream out(scene_sidecar::path_for(scene));
      out << sidecar_json;
    }
    return scene;
  }

  std::unique_ptr<QTemporaryDir> dir_;
  std::unique_ptr<MainWindow> window_;
};

TEST_F(SceneStateWindowTest, AStoredViewSuppressesTheAutoFraming) {
  // No sidecar: the scene frames itself, as it always has.
  window_->load_file(make_scene("bare.xodr", nullptr));
  EXPECT_TRUE(window_->viewport()->frame_pending());

  // With one: the restored camera wins, or reopening a scene would silently
  // snap back to the whole-network framing on the next paint.
  window_->load_file(make_scene("posed.xodr", R"({
    "scene_version": 1,
    "view": {"target": [10, 20, 1], "yaw": 0.3, "pitch": 0.6, "distance": 55,
             "projection": "orthographic"}
  })"));
  EXPECT_FALSE(window_->viewport()->frame_pending());
  const SceneViewState restored = window_->viewport()->view_state();
  EXPECT_EQ(restored.distance, 55.0F);
  EXPECT_EQ(restored.projection, ProjectionMode::Orthographic);
}

TEST_F(SceneStateWindowTest, TheStoredRenderModeWinsForThatSceneAndIsNotAPreference) {
  const bool app_default = Settings().textured_rendering();
  window_->load_file(make_scene("scene.xodr",
                                app_default ? R"({"scene_version": 1, "textured": false})"
                                            : R"({"scene_version": 1, "textured": true})"));
  EXPECT_EQ(window_->viewport()->textured_rendering(), !app_default);
  // A per-scene override is not a preference: the application default must be
  // exactly where it was, or opening someone else's scene would silently
  // re-set the user's own default.
  EXPECT_EQ(Settings().textured_rendering(), app_default);

  // A scene with no stored mode falls back to that untouched default.
  window_->load_file(make_scene("plain.xodr", nullptr));
  EXPECT_EQ(window_->viewport()->textured_rendering(), app_default);
}

TEST_F(SceneStateWindowTest, TimeOfDayRestoresAndUiChangesReachViewport) {
  window_->load_file(make_scene("night.xodr",
                  R"({"scene_version":1,"textured":true,"time_of_day":"night"})"));
  auto* combo = window_->findChild<QComboBox*>(QStringLiteral("simulation_time_of_day"));
  ASSERT_NE(combo, nullptr);
  EXPECT_EQ(combo->currentData().toString(), QStringLiteral("night"));
  EXPECT_EQ(window_->viewport()->time_of_day(), TimeOfDay::Night);
  EXPECT_TRUE(window_->viewport()->textured_rendering());

  const int dawn = combo->findData(QStringLiteral("dawn"));
  ASSERT_GE(dawn, 0);
  combo->setCurrentIndex(dawn);
  EXPECT_EQ(window_->viewport()->time_of_day(), TimeOfDay::Dawn);

  window_->load_file(make_scene("old.xodr", nullptr));
  EXPECT_EQ(combo->currentData().toString(), QStringLiteral("day"));
  EXPECT_EQ(window_->viewport()->time_of_day(), TimeOfDay::Day);
}

TEST_F(SceneStateWindowTest, WeatherAndSeasonRestoreAndChangeViewportWithUi) {
  window_->load_file(make_scene(
      "rain.xodr",
      R"({"scene_version":1,"textured":true,"time_of_day":"dusk","weather":"rain","season":"autumn"})"));
  auto* weather = window_->findChild<QComboBox*>(QStringLiteral("simulation_weather"));
  auto* season = window_->findChild<QComboBox*>(QStringLiteral("simulation_season"));
  ASSERT_NE(weather, nullptr);
  ASSERT_NE(season, nullptr);
  EXPECT_EQ(weather->currentData().toString(), QStringLiteral("rain"));
  EXPECT_EQ(season->currentData().toString(), QStringLiteral("autumn"));
  EXPECT_EQ(window_->viewport()->weather(), WeatherPreset::Rain);
  EXPECT_EQ(window_->viewport()->season(), SeasonPreset::Autumn);

  const int fog_index = weather->findData(QStringLiteral("fog"));
  const int winter_index = season->findData(QStringLiteral("winter"));
  ASSERT_GE(fog_index, 0);
  ASSERT_GE(winter_index, 0);
  weather->setCurrentIndex(fog_index);
  season->setCurrentIndex(winter_index);
  EXPECT_EQ(window_->viewport()->weather(), WeatherPreset::Fog);
  EXPECT_EQ(window_->viewport()->season(), SeasonPreset::Winter);

  window_->load_file(make_scene("old-weather.xodr", nullptr));
  EXPECT_EQ(weather->currentData().toString(), QStringLiteral("clear"));
  EXPECT_EQ(season->currentData().toString(), QStringLiteral("summer"));
  EXPECT_EQ(window_->viewport()->weather(), WeatherPreset::Clear);
  EXPECT_EQ(window_->viewport()->season(), SeasonPreset::Summer);
}

TEST_F(SceneStateWindowTest, SavingAndOpeningAProjectRemembersItsLastScene) {
  auto project = Project::create(root(), QStringLiteral("Downtown"));
  ASSERT_TRUE(project.has_value());
  const std::filesystem::path scene = make_scene("main.xodr", nullptr);
  window_->load_file(scene);

  const auto stamped = Project::open(root());
  ASSERT_TRUE(stamped.has_value());
  EXPECT_EQ(stamped->last_scene(), QStringLiteral("main.xodr"));

  // Reopening the project through the real welcome-tile path reopens that
  // scene — the half of the round trip that makes "you are where you left
  // off" true for a project, not just for a file.
  MainWindow reopened(nullptr, /*restore_saved_layout=*/false);
  auto* welcome = reopened.findChild<WelcomeWidget*>();
  ASSERT_NE(welcome, nullptr);
  EXPECT_FALSE(reopened.windowTitle().contains(QStringLiteral("main.xodr")));
  emit welcome->project_requested(dir_->path());
  // The title carries the open scene and the project — the observable that
  // says both halves happened (adopt the project, reopen its last scene).
  EXPECT_TRUE(reopened.windowTitle().contains(QStringLiteral("main.xodr")))
      << reopened.windowTitle().toStdString();
  EXPECT_TRUE(reopened.windowTitle().contains(QStringLiteral("Downtown")));
}

TEST(ElidedLabelTest, ElidesAndRestoresWithWidth) {
  ElidedLabel label;
  const QString sentence = QStringLiteral("A sentence long enough that no 40 pixels can hold it");
  label.set_full_text(sentence);

  label.resize(40, 20);
  EXPECT_NE(label.displayed_text(), sentence);
  EXPECT_TRUE(label.displayed_text().endsWith(QChar(0x2026)));

  label.resize(2000, 20);
  EXPECT_EQ(label.displayed_text(), sentence);
  EXPECT_EQ(label.full_text(), sentence);
}

} // namespace
} // namespace roadmaker::editor
