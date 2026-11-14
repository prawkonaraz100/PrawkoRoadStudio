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

#include "roadmaker/xodr/writer.hpp"

#include <gtest/gtest.h>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
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

TEST_F(MainWindowLayoutTest, ScenarioModeShowsActorLibraryAndRouteTools) {
  auto* mode = window_.findChild<QAction*>(QStringLiteral("scenario_mode"));
  auto* main = window_.findChild<QToolBar*>(QStringLiteral("toolbar.main"));
  auto* run = window_.findChild<QAction*>(QStringLiteral("run_scenario_simulation"));
  auto* tools = window_.findChild<QToolBar*>(QStringLiteral("toolbar.tools"));
  auto* storyboard_dock = window_.findChild<QDockWidget*>(QStringLiteral("dock.editor2d"));
  auto* editor = window_.findChild<Editor2DHost*>();
  auto* actor_kind = window_.findChild<QComboBox*>(QStringLiteral("actor_kind"));
  auto* options = window_.findChild<QToolBar*>(QStringLiteral("toolbar.options"));
  ASSERT_NE(mode, nullptr);
  ASSERT_NE(main, nullptr);
  ASSERT_NE(run, nullptr);
  ASSERT_NE(tools, nullptr);
  ASSERT_NE(storyboard_dock, nullptr);
  ASSERT_NE(editor, nullptr);
  ASSERT_NE(actor_kind, nullptr);
  ASSERT_NE(options, nullptr);
  QWidgetAction* actor_kind_action = nullptr;
  for (QAction* action : options->actions()) {
    auto* widget_action = qobject_cast<QWidgetAction*>(action);
    if (widget_action != nullptr && widget_action->defaultWidget() == actor_kind) {
      actor_kind_action = widget_action;
      break;
    }
  }
  ASSERT_NE(actor_kind_action, nullptr);
  ASSERT_TRUE(main->actions().contains(mode));
  EXPECT_EQ(main->actions().indexOf(run), main->actions().indexOf(mode) + 1);
  EXPECT_TRUE(run->isVisible());
  EXPECT_FALSE(run->isEnabled());
  const auto menus = window_.menuBar()->actions();
  EXPECT_TRUE(std::ranges::any_of(menus, [mode](QAction* entry) {
    return entry->menu() != nullptr && entry->menu()->actions().contains(mode);
  }));

  const auto tool_named = [tools](const QString& name) -> QAction* {
    const auto actions = tools->actions();
    const auto found = std::ranges::find_if(
        actions, [&name](QAction* action) { return action->iconText() == name; });
    return found == actions.end() ? nullptr : *found;
  };
  auto* actor = tool_named(QStringLiteral("Actor"));
  auto* route = tool_named(QStringLiteral("Trasa"));
  auto* road = tool_named(QStringLiteral("Road"));
  ASSERT_NE(actor, nullptr);
  ASSERT_NE(route, nullptr);
  ASSERT_NE(road, nullptr);
  EXPECT_FALSE(actor->isVisible());
  EXPECT_FALSE(route->isVisible());
  EXPECT_TRUE(road->isVisible());

  mode->trigger();
  EXPECT_TRUE(mode->isChecked());
  EXPECT_TRUE(actor->isVisible());
  EXPECT_TRUE(actor->isEnabled());
  EXPECT_TRUE(route->isVisible());
  EXPECT_TRUE(route->isEnabled());
  EXPECT_FALSE(road->isVisible());
  auto* category = window_.findChild<QComboBox*>(QStringLiteral("library_category"));
  ASSERT_NE(category, nullptr);
  EXPECT_EQ(category->currentData().toString(), QStringLiteral("Uczestnicy ruchu"));
  auto* storyboard = window_.menuBar()->findChild<QMenu*>(QStringLiteral("scenario_menu"));
  ASSERT_NE(storyboard, nullptr);
  const auto entries = storyboard->actions();
  auto entry = std::ranges::find_if(
      entries, [](QAction* action) { return action->text() == QStringLiteral("Storyboard…"); });
  ASSERT_NE(entry, entries.end());
  (*entry)->trigger();
  EXPECT_FALSE(storyboard_dock->isHidden());
  EXPECT_EQ(editor->current_title(), QStringLiteral("Storyboard"));
  actor->trigger();
  EXPECT_TRUE(actor_kind_action->isVisible());
  EXPECT_GE(actor_kind->count(), 2);

  mode->trigger();
  EXPECT_FALSE(mode->isChecked());
  EXPECT_TRUE(road->isVisible());
  EXPECT_TRUE(road->isEnabled());
  EXPECT_FALSE(actor->isVisible());
  EXPECT_FALSE(route->isVisible());
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
  auto* route_toolbar = window_.findChild<QToolBar*>(QStringLiteral("toolbar.tools"));
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

TEST_F(MainWindowLayoutTest, ToolbarIsTwoPlainGroupedRowsWithNoTabs) {
  // #377: the category tabs are gone. The chrome is two plain top-level
  // QToolBars in the top area — the core strip (`toolbar.main`: document ops)
  // and ONE flat tool row (`toolbar.tools`: EVERY placement tool at once,
  // separator-grouped). Both being plain top-level bars, they share one left
  // origin and align by construction — no QTabBar, no page-swapping, no widget
  // nested inside a toolbar.
  auto* core = window_.findChild<QToolBar*>(QStringLiteral("toolbar.main"));
  auto* tools = window_.findChild<QToolBar*>(QStringLiteral("toolbar.tools"));
  ASSERT_NE(core, nullptr) << "the core strip is gone";
  ASSERT_NE(tools, nullptr) << "the flat tool row is gone";

  // No tabs, and none of the old nested hosting, may come back.
  EXPECT_EQ(window_.findChild<QTabBar*>(QStringLiteral("toolbar_tabs")), nullptr)
      << "the category tab bar must be gone";
  EXPECT_EQ(window_.findChild<QToolBar*>(QStringLiteral("toolbar.tabs")), nullptr)
      << "the nested tab-host toolbar must be gone";
  for (const QToolBar* bar : window_.findChildren<QToolBar*>()) {
    EXPECT_FALSE(bar->objectName().startsWith(QStringLiteral("toolbar.tab.")))
        << "a nested page toolbar '" << bar->objectName().toStdString() << "' still exists";
  }

  // The flat row must hold every placement tool from every (non-empty) tab at
  // once — not just one tab's worth — with separators grouping them. Both counts
  // come from the registry so the assertion tracks the taxonomy as pillars land.
  int expected_tools = 0;
  int largest_tab = 0;
  for (const shortcuts::ToolbarTabInfo& info : shortcuts::toolbar_tabs()) {
    int tab_tools = 0;
    for (const shortcuts::ToolbarGroupLayout& group : shortcuts::toolbar_layout(info.tab)) {
      tab_tools += static_cast<int>(group.ids.size());
    }
    expected_tools += tab_tools;
    largest_tab = std::max(largest_tab, tab_tools);
  }

  int placed = 0;
  int separators = 0;
  for (const QAction* action : tools->actions()) {
    if (action->isSeparator()) {
      ++separators;
    } else {
      ++placed;
    }
  }
  EXPECT_EQ(placed, expected_tools) << "the flat row must render every tool across all tabs";
  EXPECT_GT(expected_tools, largest_tab)
      << "the row holds more than any single tab's tools — proof it is flat, not tabbed";
  EXPECT_GT(separators, 0) << "the tool groups must be visually separated";
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
