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

#ifndef COMPONENTS_SHAKEABLE_PROP_H_
#define COMPONENTS_SHAKEABLE_PROP_H_

#include <memory>
#include <vector>
#include "common.h"
#include "components_generated.h"
#include "config_generated.h"
#include "corgi/component.h"
#include "motive/init.h"
#include "motive/io/flatbuffers.h"
#include "motive/util.h"

namespace fpl {
namespace pie_noon {

struct ShakeablePropData {
  ShakeablePropData() {}
  float shake_scale;
  fplbase::Axis axis;
  motive::Motivator1f motivator;
};

class ShakeablePropComponent : public corgi::Component<ShakeablePropData> {
 public:
  virtual void UpdateAllEntities(corgi::WorldTime delta_time);
  virtual void AddFromRawData(corgi::EntityRef& entity, const void* data);
  virtual void InitEntity(corgi::EntityRef& entity);
  virtual void CleanupEntity(corgi::EntityRef& entity);

  void set_config(const Config* config) { config_ = config; }
  void set_engine(motive::MotiveEngine* engine) { engine_ = engine; }
  void LoadMotivatorSpecs();
  void ShakeProps(float damage_percent, const mathfu::vec3& damage_position);

 private:
  const Config* config_;
  motive::MotiveEngine* engine_;
  motive::OvershootInit motivator_inits[MotivatorSpecification_Count];
};

}  // pie_noon
}  // fpl

CORGI_REGISTER_COMPONENT(fpl::pie_noon::ShakeablePropComponent,
                         fpl::pie_noon::ShakeablePropData)

#endif  // COMPONENTS_SHAKEABLE_PROP_H_
