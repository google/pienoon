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

#include "impel_processor.h"
#include "impeller.h"


namespace impel {

ImpelProcessor::~ImpelProcessor() {
  // Reset all of the Impellers that we're currently driving.
  // We don't want any of them to reference us after we've been destroyed.
  for (ImpelIndex index = 0; index < index_allocator_.num_indices(); ++index) {
    if (impellers_[index] != nullptr) {
      RemoveImpeller(index);
    }
  }

  // Sanity-check: Ensure that we have no more active Impellers.
  assert(index_allocator_.Empty());
}

void ImpelProcessor::InitializeImpeller(
    const ImpelInit& init, ImpelEngine* engine, Impeller* impeller) {
  // Assign an 'index' to reference the new Impeller. All interactions between
  // the Impeller and ImpelProcessor use this 'index' to identify the data.
  const ImpelIndex index = index_allocator_.Alloc();

  // Keep a pointer to the Impeller around. We may Defragment() the indices and
  // move the data around. We also need remove the Impeller when we're
  // destroyed.
  impellers_[index] = impeller;

  // Initialize the impeller to point at our ImpelProcessor.
  impeller->Init(this, index);

  // Call the ImpelProcessor-specific initialization routine.
  InitializeIndex(init, index, engine);
}

void ImpelProcessor::RemoveImpeller(ImpelIndex index) {
  assert(ValidIndex(index));

  // Call the ImpelProcessor-specific remove routine.
  RemoveIndex(index);

  // Ensure the Impeller no longer references us.
  impellers_[index]->Reset();

  // Ensure we no longer reference the Impeller.
  impellers_[index] = nullptr;

  // Recycle 'index'. It will be used in the next allocation, or back-filled in
  // the next call to Defragment().
  index_allocator_.Free(index);
}

void ImpelProcessor::TransferImpeller(ImpelIndex index,
                                          Impeller* new_impeller) {
  assert(ValidIndex(index));

  // Ensure old Impeller does not reference us anymore. Only one Impeller is
  // allowed to reference 'index'.
  Impeller* old_impeller = impellers_[index];
  old_impeller->Reset();

  // Set up new_impeller to reference 'index'.
  new_impeller->Init(this, index);

  // Update our reference to the unique Impeller that references 'index'.
  impellers_[index] = new_impeller;
}

bool ImpelProcessor::ValidIndex(ImpelIndex index) const {
  return index < index_allocator_.num_indices() &&
         impellers_[index] != nullptr &&
         impellers_[index]->Processor() == this;
}

void ImpelProcessor::SetNumIndicesBase(ImpelIndex num_indices) {
  // When the size decreases, we don't bother reallocating the size of the
  // 'impellers_' vector. We want to avoid reallocating as much as possible,
  // so we let it grow to its high-water mark.
  //
  // TODO: Ideally, we should reserve approximately the right amount of storage
  // for impellers_. That would require adding a user-defined initialization
  // parameter.
  impellers_.resize(num_indices);

  // Call derived class.
  SetNumIndices(num_indices);
}

void ImpelProcessor::MoveIndexBase(ImpelIndex old_index,
                                       ImpelIndex new_index) {
  // Assert we're moving something valid onto something invalid.
  assert(impellers_[new_index] == nullptr && impellers_[old_index] != nullptr);

  // Reinitialize the impeller to point to the new index.
  Impeller* impeller = impellers_[old_index];
  impeller->Init(this, new_index);

  // Swap the pointer values stored at indices.
  impellers_[new_index] = impeller;
  impellers_[old_index] = nullptr;

  // Call derived class so the derived class can perform similar data movement.
  MoveIndex(old_index, new_index);
}


} // namespace impel
