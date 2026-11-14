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

#include "tools/route_tool.hpp"

#include "roadmaker/osc/edit.hpp"
#include "roadmaker/osc/route.hpp"

#include <QString>
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <variant>

#include "document/document.hpp"
#include "document/route_preview.hpp"
#include "document/selection_model.hpp"
#include "viewport/projection.hpp"

namespace roadmaker::editor {
namespace {

/// A drag has to move this far before it stops being a click [m] — the same
/// threshold the Actor tool uses, for the same tremor.
constexpr double kDragThreshold = 0.25;

/// How near the cursor must be to a laid waypoint for a press to grab it [m].
/// Matches kActorGrabRadius: the two knobs live on the same lanes at the same
/// zoom, and two different grab feels would read as one of them being broken.
constexpr double kWaypointGrabRadius = 3.0;
constexpr int kRemoveLastPoint = -2;
constexpr double kWalkingSpeed = 1.4; // m/s
constexpr double kDrivingSpeed = 30.0 / 3.6;
constexpr double kStopAtEndSeconds = 0.5;

std::array<double, 3> coordinates(const osc::WorldPosition& position) {
  return {position.x, position.y, position.z};
}

std::size_t editable_point_count(const osc::TimedPolyline& path) {
  const auto count = path.vertices.size();
  return count >= 3 && coordinates(path.vertices.back().position) ==
                           coordinates(path.vertices[count - 2].position)
             ? count - 1
             : count;
}

bool has_story_trajectory(const osc::Scenario& scenario, const std::string& actor) {
  for (const auto& story : scenario.storyboard.stories) {
    for (const auto& act : story.acts) {
      for (const auto& group : act.maneuver_groups) {
        if (std::ranges::find(group.actors, actor, &osc::EntityRef::entity_ref) ==
            group.actors.end()) {
          continue;
        }
        for (const auto& maneuver : group.maneuvers) {
          for (const auto& event : maneuver.events) {
            for (const auto& action : event.actions) {
              const auto* entry = std::get_if<osc::PrivateAction>(&action.action);
              if (!entry) continue;
              if (entry->routing && entry->routing->timed_polyline) return true;
              if (std::ranges::any_of(entry->preserved.children, [](const std::string& xml) {
                    return xml.find("<FollowTrajectoryAction") != std::string::npos;
                  })) {
                return true;
              }
            }
          }
        }
      }
    }
  }
  return false;
}

bool has_preserved_init_routing(const osc::Scenario& scenario, const std::string& actor) {
  for (const auto& entry : scenario.storyboard.init.actions.privates) {
    if (entry.entity_ref != actor) continue;
    for (const auto& action : entry.actions) {
      if (std::ranges::any_of(action.preserved.children, [](const std::string& xml) {
            return xml.find("<RoutingAction") != std::string::npos;
          })) {
        return true;
      }
    }
  }
  return false;
}

/// A route name no other route in the scenario carries. The writer refuses a
/// duplicate (`naming.unique_element_names_on_same_level`), so uniqueness here
/// is what keeps a second actor's route from making the document unsavable.
std::string unique_route_name(const osc::Scenario& scenario, const std::string& base) {
  const auto taken = [&scenario](const std::string& name) {
    for (const osc::AssignedRoute& assigned : osc::assigned_routes(scenario)) {
      if (assigned.route->name == name) {
        return true;
      }
    }
    return false;
  };
  if (!taken(base)) {
    return base;
  }
  for (int counter = 2;; ++counter) {
    std::string candidate = base + std::to_string(counter);
    if (!taken(candidate)) {
      return candidate;
    }
  }
}

osc::RouteWaypoint waypoint_from(const LaneAnchor& anchor) {
  return osc::RouteWaypoint{.route_strategy = std::string(osc::kDefaultRouteStrategy),
                            .position = osc::Position{to_lane_position(anchor)},
                            .preserved = {}};
}

} // namespace

RouteTool::RouteTool(Document& document, SelectionModel& selection, QObject* parent)
    : Tool(parent), document_(document), selection_(selection) {}

void RouteTool::activate() {
  // An actor already selected IS the answer to "whose route?" — re-asking with
  // a click would make the Scenario panel selection meaningless here.
  const std::vector<std::string> selected = selection_.selected_actors();
  if (!selected.empty()) {
    target_ = selected.front();
  }
}

void RouteTool::begin_from_actor(const std::string& actor) {
  target_ = actor;
  pending_.reset();
  hovered_.reset();
  walking_hover_.reset();
  hover_hint_.clear();
  press_.reset();
  start_at_actor_ = true;
  emit preview_changed();
}

void RouteTool::deactivate() {
  target_.clear();
  pending_.reset();
  hovered_.reset();
  walking_hover_.reset();
  screen_.reset();
  start_at_actor_ = false;
  hover_hint_.clear();
  press_.reset();
  emit preview_changed();
}

bool RouteTool::pedestrian() const {
  const auto& objects = document_.scenario().entities.scenario_objects;
  const auto object = std::ranges::find(objects, target_, &osc::ScenarioObject::name);
  return object != objects.end() && std::holds_alternative<osc::Pedestrian>(object->entity_object);
}

const osc::TimedPolyline* RouteTool::walking_path() const {
  for (const auto& entry : document_.scenario().storyboard.init.actions.privates) {
    if (entry.entity_ref != target_) continue;
    for (const auto& action : entry.actions) {
      if (action.routing && action.routing->timed_polyline) return &*action.routing->timed_polyline;
    }
  }
  return nullptr;
}

void RouteTool::edit_walking_path(int waypoint, const std::array<double, 3>& point) {
  const auto* existing = walking_path();
  if (!existing && has_story_trajectory(document_.scenario(), target_)) {
    emit toast_requested(tr("Ten uczestnik ma już ścieżkę przypisaną do zdarzenia w Storyboard."),
                         ToastSeverity::Warning);
    return;
  }
  if (!existing && has_preserved_init_routing(document_.scenario(), target_)) {
    emit toast_requested(tr("Ten uczestnik ma już zaimportowaną trasę."), ToastSeverity::Warning);
    return;
  }
  const auto* initial = actor_init_position(document_.scenario(), target_);
  const auto start = initial ? actor_world_pose(document_.network(), *initial) : std::nullopt;
  if (!start) return;
  osc::TimedPolyline path;
  path.name = target_ + "WalkingPath";
  if (existing) {
    path = *existing;
    path.vertices.resize(editable_point_count(path));
  } else {
    path.vertices.push_back({0.0, osc::WorldPosition{.x = start->position[0],
                                                     .y = start->position[1],
                                                     .z = start->position[2]}});
  }
  if (waypoint == kRemoveLastPoint && path.vertices.size() > 2) {
    path.vertices.pop_back();
    waypoint = static_cast<int>(path.vertices.size() - 1);
  } else if (waypoint < 0) {
    if (waypoint == kRemoveLastPoint) {
      emit toast_requested(tr("Ścieżka potrzebuje punktu docelowego."), ToastSeverity::Info);
      return;
    }
    path.vertices.emplace_back();
    waypoint = static_cast<int>(path.vertices.size() - 1);
  }
  auto& position = path.vertices[static_cast<std::size_t>(waypoint)].position;
  position.x = point[0];
  position.y = point[1];
  position.z = point[2];
  for (std::size_t i = 1; i < path.vertices.size(); ++i) {
    auto& before = path.vertices[i - 1];
    auto& after = path.vertices[i];
    const double dx = after.position.x - before.position.x;
    const double dy = after.position.y - before.position.y;
    const double dz = after.position.z - before.position.z;
    const double distance = std::hypot(dx, dy, dz);
    if (distance < 0.05) {
      emit toast_requested(tr("Wybierz punkt dalej od poprzedniego."), ToastSeverity::Warning);
      return;
    }
    before.position.h = std::atan2(dy, dx);
    after.time = before.time + distance / kWalkingSpeed;
  }
  path.vertices.back().position.h = path.vertices[path.vertices.size() - 2].position.h;
  auto stop = path.vertices.back();
  stop.time += kStopAtEndSeconds;
  path.vertices.push_back(std::move(stop));
  const auto edited = document_.push_scenario_command(osc::edit::set_init_polyline(
      document_.scenario(), target_, std::move(path)));
  if (!edited) emit toast_requested(QString::fromStdString(edited.error().message),
                                    ToastSeverity::Warning);
}

const osc::Route* RouteTool::current_route() const {
  if (target_.empty()) {
    return nullptr;
  }
  for (const osc::AssignedRoute& assigned : osc::assigned_routes(document_.scenario())) {
    if (assigned.entity_ref == target_) {
      return assigned.route;
    }
  }
  return nullptr;
}

int RouteTool::waypoint_at(double x, double y) const {
  const osc::Route* route = current_route();
  std::vector<std::optional<ActorPose>> poses;
  if (const auto* path = walking_path(); pedestrian() && path) {
    for (std::size_t i = 0; i < editable_point_count(*path); ++i) {
      poses.push_back(actor_world_pose(document_.network(), osc::Position{path->vertices[i].position}));
    }
  } else if (route) {
    poses = route_waypoint_poses(document_.network(), *route);
  }
  int best = -1;
  double best_distance = screen_ ? 10.0 : kWaypointGrabRadius;
  for (std::size_t index = 0; index < poses.size(); ++index) {
    if (!poses[index].has_value()) {
      continue; // not drawn, so not grabbable — the actor hit test's own rule
    }
    const auto& point = poses[index]->position;
    double distance = std::hypot(point[0] - x, point[1] - y);
    if (screen_) {
      const auto pixel = project_to_screen(screen_->camera, point[0], point[1], point[2],
                                           screen_->width, screen_->height);
      if (!pixel) continue;
      distance = std::hypot((*pixel)[0] - screen_->px, (*pixel)[1] - screen_->py);
    }
    if (distance < best_distance) {
      best_distance = distance;
      best = static_cast<int>(index);
    }
  }
  return best;
}

bool RouteTool::mouse_press(const ToolEvent& event) {
  if ((event.buttons & Qt::LeftButton) == 0) {
    return false;
  }

  screen_ = event.screen;
  const auto surface = event.surface_point();
  if (!surface && !event.actor_hit) return false;
  const auto world = surface.value_or(std::array<double, 3>{});
  PressState state{.world_x = world[0], .world_y = world[1]};

  // An actor under the cursor wins over a waypoint: retargeting is the rarer
  // gesture, so it must not be shadowed by a waypoint that happens to sit on
  // the actor's own anchor. Same shared hit test as the Actor/Select tools.
  if (const auto hit = event.screen ? event.actor_hit :
          actor_at(document_.scenario(), document_.network(), world[0], world[1],
                   pedestrian() ? 0.55 : kActorGrabRadius)) {
    if (*hit != target_ || !pedestrian() || !walking_path()) state.actor = *hit;
  }
  if (state.actor.empty()) {
    state.waypoint = waypoint_at(world[0], world[1]);
  }

  press_ = state;
  return true;
}

bool RouteTool::mouse_move(const ToolEvent& event) {
  screen_ = event.screen;
  const auto world = event.surface_point();
  if (world) {
    cursor_x_ = (*world)[0];
    cursor_y_ = (*world)[1];
  }
  const bool walking = pedestrian();
  walking_hover_ = walking ? world : std::nullopt;
  hovered_ = world && !walking
                 ? nearest_lane_anchor(document_.network(), cursor_x_, cursor_y_)
                 : std::nullopt;
  hover_hint_ = world && !walking && !target_.empty() && !hovered_
                    ? actor_drop_hint(document_.network(), cursor_x_, cursor_y_)
                    : std::string{};

  if (press_.has_value() && press_->waypoint >= 0) {
    const double moved =
        std::hypot(cursor_x_ - press_->world_x, cursor_y_ - press_->world_y);
    if (world && moved > kDragThreshold) {
      press_->dragging = true;
    }
  }

  emit preview_changed();
  return true;
}

bool RouteTool::mouse_release(const ToolEvent& event) {
  if (!press_.has_value()) {
    return false;
  }
  const PressState state = *press_;
  press_.reset();

  // A press on an actor retargets the tool; the pending first waypoint (if
  // any) belonged to the previous target and is discarded with it.
  if (!state.actor.empty()) {
    selection_.select(SelectionEntry{.actor = state.actor});
    if (target_ != state.actor) {
      begin_from_actor(state.actor);
      start_at_actor_ = false;
    }
    return true;
  }

  const auto world = event.surface_point();
  if (!world) return true;
  if (pedestrian()) {
    if (state.waypoint < 0 || state.dragging) edit_walking_path(state.waypoint, *world);
    emit preview_changed();
    return true;
  }
  const std::optional<LaneAnchor> anchor =
      nearest_lane_anchor(document_.network(), (*world)[0], (*world)[1]);

  // A grabbed waypoint that actually travelled commits ONE set_route_waypoint.
  if (state.dragging && state.waypoint >= 0) {
    if (!anchor.has_value()) {
      emit toast_requested(QString::fromStdString(
                               actor_drop_hint(document_.network(), event.world_x, event.world_y)),
                           ToastSeverity::Warning);
      emit preview_changed();
      return true;
    }
    const auto moved = document_.push_scenario_command(
        osc::edit::set_route_waypoint(document_.scenario(),
                                      target_,
                                      static_cast<std::size_t>(state.waypoint),
                                      osc::Position{to_lane_position(*anchor)}));
    if (!moved.has_value()) {
      emit toast_requested(QString::fromStdString(moved.error().message), ToastSeverity::Error);
    }
    emit preview_changed();
    return true;
  }
  if (state.waypoint >= 0) {
    // A click on a waypoint that never became a drag: nothing to do — the knob
    // is already where the user left it.
    emit preview_changed();
    return true;
  }

  if (target_.empty()) {
    emit toast_requested(tr("Click an actor first — the route needs someone to drive it."),
                         ToastSeverity::Warning);
    emit preview_changed();
    return true;
  }
  if (!anchor.has_value()) {
    // Same refusal-with-a-reason the Actor tool gives: a waypoint off every
    // lane is a waypoint the resolver could only report as unresolvable.
    emit toast_requested(
        QString::fromStdString(actor_drop_hint(document_.network(), event.world_x, event.world_y)),
        ToastSeverity::Warning);
    emit preview_changed();
    return true;
  }

  if (const osc::Route* route = current_route()) {
    if (start_at_actor_) {
      auto extended = *route;
      extended.waypoints.push_back(waypoint_from(*anchor));
      if (!osc::resolve_route(document_.network(), extended).complete) {
        emit toast_requested(tr("Nie ma przejazdu do tego pasa. Wskaż połączony pas."),
                             ToastSeverity::Warning);
        return true;
      }
    }
    // ONE command appends the waypoint at the end.
    const auto appended = document_.push_scenario_command(osc::edit::insert_route_waypoint(
        document_.scenario(), target_, route->waypoints.size(), waypoint_from(*anchor)));
    if (!appended.has_value()) {
      emit toast_requested(QString::fromStdString(appended.error().message), ToastSeverity::Error);
    }
    emit preview_changed();
    return true;
  }

  const auto* initial = start_at_actor_ ? actor_init_position(document_.scenario(), target_) : nullptr;
  const auto* initial_lane = initial ? std::get_if<osc::LanePosition>(initial) : nullptr;
  if (!pending_.has_value() && !initial_lane) {
    // The schema needs two waypoints, so the first click commits nothing: the
    // document never holds a one-waypoint route it could not save.
    pending_ = anchor;
    emit preview_changed();
    return true;
  }

  osc::Route route;
  route.name = unique_route_name(document_.scenario(), target_ + "Route");
  route.waypoints.push_back(pending_ ? waypoint_from(*pending_) :
      osc::RouteWaypoint{.route_strategy = std::string(osc::kDefaultRouteStrategy),
                         .position = *initial_lane});
  route.waypoints.push_back(waypoint_from(*anchor));
  if (start_at_actor_ && !osc::resolve_route(document_.network(), route).complete) {
    emit toast_requested(tr("Nie ma przejazdu do tego pasa. Wskaż połączony pas."),
                         ToastSeverity::Warning);
    return true;
  }
  const auto assigned = document_.push_scenario_command(osc::edit::start_vehicle_route(
      document_.scenario(), target_, std::move(route), kDrivingSpeed));
  if (!assigned.has_value()) {
    emit toast_requested(QString::fromStdString(assigned.error().message), ToastSeverity::Error);
  } else {
    pending_.reset();
  }
  emit preview_changed();
  return true;
}

bool RouteTool::key_press(int key, Qt::KeyboardModifiers /*modifiers*/) {
  if (key == Qt::Key_Return || key == Qt::Key_Enter) {
    finish();
    return true;
  }
  if (key == Qt::Key_Escape) {
    // First Esc drops the uncommitted waypoint, a second drops the target —
    // stepping out of the gesture in the order it was stepped into.
    if (press_.has_value() || pending_.has_value()) {
      press_.reset();
      pending_.reset();
    } else {
      target_.clear();
    }
    emit preview_changed();
    return true;
  }
  if (key == Qt::Key_Backspace || key == Qt::Key_Delete) {
    if (pedestrian()) {
      if (const auto* path = walking_path()) {
        const auto count = editable_point_count(*path);
        edit_walking_path(kRemoveLastPoint,
                          coordinates(path->vertices[count > 2 ? count - 2 : 0].position));
        emit preview_changed();
        return true;
      }
    }
    const osc::Route* route = current_route();
    if (route == nullptr || route->waypoints.empty()) {
      return false;
    }
    // The kernel refuses removing below two ("delete the whole route
    // instead"); the refusal surfaces as a toast rather than being pre-checked
    // here — one rule, owned by the command layer.
    const auto removed = document_.push_scenario_command(osc::edit::remove_route_waypoint(
        document_.scenario(), target_, route->waypoints.size() - 1));
    if (!removed.has_value()) {
      emit toast_requested(QString::fromStdString(removed.error().message), ToastSeverity::Warning);
    }
    emit preview_changed();
    return true;
  }
  return false;
}

void RouteTool::finish() {
  if (pending_) {
    emit toast_requested(tr("Kliknij punkt docelowy lub anuluj klawiszem Esc."),
                         ToastSeverity::Info);
    return;
  }
  emit request_tool(ToolId::Select);
}

PreviewGeometry RouteTool::preview() const {
  PreviewGeometry geometry;
  const auto append_polyline = [](const std::vector<std::array<double, 3>>& path,
                                  std::vector<double>& out) {
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
      out.insert(
          out.end(),
          {path[i][0], path[i][1], path[i][2], path[i + 1][0], path[i + 1][1], path[i + 1][2]});
    }
  };

