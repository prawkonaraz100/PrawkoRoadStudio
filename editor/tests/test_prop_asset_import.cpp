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

// Importing a user's own glTF/GLB as a project prop (p6-s8, #322 · ADR-0013).
//
// The claim these tests exist to hold is that NOTHING downstream of the importer
// changed: once a model is registered, the mesh builder instances it, the prop
// tools place it and the exporters draw it exactly as they do a bundled prop.

#include "roadmaker/assets/prop_library.hpp"
#include "roadmaker/assets/prop_import.hpp"
#include "roadmaker/edit/operations.hpp"
#include "roadmaker/mesh/mesh_builder.hpp"
#include "roadmaker/road/authoring.hpp"
#include "roadmaker/road/network.hpp"
#include "roadmaker/xodr/reader.hpp"
#include "roadmaker/xodr/writer.hpp"

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUndoStack>
#include <algorithm>
#include <filesystem>
#include <string>
#include <tuple>
#include <vector>

#include "document/asset_import.hpp"
#include "document/document.hpp"
#include "document/library_drop.hpp"
#include "document/library_manifest.hpp"
#include "document/prop_placement.hpp"

namespace roadmaker::editor {
namespace {

std::filesystem::path fixture(const char* name) {
  return std::filesystem::path(RM_GLTF_FIXTURES_DIR) / name;
}

std::filesystem::path fs_path(const QString& path) {
  return std::filesystem::path(path.toStdString());
}

AssetImportRequest request_for(const std::filesystem::path& source, const QString& label) {
  AssetImportRequest request;
  request.source = source;
  request.label = label;
  request.category = QStringLiteral("Props");
  request.license = QStringLiteral("CC0 / own work");
  return request;
}

/// The overlay is process-wide, so every case here has to put it back.
class PropAssetImport : public testing::Test {
protected:
  void TearDown() override { props::clear_project_models(); }
};

} // namespace

TEST_F(PropAssetImport, AModelIsClassifiedAsAPropRatherThanAMaterial) {
  EXPECT_EQ(asset_import_kind("chair.glb"), AssetImportKind::Prop);
  EXPECT_EQ(asset_import_kind("chair.gltf"), AssetImportKind::Prop);
  EXPECT_EQ(asset_import_kind("brick.png"), AssetImportKind::Material);
  // Still refused by name, per ADR-0013's closed list.
  EXPECT_FALSE(asset_import_kind("chair.obj").has_value());
  EXPECT_FALSE(asset_import_kind("chair.fbx").has_value());
  // And the dialog filter offers both kinds now.
  const QString filter = asset_import_filter();
  EXPECT_TRUE(filter.contains(QStringLiteral("*.glb")));
  EXPECT_TRUE(filter.contains(QStringLiteral("*.png")));
}

TEST_F(PropAssetImport, CopiesTheModelIntoTheProjectAndDescribesTheAsset) {
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported =
      import_prop_asset(fs_path(project.path()),
                        request_for(fixture("textured_box.glb"), QStringLiteral("Wooden Crate")));
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  const auto& [item, result, diagnostics] = *imported;

  EXPECT_EQ(result.slug, QStringLiteral("wooden_crate"));
  EXPECT_EQ(result.key, QStringLiteral("prop.wooden_crate"));
  EXPECT_EQ(result.copied_to, fs_path(project.path()) / "assets" / "props" / "wooden_crate.glb");
  EXPECT_TRUE(std::filesystem::is_regular_file(result.copied_to));

  // `kind: tree` is the create-intent tag for EVERY point prop; the OpenDRIVE
  // class comes from the model, not this tag.
  EXPECT_EQ(item.kind, LibraryItem::Kind::Tree);
  EXPECT_EQ(item.model, QStringLiteral("wooden_crate"));
  EXPECT_EQ(item.category, QStringLiteral("Props"));
  // No thumbnail — #322 puts render-to-thumbnail out of scope (#509); the panel
  // falls back to a themed glyph.
  EXPECT_TRUE(item.thumbnail.isEmpty());

  // Provenance and the model file travel in create_raw, which to_json re-emits
  // verbatim — the same mechanism #336 used for default_scale.
  EXPECT_EQ(item.create_raw.value(QStringLiteral("model_file")).toString(),
            QStringLiteral("assets/props/wooden_crate.glb"));
  EXPECT_EQ(item.create_raw.value(QStringLiteral("license")).toString(),
            QStringLiteral("CC0 / own work"));
  EXPECT_FALSE(item.create_raw.value(QStringLiteral("source")).toString().isEmpty());

  // UV and the actual pixel bytes survive rather than being averaged.
  const bool preserved =
      std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& d) {
        return d.message.find("original UV0") != std::string::npos;
      });
  EXPECT_TRUE(preserved) << "the textured fixture must retain its authored image";
}

