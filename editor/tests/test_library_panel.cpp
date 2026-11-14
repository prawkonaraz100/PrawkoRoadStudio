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

#include <gtest/gtest.h>

#include <QComboBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QSignalSpy>
#include <QTest>
#include <filesystem>

#include "document/library_list_model.hpp"
#include "document/library_manifest.hpp"
#include "panels/library_panel.hpp"

namespace roadmaker::editor {
namespace {

LibraryListModel& populated_model() {
  static LibraryListModel model;
  const auto manifest =
      LibraryManifest::load(std::filesystem::path(RM_ASSETS_DIR) / "library" / "manifest.json");
  if (manifest.has_value())
    model.set_manifest(*manifest);
  return model;
}

TEST(LibraryPanel, RoadStyleHasActionableHelpAndDoubleClick) {
  LibraryPanel panel(populated_model());
  panel.resize(420, 560);
  panel.show();
  panel.focus_category(QStringLiteral("Prawko | Krawężniki i chodniki"));
  auto* instructions = panel.findChild<QLabel*>(QStringLiteral("library_drag_instructions"));
  ASSERT_NE(instructions, nullptr);
  EXPECT_TRUE(instructions->text().contains(QStringLiteral("2×")));
  EXPECT_TRUE(instructions->text().contains(QStringLiteral("zaznaczoną drogę")));
  const QModelIndex first = panel.view()->model()->index(0, 0);
  ASSERT_TRUE(first.isValid());
  QSignalSpy activated(&panel, &LibraryPanel::asset_activated);
  ASSERT_TRUE(activated.isValid());
  QApplication::processEvents();
  const QRect tile = panel.view()->visualRect(first);
  ASSERT_TRUE(tile.isValid());
  QTest::mouseClick(panel.view()->viewport(), Qt::LeftButton, Qt::NoModifier, tile.center());
  QTest::mouseDClick(panel.view()->viewport(), Qt::LeftButton, Qt::NoModifier, tile.center());
  ASSERT_EQ(activated.count(), 1);
  EXPECT_EQ(activated.front().at(0).toString(),
            panel.view()->model()->data(first, LibraryListModel::KeyRole).toString());

  panel.focus_category(QStringLiteral("Prawko | Gotowe drogi"));
  EXPECT_TRUE(instructions->text().contains(QStringLiteral("Enter")));
  panel.focus_category(QStringLiteral("Materials")); // hidden, no longer navigable
  EXPECT_TRUE(instructions->text().contains(QStringLiteral("Enter")));
}

TEST(LibraryPanel, CatalogueContainsOnlyEighteenPrawkoItems) {
  LibraryPanel panel(populated_model());
  const QAbstractItemModel* visible = panel.view()->model();
  ASSERT_NE(visible, nullptr);
  ASSERT_EQ(visible->rowCount(), 18); // 6 roads + 6 surface styles + 6 traffic actors
  auto* combo = panel.category_combo();
  ASSERT_NE(combo, nullptr);
  ASSERT_EQ(combo->count(), 4);
  EXPECT_TRUE(combo->itemData(0).toString().isEmpty());
  EXPECT_EQ(combo->itemData(1).toString(), QStringLiteral("Prawko | Gotowe drogi"));
  EXPECT_EQ(combo->itemData(2).toString(), QStringLiteral("Prawko | Krawężniki i chodniki"));
  EXPECT_EQ(combo->itemData(3).toString(), kScenarioActorCategory);
  for (int i = 0; i < visible->rowCount(); ++i) {
    const QModelIndex index = visible->index(i, 0);
    const QString category = visible->data(index, LibraryListModel::CategoryRole).toString();
    const QString key = visible->data(index, LibraryListModel::KeyRole).toString();
    EXPECT_TRUE(category == QStringLiteral("Prawko | Gotowe drogi") ||
                category == QStringLiteral("Prawko | Krawężniki i chodniki") ||
                category == kScenarioActorCategory);
    EXPECT_TRUE(key.startsWith(QStringLiteral("road.prawko.")) ||
                key.startsWith(QStringLiteral("style.prawko.")) ||
                key.startsWith(QStringLiteral("actor.")));
    const QVariant decoration = visible->data(index, Qt::DecorationRole);
    EXPECT_FALSE(decoration.isNull());
  }
  EXPECT_EQ(populated_model().item_for_key(QStringLiteral("road.freeway")), nullptr);
  EXPECT_EQ(populated_model().item_for_key(QStringLiteral("prop.tree.pine")), nullptr);
}

TEST(LibraryPanel, SearchCannotRevealObsoleteCategories) {
  LibraryPanel panel(populated_model());
  auto* search = panel.findChild<QLineEdit*>(QStringLiteral("library_search"));
  ASSERT_NE(search, nullptr);
  for (const QString& query : {QStringLiteral("freeway"),
                               QStringLiteral("intersection"),
                               QStringLiteral("pine"),
                               QStringLiteral("Traffic light")}) {
    search->setText(query);
    EXPECT_EQ(panel.view()->model()->rowCount(), 0) << query.toStdString();
  }
  search->setText(QStringLiteral("Samochód"));
  EXPECT_EQ(panel.view()->model()->rowCount(), 1);
  search->clear();
  EXPECT_EQ(panel.view()->model()->rowCount(), 18);
}

TEST(LibraryPanel, CategoriesFilterTheGridAndCombineWithSearch) {
  LibraryPanel panel(populated_model());
  auto* combo = panel.category_combo();
  auto* search = panel.findChild<QLineEdit*>(QStringLiteral("library_search"));
  ASSERT_NE(combo, nullptr);
  ASSERT_NE(search, nullptr);
  for (int index = 1; index < 4; ++index) {
    combo->setCurrentIndex(index);
    EXPECT_EQ(panel.view()->model()->rowCount(), 6);
    for (int row = 0; row < 6; ++row) {
      EXPECT_EQ(panel.view()
                    ->model()
                    ->data(panel.view()->model()->index(row, 0), LibraryListModel::CategoryRole)
                    .toString(),
                combo->currentData().toString());
    }
  }
  combo->setCurrentIndex(1);
  search->setText(QStringLiteral("chodnik"));
  EXPECT_GT(panel.view()->model()->rowCount(), 0);
  search->setText(QStringLiteral("Pieszy"));
  EXPECT_EQ(panel.view()->model()->rowCount(), 0);
  combo->setCurrentIndex(3);
  EXPECT_EQ(panel.view()->model()->rowCount(), 1);
  combo->setCurrentIndex(0);
  search->clear();
  EXPECT_EQ(panel.view()->model()->rowCount(), 18);
}

TEST(LibraryPanel, SelectingAVisibleItemEmitsCurrentChanged) {
  LibraryPanel panel(populated_model());
  QSignalSpy spy(&panel, &LibraryPanel::asset_current_changed);
  ASSERT_TRUE(spy.isValid());
  const QModelIndex first = panel.view()->model()->index(0, 0);
  panel.view()->setCurrentIndex(first);
  ASSERT_EQ(spy.count(), 1);
  EXPECT_TRUE(spy.at(0).at(0).toString().startsWith(QStringLiteral("road.prawko.")));
}

TEST(LibraryPanel, FocusCategoryClearsSearchAndHidesOldCategories) {
  LibraryPanel panel(populated_model());
  auto* search = panel.findChild<QLineEdit*>(QStringLiteral("library_search"));
  ASSERT_NE(search, nullptr);
  search->setText(QStringLiteral("Samochód"));
  ASSERT_EQ(panel.view()->model()->rowCount(), 1);
  panel.focus_category(QStringLiteral("Prawko | Gotowe drogi"));
  EXPECT_TRUE(search->text().isEmpty());
  EXPECT_EQ(panel.view()->model()->rowCount(), 6);
  ASSERT_TRUE(panel.view()->currentIndex().isValid());
  EXPECT_EQ(panel.view()
                ->model()
                ->data(panel.view()->currentIndex(), LibraryListModel::CategoryRole)
                .toString(),
            QStringLiteral("Prawko | Gotowe drogi"));
  panel.focus_category(QStringLiteral("Props"));
  EXPECT_EQ(panel.view()->model()->rowCount(), 6);
  EXPECT_EQ(panel.category_combo()->currentData().toString(),
            QStringLiteral("Prawko | Gotowe drogi"));
}

TEST(LibraryPanel, LegacyProjectOverlayCannotLeakIntoCuratedPalette) {
  LibraryListModel model;
  const auto builtin =
      LibraryManifest::load(std::filesystem::path(RM_ASSETS_DIR) / "library" / "manifest.json");
  ASSERT_TRUE(builtin.has_value());
  model.set_manifest(*builtin);
  const auto overlay = LibraryManifest::parse(QByteArray(R"({
    "manifest_version": 1,
    "items": [{"key":"project.old","label":"Old signal","category":"Signals",
               "create":{"kind":"signal","signal":"us.r1_1"}}]
  })"));
  ASSERT_TRUE(overlay.has_value());
  model.set_overlay(*overlay, QString());
  EXPECT_NE(model.item_for_key(QStringLiteral("project.old")), nullptr);
  LibraryPanel panel(model);
  EXPECT_EQ(panel.view()->model()->rowCount(), 18);
  EXPECT_EQ(panel.view()->contextMenuPolicy(), Qt::NoContextMenu);
  auto* search = panel.findChild<QLineEdit*>(QStringLiteral("library_search"));
  ASSERT_NE(search, nullptr);
  search->setText(QStringLiteral("Old signal"));
  EXPECT_EQ(panel.view()->model()->rowCount(), 0);
}

TEST(LibraryPanel, ActorWithoutThumbnailStillHasAGlyph) {
  LibraryPanel panel(populated_model());
  panel.focus_category(kScenarioActorCategory);
  ASSERT_EQ(panel.view()->model()->rowCount(), 6);
  const QModelIndex first = panel.view()->model()->index(0, 0);
  const QVariant decoration = panel.view()->model()->data(first, Qt::DecorationRole);
  EXPECT_TRUE(decoration.isValid());
  EXPECT_FALSE(decoration.value<QIcon>().isNull());
}

} // namespace
} // namespace roadmaker::editor
