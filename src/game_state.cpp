// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "precompiled.h"
#include "game_state.h"
#include "character_state_machine.h"
#include "timeline_generated.h"
#include "character_state_machine_def_generated.h"
#include "controller.h"
#include "render_scene.h"

namespace fpl {
namespace splat {

// Time between pie being launched and hitting target.
// TODO: This should be in a configuration file.
static const WorldTime kPieFlightTime = 30;

WorldTime GameState::GetAnimationTime(const Character& c) const {
  return time_ - c.state_machine()->current_state_start_time();
}

void GameState::ProcessEvents(Character& c) {
  // Process events in timeline.
  const Timeline* const timeline =
      c.state_machine()->current_state()->timeline();
  if (!timeline)
    return;

  const WorldTime animTime = GetAnimationTime(c);
  const auto events = timeline->events();
  const int start_index = TimelineIndexAfterTime(events, 0, animTime);
  const int end_index = TimelineIndexAfterTime(events, start_index,
                                               animTime + kDeltaTime);

  for (int i = start_index; i < end_index; ++i) {
    const TimelineEvent& timeline_event = *events->Get(i);
    switch (timeline_event.event())
    {
      case EventId_TakeDamage:
        c.set_health(std::max(c.health() + timeline_event.modifier(), 0));
        break;

      case EventId_ReleasePie:
        pies_.push_back(AirbornePie(c.id(), c.target(), time_,
                                    c.pie_damage()));
        break;

      case EventId_LoadPie:
        c.set_pie_damage(std::max(c.pie_damage() + timeline_event.modifier(),
                                  0));
        break;

      default:
        assert(0);
    }
  }
}

static Quat CalculatePieOrientation(const Character& source,
    const Character& target, WorldTime time_since_launch) {
  (void)source; (void)target; (void)time_since_launch;

  // TODO: Make pie spin about axis of rotation.
  return Quat(0.0f, mathfu::vec3(0.0f, 1.0f, 0.0f));
}

static mathfu::vec3 CalculatePiePosition(const Character& source,
    const Character& target, WorldTime time_since_launch) {
  // TODO: Make the pie follow a trajectory.
  const float percent = static_cast<float>(time_since_launch) / kPieFlightTime;
  return mathfu::vec3::Lerp(source.position(), target.position(), percent);
}

void GameState::UpdatePiePosition(AirbornePie& pie) const {
  const WorldTime time_since_launch = time_ - pie.start_time();
  const Character& source = characters_[pie.source()];
  const Character& target = characters_[pie.target()];

  const Quat pie_orientation = CalculatePieOrientation(source, target,
                                                       time_since_launch);
  const mathfu::vec3 pie_position = CalculatePiePosition(source, target,
                                                         time_since_launch);

  pie.set_orientation(pie_orientation);
  pie.set_position(pie_position);
}

void GameState::AdvanceFrame() {
  // Update controller to gather state machine inputs.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    Controller* controller = it->controller();
    controller->AdvanceFrame();
    controller->SetLogicalInputs(LogicalInputs_JustHit, false);
    controller->SetLogicalInputs(LogicalInputs_NoHealth, it->health() <= 0);
    const Timeline* timeline = it->state_machine()->current_state()->timeline();
    controller->SetLogicalInputs(LogicalInputs_AnimationEnd,
        timeline && (GetAnimationTime(*it) >= timeline->end_time()));
  }

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end(); ++it) {
    UpdatePiePosition(*it);

    // Remove pies that have made contact.
    const WorldTime time_since_launch = time_ - it->start_time();
    if (time_since_launch >= kPieFlightTime) {
      Character& c = characters_[it->target()];
      c.controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
      it = pies_.erase(it);
    }
  }

  // Update the state machines.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    TransitionInputs transition_inputs;
    transition_inputs.logical_inputs = it->controller()->logical_inputs();
    transition_inputs.animation_time = GetAnimationTime(*it);
    it->state_machine()->Update(transition_inputs);
  }

  // Look to timeline to see what's happening. Make it happen.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    ProcessEvents(*it);
  }
}

static uint16_t RenderableIdForPieDamage(CharacterHealth damage) {
  const int kMaxDamageForRenderable =
    RenderableId_PieLarge - RenderableId_PieEmpty;
  const int clamped_damage = std::min(damage, kMaxDamageForRenderable);
  return static_cast<uint16_t>(RenderableId_PieEmpty + clamped_damage);
}

// TODO: Make this function a member of GameState, once that class has been
// submitted to git. Then populate from the values in GameState.
void GameState::PopulateScene(RenderScene* scene) const {
  // Camera.
  scene->set_camera(mathfu::mat4::FromTranslationVector(
                        mathfu::vec3(0.0f, 5.0f, -10.0f)));

  // Characters.
  for (auto c = characters_.begin(); c != characters_.end(); ++c) {
    const WorldTime anim_time = GetAnimationTime(*c);
    scene->renderables().push_back(
        Renderable(c->RenderableId(anim_time), c->CalculateMatrix()));
  }

  // Pies.
  for (auto it = pies_.begin(); it != pies_.end(); ++it) {
    const AirbornePie& pie = *it;
    scene->renderables().push_back(
        Renderable(RenderableIdForPieDamage(pie.damage()),
                                            pie.CalculateMatrix()));
  }

  // Lights.
  scene->lights().push_back(mathfu::vec3(-20.0f, 20.0f, -20.0f));
}



}  // splat
}  // fpl

