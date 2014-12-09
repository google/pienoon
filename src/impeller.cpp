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

#include "impeller.h"
#include "impel_engine.h"

namespace impel {


void ImpellerBase::Initialize(const ImpelInit& init, ImpelEngine* engine) {
  // Unregister ourselves with our existing ImpelProcessor.
  Invalidate();

  // The ImpelProcessors are held centrally in the ImpelEngine. There is only
  // one processor per type. Get that processor.
  ImpelProcessorBase* processor = engine->Processor(init.type());

  // Register and initialize ourselves with the ImpelProcessor.
  processor->InitializeImpeller(init, engine, this);
}


} // namespace impel
