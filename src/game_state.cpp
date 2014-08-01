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
#include "character_state_machine_def_generated.h"
#include "timeline_generated.h"
#include "controller.h"

namespace fpl {
namespace splat {

template<class T>
static inline int GetFirstIndexAfterTime(
    const T& arr, const int start_index, const WorldTime t) {

  // Assume that T is a flatbuffer::Vector type.
  for (int i = start_index; i < static_cast<int>(arr->Length()); ++i) {
    if (arr->Get(i)->time() >= t)
      return i;
  }
  return arr->Length();
}

WorldTime GameState::GetAnimationTime(const Character& c) const {
  return time_ - c.state_machine()->current_state_start_time();
}

void GameState::ProcessEvents(Character& c) {
  // Process events in timeline.
  const Timeline* const timeline =
      GetTimeline(c.state_machine()->current_state());
  const WorldTime animTime = GetAnimationTime(c);
  const auto events = timeline->events();
  const int start_index = GetFirstIndexAfterTime(events, 0, animTime);
  const int end_index = GetFirstIndexAfterTime(events, start_index,
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

void GameState::AdvanceFrame() {
  // Update controller to gather state machine inputs.
  for (auto it = characters_.begin(); it != characters_.end(); ++it) {
    it->controller()->AdvanceFrame();
    it->controller()->SetLogicalInputs(LogicalInputs_JustHit, false);
    it->controller()->SetLogicalInputs(LogicalInputs_NoHealth,
                                       it->health() <= 0);
  }

  // Update pies. Modify state machine input when character hit by pie.
  for (auto it = pies_.begin(); it != pies_.end(); ++it) {
    if (it->ContactTime() < time_)
      continue;

    Character& c = characters_[it->target()];
    c.controller()->SetLogicalInputs(LogicalInputs_JustHit, true);
    it = pies_.erase(it);
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

}  // splat
}  // fpl