  if (pedestrian()) {
    std::vector<std::array<double, 3>> points;
    if (const auto* path = walking_path()) {
      const auto count = editable_point_count(*path);
      const int hovered = waypoint_at(cursor_x_, cursor_y_);
      for (std::size_t i = 0; i < count; ++i) {
        const bool dragged = press_ && press_->dragging && press_->waypoint == static_cast<int>(i);
        points.push_back(dragged && walking_hover_ ? *walking_hover_ :
                            coordinates(path->vertices[i].position));
        const auto& point = points.back();
        geometry.add_handle(point[0], point[1], point[2], HandleKind::Node,
                            dragged ? HandleState::Grabbed :
                            hovered == static_cast<int>(i) ? HandleState::Hovered : HandleState::Idle);
      }
    } else if (const auto* initial = actor_init_position(document_.scenario(), target_)) {
      if (const auto start = actor_world_pose(document_.network(), *initial)) {
        points.push_back(start->position);
      }
    }
    append_polyline(points, geometry.line_positions);
    if (walking_hover_ && !points.empty() && !(press_ && press_->dragging)) {
      append_polyline({points.back(), *walking_hover_}, geometry.dashed_line_positions);
      geometry.add_handle((*walking_hover_)[0], (*walking_hover_)[1], (*walking_hover_)[2],
                          HandleKind::Midpoint);
    }
    return geometry;
  }
  if (const osc::Route* route = current_route()) {
    // ★ THE RESOLVED ROUTE, not the clicks. Solid legs are what resolve_route
    // solved against the live network right now — after a road move they
    // follow it, after a lane deletion the solved stretches still draw and the
    // broken one visibly does not (GW-6 steps 7 and 8, in the viewport).
    const osc::ResolvedRoute resolved = osc::resolve_route(document_.network(), *route);
    for (const osc::RouteLeg& leg : resolved.legs) {
      append_polyline(route_leg_polyline(document_.network(), leg), geometry.line_positions);
    }

    const std::vector<std::optional<ActorPose>> poses =
        route_waypoint_poses(document_.network(), *route);
    if (!resolved.complete) {
      // The waypoint SEQUENCE, dashed, wherever resolution fell short — intent
      // where there is no solved path, so the gap is visible instead of the
      // route just looking shorter than it is.
      std::vector<std::array<double, 3>> sequence;
      for (const std::optional<ActorPose>& pose : poses) {
        if (pose.has_value()) {
          sequence.push_back(pose->position);
        }
      }
      append_polyline(sequence, geometry.dashed_line_positions);
    }

    const int hovered_waypoint = press_.has_value() && press_->waypoint >= 0
                                     ? press_->waypoint
                                     : waypoint_at(cursor_x_, cursor_y_);
    for (std::size_t index = 0; index < poses.size(); ++index) {
      if (!poses[index].has_value()) {
        continue;
      }
      const bool grabbed =
          press_.has_value() && press_->dragging && press_->waypoint == static_cast<int>(index);
      const bool hot = grabbed || hovered_waypoint == static_cast<int>(index);
      geometry.add_handle(poses[index]->position[0],
                          poses[index]->position[1],
                          poses[index]->position[2],
                          HandleKind::Node,
                          grabbed ? HandleState::Grabbed
                          : hot   ? HandleState::Hovered
                                  : HandleState::Idle);
    }
  }