TEST_F(PropAssetImport, AModelIsReadBeforeItIsCopied) {
  // ★ A file that cannot become a prop must not leave bytes inside the project.
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported = import_prop_asset(
      fs_path(project.path()), request_for(fixture("truncated.glb"), QStringLiteral("Broken")));
  ASSERT_FALSE(imported.has_value());
  EXPECT_FALSE(
      std::filesystem::exists(fs_path(project.path()) / "assets" / "props" / "broken.glb"));
}

TEST_F(PropAssetImport, ACollidingNameIsSuffixedRatherThanOverwritten) {
  // Stronger reason than for materials: overwriting a slug already placed in a
  // scene would silently swap the geometry under it.
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto one = import_prop_asset(
      fs_path(project.path()), request_for(fixture("textured_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(one.has_value()) << one.error().message;
  const auto two = import_prop_asset(
      fs_path(project.path()), request_for(fixture("factor_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(two.has_value()) << two.error().message;
  EXPECT_EQ(std::get<1>(*one).slug, QStringLiteral("crate"));
  EXPECT_EQ(std::get<1>(*two).slug, QStringLiteral("crate_2"));
  EXPECT_TRUE(std::get<1>(*two).renamed);
  EXPECT_TRUE(std::filesystem::is_regular_file(std::get<1>(*one).copied_to));
}

TEST_F(PropAssetImport, AProjectsModelsAreRegisteredFromItsManifest) {
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported = import_prop_asset(
      fs_path(project.path()), request_for(fixture("textured_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(imported.has_value()) << imported.error().message;

  LibraryManifest manifest;
  manifest.upsert(std::get<0>(*imported));

  // Before registration the model does not resolve at all.
  EXPECT_EQ(props::model("crate"), nullptr);

  const std::vector<Diagnostic> diagnostics =
      register_project_prop_models(fs_path(project.path()), manifest);

  const props::PropModel* model = props::model("crate");
  ASSERT_NE(model, nullptr);
  EXPECT_EQ(model->id, "crate");
  EXPECT_TRUE(props::is_project_model("crate"));
  EXPECT_GT(model->height, 0.0);
  // The bundled catalogue is untouched.
  EXPECT_NE(props::model("tree_pine"), nullptr);
  // Valid UV0 and decoded baseColor image are preserved on every project
  // reopen: no legacy "flattened" warning is expected for this asset.
  EXPECT_EQ(model->textures.size(), 1U);
  EXPECT_EQ(model->parts.front().base_color_texture, 0);
  EXPECT_TRUE(diagnostics.empty());
}

TEST_F(PropAssetImport, AMissingModelFileIsReportedAndSkippedRatherThanFatal) {
  // ★ The project still opens. A broken asset is a diagnostic, not a reason to
  // refuse the whole library.
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  LibraryManifest manifest;
  LibraryItem present;
  present.key = QStringLiteral("prop.gone");
  present.kind = LibraryItem::Kind::Tree;
  present.model = QStringLiteral("gone");
  QJsonObject create;
  create[QStringLiteral("kind")] = QStringLiteral("tree");
  create[QStringLiteral("model")] = QStringLiteral("gone");
  create[QStringLiteral("model_file")] = QStringLiteral("assets/props/gone.glb");
  present.create_raw = create;
  manifest.upsert(present);

  const std::vector<Diagnostic> diagnostics =
      register_project_prop_models(fs_path(project.path()), manifest);

  EXPECT_EQ(props::model("gone"), nullptr);
  ASSERT_FALSE(diagnostics.empty());
  const bool named = std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& d) {
    return d.message.find("gone") != std::string::npos &&
           d.message.find("will not") != std::string::npos;
  });
  EXPECT_TRUE(named) << "the diagnostic has to name the asset that will not draw";
}

TEST_F(PropAssetImport, ABundledPropItemIsLeftToTheCompiledInCatalogue) {
  // An items[] row with no model_file is a bundled prop, and must not be treated
  // as a missing project asset.
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  LibraryManifest manifest;
  LibraryItem bundled;
  bundled.key = QStringLiteral("prop.tree.pine");
  bundled.kind = LibraryItem::Kind::Tree;
  bundled.model = QStringLiteral("tree_pine");
  manifest.upsert(bundled);

  const std::vector<Diagnostic> diagnostics =
      register_project_prop_models(fs_path(project.path()), manifest);
  EXPECT_TRUE(diagnostics.empty());
  EXPECT_NE(props::model("tree_pine"), nullptr);
  EXPECT_FALSE(props::is_project_model("tree_pine"));
}

TEST_F(PropAssetImport, AnImportedPropPlacesAndMeshesLikeABundledOne) {
  // ★ THE CLAIM OF THE WHOLE SPRINT: nothing downstream changed. This goes through
  // the ordinary placement path (make_prop_object, the same call every prop tool
  // and the Library drop make) and then the ordinary mesh builder.
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported = import_prop_asset(
      fs_path(project.path()), request_for(fixture("textured_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  const LibraryItem& item = std::get<0>(*imported);

  LibraryManifest manifest;
  manifest.upsert(item);
  // The returned diagnostics carry the flatten warning, which its own case
  // asserts; here only the registration matters.
  const std::vector<Diagnostic> ignored =
      register_project_prop_models(fs_path(project.path()), manifest);
  (void)ignored;
  const props::PropModel* model = props::model("crate");
  ASSERT_NE(model, nullptr);

  // The placement path gates on the model resolving, so this is also the check
  // that an imported asset is placeable at all.
  EXPECT_TRUE(is_prop_asset(item));

  RoadNetwork network;
  const std::vector<Waypoint> waypoints{Waypoint{.x = 0.0, .y = 0.0},
                                        Waypoint{.x = 60.0, .y = 0.0}};
  auto authored = author_clothoid_road(network, waypoints, LaneProfile::urban_sidewalk());
  ASSERT_TRUE(authored.has_value()) << authored.error().message;
  const RoadId road = *authored;
  Object placed = make_prop_object(item, "1", 30.0, -6.0);
  EXPECT_EQ(placed.name, "crate") << "the object's @name IS the model id";
  EXPECT_TRUE(placed.height.has_value());
  EXPECT_NEAR(*placed.height, model->height, 1e-9);
  (void)network.add_object(road, placed);

  const NetworkMesh mesh = build_network_mesh(network);
  ASSERT_EQ(mesh.objects.size(), 1U);
  EXPECT_EQ(mesh.objects.front().model_id, "crate");
  // @height / model height, the same rule a bundled prop obeys (#335).
  EXPECT_NEAR(mesh.objects.front().scale, 1.0, 1e-9);

  // And when the project closes, the same scene meshes to nothing — the gap #508
  // tracks, pinned so the follow-up has a test to flip.
  props::clear_project_models();
  EXPECT_TRUE(build_network_mesh(network).objects.empty());
}

TEST_F(PropAssetImport, ScalingAnImportedPropUsesTheDeclaredHeight) {
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported = import_prop_asset(
      fs_path(project.path()), request_for(fixture("factor_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  LibraryManifest manifest;
  manifest.upsert(std::get<0>(*imported));
  // The returned diagnostics carry the flatten warning, which its own case
  // asserts; here only the registration matters.
  const std::vector<Diagnostic> ignored =
      register_project_prop_models(fs_path(project.path()), manifest);
  (void)ignored;
  const props::PropModel* model = props::model("crate");
  ASSERT_NE(model, nullptr);

  RoadNetwork network;
  const std::vector<Waypoint> waypoints{Waypoint{.x = 0.0, .y = 0.0},
                                        Waypoint{.x = 60.0, .y = 0.0}};
  auto authored = author_clothoid_road(network, waypoints, LaneProfile::urban_sidewalk());
  ASSERT_TRUE(authored.has_value()) << authored.error().message;
  const RoadId road = *authored;
  Object placed = make_prop_object(std::get<0>(*imported), "1", 30.0, -6.0);
  placed.height = model->height * 2.5;
  (void)network.add_object(road, placed);

  const NetworkMesh mesh = build_network_mesh(network);
  ASSERT_EQ(mesh.objects.size(), 1U);
  EXPECT_NEAR(mesh.objects.front().scale, 2.5, 1e-9);
}

TEST_F(PropAssetImport, TheItemRoundTripsThroughTheManifest) {
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  const auto imported = import_prop_asset(
      fs_path(project.path()), request_for(fixture("textured_box.glb"), QStringLiteral("Crate")));
  ASSERT_TRUE(imported.has_value()) << imported.error().message;

  LibraryManifest manifest;
  manifest.upsert(std::get<0>(*imported));
  const auto path = fs_path(project.path()) / "assets" / "library" / "manifest.json";
  std::filesystem::create_directories(path.parent_path());
  ASSERT_TRUE(manifest.save(path).has_value());

  const auto reloaded = LibraryManifest::load(path);
  ASSERT_TRUE(reloaded.has_value()) << reloaded.error().message;
  ASSERT_EQ(reloaded->items().size(), 1U);
  const LibraryItem& item = reloaded->items().front();
  EXPECT_EQ(item.kind, LibraryItem::Kind::Tree);
  EXPECT_EQ(item.model, QStringLiteral("crate"));
  EXPECT_EQ(item.create_raw.value(QStringLiteral("model_file")).toString(),
            QStringLiteral("assets/props/crate.glb"));
  EXPECT_EQ(item.create_raw.value(QStringLiteral("license")).toString(),
            QStringLiteral("CC0 / own work"));

  // And the reloaded item still registers, which is what "survives close and
  // reopen" actually means.
  const std::vector<Diagnostic> diagnostics =
      register_project_prop_models(fs_path(project.path()), *reloaded);
  EXPECT_NE(props::model("crate"), nullptr);
  const bool broken = std::any_of(diagnostics.begin(), diagnostics.end(), [](const Diagnostic& d) {
    return d.message.find("could not be loaded") != std::string::npos;
  });
  EXPECT_FALSE(broken);
}

TEST_F(PropAssetImport, PrawkoA1BlenderShelterKeepsAlphaAndRoughnessWhileReportingTextureLimits) {
  const auto source = std::filesystem::path(RM_ASSETS_DIR) /
      "prawko/blender/wiata_przystankowa/prawko_wiata_przystankowa.glb";
  ASSERT_TRUE(std::filesystem::is_regular_file(source));
  auto imported = props::import_prop_model(source, "prawko_wiata_przystankowa");
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  EXPECT_GT(imported->model.parts.size(), 80U);
  EXPECT_LE(imported->model.parts.size(), props::kMaxPropParts);
  EXPECT_NEAR(imported->model.height, 2.77, 0.05);
  EXPECT_GT(imported->model.radius, 2.3);
  std::size_t triangle_count = 0;
  bool glass_found = false;
  bool textured_found = false;
  bool metal_found = false;
  std::size_t uv_meshes = 0;
  EXPECT_EQ(imported->model.textures.size(), 2U)
      << "One RGBA image for each authored albedo map, not per wood/concrete piece";
  for (const auto& texture : imported->model.textures) {
    EXPECT_EQ(texture.width, 256);
    EXPECT_EQ(texture.height, 256);
    EXPECT_EQ(texture.rgba.size(), 256U * 256U * 4U);
  }
  for (const props::PropPart& part : imported->model.parts) {
    triangle_count += part.indices.size() / 3;
    EXPECT_FALSE(part.positions.empty());
    if (part.base_color_texture >= 0) {
      ++uv_meshes;
      EXPECT_EQ(part.texcoords.size(), (part.positions.size() / 3U) * 2U)
          << "glTF UV0 must follow the same vertices as geometry";
      EXPECT_LT(static_cast<std::size_t>(part.base_color_texture),
                imported->model.textures.size());
    }
    if (part.name.find("Clear safety glass") != std::string::npos) {
      glass_found = true;
      EXPECT_NEAR(part.opacity, 0.23, 0.04);
      EXPECT_LT(part.roughness, 0.2);
      EXPECT_EQ(part.alpha_mode, 2);
      EXPECT_NEAR(part.transmission, 0.64F, 0.04F);
      EXPECT_NEAR(part.ior, 1.45F, 0.03F);
    }
    if (part.name.find("Brushed stainless") != std::string::npos) {
      metal_found = true;
      EXPECT_TRUE(part.gltf_pbr);
      EXPECT_GT(part.metallic, 0.8F);
      EXPECT_LT(part.roughness, 0.4);
    }
  }
  for (const Diagnostic& diagnostic : imported->diagnostics) {
    textured_found |= diagnostic.message.find("flattened") != std::string::npos;
  }
  EXPECT_TRUE(glass_found);
  EXPECT_TRUE(metal_found);
  EXPECT_FALSE(textured_found) << "Real UV materials must no longer be flattened";
  EXPECT_GT(uv_meshes, 6U) << "Textured wood and concrete must retain UV0";
  EXPECT_GT(triangle_count, 5000U);
  EXPECT_LT(triangle_count, props::kMaxPropTriangles);
}

TEST_F(PropAssetImport, R6b2gGlbEmissiveLensIsImportedFromAuthorMaterial) {
  const auto source = std::filesystem::path(RM_ASSETS_DIR) /
      "prawko/blender/oprawa_r6b2g/r6b2g-latarnia-emisyjna.glb";
  ASSERT_TRUE(std::filesystem::is_regular_file(source));
  const auto imported=props::import_prop_model(source,"r6b2g-latarnia-emisyjna");
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  int glowing=0;
  for (const auto& part:imported->model.parts) {
    if (part.emissive[0] > 0.5F && part.emissive[1] > 0.5F) {
      ++glowing;
      EXPECT_TRUE(part.gltf_pbr);
      EXPECT_LT(part.emissive[2],part.emissive[0]);
      EXPECT_GT(part.emissive[2],0.2F);
    } else {
      EXPECT_FLOAT_EQ(part.emissive[0],0.0F);
      EXPECT_FLOAT_EQ(part.emissive[1],0.0F);
    }
  }
  EXPECT_EQ(glowing,1) << "Emission belongs ONLY to the physical lens material";
}

TEST_F(PropAssetImport, PrawkoR6b2dAuthoredPbrAssetsPreserveUvNormalAndOrm) {
  for (const char* stem : {"r6b2d-przejscie-plyty-12m",
                           "r6b2d-styk-chodnik-ziemia-20m"}) {
    const auto source = std::filesystem::path(RM_ASSETS_DIR) /
        "prawko/blender/styki_r6b2d" / (std::string(stem) + ".glb");
    ASSERT_TRUE(std::filesystem::is_regular_file(source)) << stem;
    auto imported = props::import_prop_model(source, stem);
    ASSERT_TRUE(imported.has_value()) << stem << ": " << imported.error().message;
    ASSERT_GE(imported->model.textures.size(), 3U) << stem;
    std::size_t pbr_parts = 0;
    for (const auto& part : imported->model.parts) {
      if (part.base_color_texture < 0) continue;
      ASSERT_GE(part.normal_texture, 0) << stem << ": missing normal map";
      ASSERT_GE(part.metallic_roughness_texture, 0)
          << stem << ": missing glTF ORM roughness map";
      EXPECT_TRUE(part.gltf_pbr);
      EXPECT_EQ(part.texcoords.size(), part.positions.size() / 3U * 2U);
      EXPECT_LT(static_cast<std::size_t>(part.base_color_texture),
                imported->model.textures.size());
      EXPECT_LT(static_cast<std::size_t>(part.normal_texture),
                imported->model.textures.size());
      EXPECT_LT(static_cast<std::size_t>(part.metallic_roughness_texture),
                imported->model.textures.size());
      ++pbr_parts;
    }
    EXPECT_GE(pbr_parts, 1U) << stem;
    for (const auto& diagnostic : imported->diagnostics) {
      EXPECT_EQ(diagnostic.message.find("flattened"), std::string::npos);
    }
  }
}

// This is the ACTUAL asset authored and exported by Blender 5.3, not a
// programmatically fabricated fixture. Exercises its real importer, then the
// same Library-drop and pose commands as the mouse and Properties panel.
TEST_F(PropAssetImport, PrawkoBlenderBarrierDropsMovesRotatesAndSurvivesReload) {
  const auto source =
      std::filesystem::path(RM_ASSETS_DIR) / "prawko/blender/prawko_bariera_drogowa.glb";
  ASSERT_TRUE(std::filesystem::is_regular_file(source));
  QTemporaryDir project;
  ASSERT_TRUE(project.isValid());
  AssetImportRequest req = request_for(source, QStringLiteral("Prawko Bariera Drogowa"));
  req.category = QStringLiteral("Prawko | Obiekty 3D");
  auto imported = import_prop_asset(fs_path(project.path()), req);
  ASSERT_TRUE(imported.has_value()) << imported.error().message;
  const LibraryItem item = std::get<0>(*imported);
  EXPECT_EQ(item.category, QStringLiteral("Prawko | Obiekty 3D"));
  EXPECT_EQ(item.model, QStringLiteral("prawko_bariera_drogowa"));
  EXPECT_TRUE(std::filesystem::exists(std::get<1>(*imported).copied_to));
  LibraryManifest manifest;
  manifest.upsert(item);
  const auto path = fs_path(project.path()) / "assets/library/manifest.json";
  std::filesystem::create_directories(path.parent_path());
  ASSERT_TRUE(manifest.save(path).has_value());
  auto reloaded = LibraryManifest::load(path);
  ASSERT_TRUE(reloaded.has_value()) << reloaded.error().message;
  ASSERT_EQ(reloaded->items().size(), 1U);
  const auto registration = register_project_prop_models(fs_path(project.path()), *reloaded);
  (void)registration;
  const props::PropModel* model = props::model("prawko_bariera_drogowa");
  ASSERT_NE(model, nullptr);
  EXPECT_GT(model->height, 1.0);
  EXPECT_GT(model->radius, 0.5);
  EXPECT_GE(model->parts.size(), 10U);

  Document document;
  ASSERT_TRUE(document
                  .push_command(edit::create_road(
                      {{0.0, 0.0}, {100.0, 0.0}}, LaneProfile::two_lane_default(), "1"))
                  .has_value());
  auto drop = resolve_library_drop(reloaded->items().front(), document.network(), 42.0, -5.0);
  EXPECT_EQ(drop.kind, LibraryDropKind::Tree);
  ASSERT_NE(drop.command, nullptr);
  ASSERT_TRUE(document.push_command(std::move(drop.command)).has_value());
  ObjectId object_id{};
  document.network().for_each_object([&](ObjectId found, const Object&) { object_id = found; });
  ASSERT_TRUE(object_id.is_valid());
  const Object* before = document.network().object(object_id);
  ASSERT_NE(before, nullptr);
  EXPECT_EQ(before->name, "prawko_bariera_drogowa");
  const double initial_s = before->s;
  const double initial_hdg = before->hdg;
  auto move =
      move_object_command(document.network(), object_id, initial_s + 6.0, -6.0, 1.5707963267948966);
  ASSERT_NE(move, nullptr);
  ASSERT_TRUE(document.push_command(std::move(move)).has_value());
  const Object* moved = document.network().object(object_id);
  ASSERT_NE(moved, nullptr);
  EXPECT_NEAR(moved->s, initial_s + 6.0, 1e-6);
  EXPECT_NEAR(moved->t, -6.0, 1e-6);
  EXPECT_NEAR(moved->hdg, 1.5707963267948966, 1e-6);
  EXPECT_EQ(build_network_mesh(document.network()).objects.size(), 1U);
  document.undo_stack()->undo();
  EXPECT_NEAR(document.network().object(object_id)->hdg, initial_hdg, 1e-6);
  document.undo_stack()->redo();
  EXPECT_NEAR(document.network().object(object_id)->hdg, 1.5707963267948966, 1e-6);

  const auto scene = fs_path(project.path()) / "barrier.xodr";
  const auto saved = save_xodr(document.network(), scene, "Prawko Barrier Proof");
  ASSERT_TRUE(saved.has_value()) << saved.error().message;
  const auto reopened = load_xodr(scene);
  ASSERT_TRUE(reopened.has_value()) << reopened.error().message;
  int count = 0;
  reopened->network.for_each_object([&](ObjectId, const Object& obj) {
    ++count;
    EXPECT_EQ(obj.name, "prawko_bariera_drogowa");
    EXPECT_NEAR(obj.s, initial_s + 6.0, 1e-4);
    EXPECT_NEAR(obj.hdg, 1.5707963267948966, 1e-4);
  });
  EXPECT_EQ(count, 1);
  EXPECT_EQ(build_network_mesh(reopened->network).objects.size(), 1U);
}

TEST_F(PropAssetImport, PrawkoDemoSceneReloadsItsOwnBlenderModel) {
  const std::filesystem::path project =
      std::filesystem::path(RM_REPO_ROOT_DIR) / "tests/fixtures/Prawko-Bariera-Demo";
  Document loaded_before_registration;
  ASSERT_TRUE(loaded_before_registration.load(project / "bariera-ustawiona.xodr").has_value());
  EXPECT_EQ(loaded_before_registration.mesh().objects.size(), 0U);
  const auto manifest = LibraryManifest::load(project / "assets/library/manifest.json");
  ASSERT_TRUE(manifest.has_value()) << manifest.error().message;
  const auto diagnostics = register_project_prop_models(project, *manifest);
  (void)diagnostics;
  loaded_before_registration.refresh_registered_project_models();
  EXPECT_EQ(loaded_before_registration.mesh().objects.size(), 1U);
  ASSERT_NE(props::model("prawko_bariera_drogowa"), nullptr);
  const auto scene = load_xodr(project / "bariera-ustawiona.xodr");
  ASSERT_TRUE(scene.has_value()) << scene.error().message;
  std::size_t objects = 0;
  scene->network.for_each_object([&](ObjectId, const Object& obj) {
    ++objects;
    EXPECT_EQ(obj.name, "prawko_bariera_drogowa");
    EXPECT_NEAR(obj.hdg, 1.5707963267949, 1e-4);
  });
  EXPECT_EQ(objects, 1U);
  const NetworkMesh mesh = build_network_mesh(scene->network);
  EXPECT_EQ(mesh.objects.size(), 1U);
  if (mesh.objects.size() == 1U) {
    EXPECT_EQ(mesh.objects.front().model_id, "prawko_bariera_drogowa");
  }
}

} // namespace roadmaker::editor
