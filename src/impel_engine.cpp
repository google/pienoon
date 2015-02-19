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

#include "impel_engine.h"
#include "impel_processor.h"

namespace impel {

// static
ImpelEngine::FunctionMap ImpelEngine::function_map_;

// static
void ImpelEngine::RegisterProcessorFactory(ImpellerType type,
                                           const ImpelProcessorFunctions& fns) {
  function_map_.insert(FunctionPair(type, fns));
}

void ImpelEngine::Reset() {
  for (ProcessorMap::iterator it = mapped_processors_.begin();
       it != mapped_processors_.end(); ++it) {
    // Get the factory for each processor. Factory must exist since it is what
    // created the processor in the first place.
    const ImpelProcessorFunctions& fns = function_map_.find(it->first)->second;

    // Destroy each processor in turn.
    fns.destroy(it->second);
    it->second = nullptr;
  }

  // Remove all elements from the map. Their processors have all been destroyed.
  mapped_processors_.clear();
}

ImpelProcessor* ImpelEngine::Processor(ImpellerType type) {
  // If processor already exists, return it.
  ProcessorMap::iterator it = mapped_processors_.find(type);
  if (it != mapped_processors_.end()) return it->second;

  // Look up the processor-creation-function in the registry.
  const auto function_pair = function_map_.find(type);
  if (function_pair == function_map_.end()) return nullptr;
  const ImpelProcessorFunctions& fns = function_pair->second;

  // Remember processor for next time. We only want at most one processor per
  // type in an engine.
  ImpelProcessor* processor = fns.create();
  mapped_processors_.insert(ProcessorPair(type, processor));

  sorted_processors_.insert(processor);
  return processor;
}

void ImpelEngine::AdvanceFrame(ImpelTime delta_time) {
  // Advance the simulation in each processor.
  // TODO: At some point, we'll want to do several passes. An item in
  // processor A might depend on the output of an item in processor B,
  // which might in turn depend on the output of a *different* item in
  // processor A. In this case, we have to do two passes. For now, just
  // assume that one pass is sufficient.
  for (ProcessorSet::iterator it = sorted_processors_.begin();
       it != sorted_processors_.end(); ++it) {
    ImpelProcessor* processor = *it;
    processor->AdvanceFrame(delta_time);
  }
}

}  // namespace impel