  // The uncommitted first waypoint, plus its dashed reach to the cursor: the
  // route-to-be, drawn as guidance rather than as a solved path.
  if (pending_.has_value()) {
    if (const std::optional<ActorPose> pose = actor_world_pose(document_.network(), *pending_)) {
      geometry.add_handle(pose->position[0], pose->position[1], pose->position[2]);
      if (hovered_.has_value()) {
        if (const std::optional<ActorPose> hover =
                actor_world_pose(document_.network(), *hovered_)) {
          geometry.dashed_line_positions.insert(geometry.dashed_line_positions.end(),
                                                {pose->position[0],
                                                 pose->position[1],
                                                 pose->position[2],
                                                 hover->position[0],
                                                 hover->position[1],
                                                 hover->position[2]});
        }
      }
    }
  }
  if (!pending_ && start_at_actor_ && !current_route() && hovered_) {
    const auto* initial = actor_init_position(document_.scenario(), target_);
    if (initial) {
      const auto start = actor_world_pose(document_.network(), *initial);
      const auto end = actor_world_pose(document_.network(), *hovered_);
      if (start && end) {
        append_polyline({start->position, end->position}, geometry.dashed_line_positions);
      }
    }
  }

  // The "a waypoint would land here" ghost — the same projection the commit
  // uses, as a Midpoint knob so it reads as "insert" rather than as a node.
  if (!target_.empty() && hovered_.has_value() && !(press_.has_value() && press_->dragging)) {
    if (const std::optional<ActorPose> pose = actor_world_pose(document_.network(), *hovered_)) {
      geometry.add_handle(
          pose->position[0], pose->position[1], pose->position[2], HandleKind::Midpoint);
    }
  }

