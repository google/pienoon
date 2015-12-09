// Copyright 2015 Google Inc. All rights reserved.
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

#ifndef COMPONENTS_DRIPANDVANISH_H_
#define COMPONENTS_DRIPANDVANISH_H_

#include "common.h"
#include "components_generated.h"
#include "corgi/component.h"
#include "mathfu/constants.h"
#include "scene_description.h"

namespace fpl {
namespace pie_noon {

// Data for accessory components.
struct DripAndVanishData {
  float lifetime_remaining;
  float slide_time;
  float drip_distance;
  mathfu::vec3_packed start_position;
  mathfu::vec3_packed start_scale;
};

// Basic behavior for pie splatters:  They stay there for a while,
// and then they slowly drip down and vanish.
class DripAndVanishComponent : public corgi::Component<DripAndVanishData> {
 public:
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual void UpdateAllEntities(corgi::WorldTime /*delta_time*/);
  virtual void InitEntity(corgi::EntityRef& entity);
  void SetStartingValues(corgi::EntityRef& entity);
};

}  // pie_noon
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::pie_noon::DripAndVanishComponent,
                         fpl::pie_noon::DripAndVanishData)

#endif  // COMPONENTS_DRIPANDVANISH_H_