  return geometry;
}

QString RouteTool::instruction() const {
  if (!hover_hint_.empty()) {
    return QString::fromStdString(hover_hint_);
  }
  if (target_.empty()) {
    return tr("Kliknij uczestnika, aby wyznaczyć jego trasę.");
  }
  if (pedestrian()) {
    return walking_path()
               ? tr("Kliknij kolejny punkt lub przeciągnij węzeł. Zakończ trasę, gdy jest gotowa.")
               : tr("Kliknij punkt, do którego ma dojść %1. Możesz wskazać chodnik lub przejście.")
                     .arg(QString::fromStdString(target_));
  }
  if (start_at_actor_ && current_route() == nullptr) {
    return tr("Kliknij dalej na pasie, aby poprowadzić %1 od miejsca ustawienia.")
        .arg(QString::fromStdString(target_));
  }
  if (pending_.has_value()) {
    return tr("Click the next lane to finish %1's route (Esc discards the first waypoint).")
        .arg(QString::fromStdString(target_));
  }
  if (current_route() == nullptr) {
    return tr("Click a driving lane to lay %1's first waypoint.")
        .arg(QString::fromStdString(target_));
  }
  return tr("Kliknij pas, aby dodać punkt. Przeciągnij węzeł lub wybierz Zakończ trasę.");
}

} // namespace roadmaker::editor
