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

#ifndef IMPEL_PROCESSOR_H_
#define IMPEL_PROCESSOR_H_

#include <vector>

#include "fplutil/index_allocator.h"
#include "impel_common.h"

namespace fpl { class CompactSpline; }

namespace impel {

class Impeller;
class ImpelEngine;
class ImpelTarget1f;


// ImpelProcessor
// ==============
// An ImpelProcessor processes *all* instances of one type of Impeller.
// Or, at least, all instances within a given ImpelEngine.
//
// We pool the processing for potential optimization opportunities. We may have
// hundreds of smoothly-interpolating one-dimensional Impellers, for example.
// It's nice to be able to update those 4 or 8 or 16 at a time using SIMD.
// And it's nice to have the data gathered in one spot if we want to use
// multiple threads.
//
// ImpelProcessors exists in the internal API. For the external API, please see
// Impeller.
//
// Users can create their own Impeller algorithms by deriving from
// ImpelProcessor. ImpelProcessors must have a factory that's registered with
// the ImpelEngine (please see ImpelEngine for details). Once registered,
// you can use your new Impeller algorithm by calling Impeller::Initialize()
// with ImpelInit::type set to your ImpelProcessor's ImpellerType.
//
// ImpelProcessors run on mathfu types. Please see the specializations below
// for ImpelProcessors of various dimensions. Note that Impeller expects the
// ImpelProcessor to return a mathfu type, so you shouldn't try to specialize
// ImpelProcessor<> with any of your native types.
//
class ImpelProcessor {
 public:
  ImpelProcessor() :
      allocator_callbacks_(this),
      index_allocator_(allocator_callbacks_) {
  }
  virtual ~ImpelProcessor();

  // Instantiate impeller data inside the ImpelProcessor, and initialize
  // 'impeller' as a reference to that data.
  // The 'engine' is required if the ImpelProcessor itself creates child
  // Impellers. This function should only be called by Impeller::Initialize().
  void InitializeImpeller(const ImpelInit& init, ImpelEngine* engine,
                          Impeller* impeller);

  // Remove an impeller and return its index to the pile of allocatable indices.
  // Should only be called by Impeller::Invalidate().
  void RemoveImpeller(ImpelIndex index);

  // Transfer ownership of the impeller at 'index' to 'new_impeller'.
  // Resets the Impeller that currently owns 'index' and initializes
  // 'new_impeller'.
  // Should only be called by Impeller copy operations.
  void TransferImpeller(ImpelIndex index, Impeller* new_impeller);

  // Returns true if 'index' is currently driving an impeller.
  bool ValidIndex(ImpelIndex index) const;

  // Returns true if 'index' is currently driving 'impeller'.
  bool ValidImpeller(ImpelIndex index, const Impeller* impeller) const {
    return ValidIndex(index) && impellers_[index] == impeller;
  }

  // Advance the simulation by delta_time.
  // Should only be called by ImpelEngine::AdvanceFrame.
  virtual void AdvanceFrame(ImpelTime delta_time) = 0;

  // Return GUID representing the Impeller's type. Must be implemented by
  // derived class.
  virtual ImpellerType Type() const = 0;

  // The number of floats (or doubles) being animated. For example, a position
  // in 3D space would return 3.
  virtual int Dimensions() const = 0;

  // The lower the number, the sooner the ImpelProcessor gets updated.
  // Should never change. We want a static ordering of processors.
  // Some ImpelProcessors use the output of other ImpelProcessors, so
  // we impose a strict ordering here.
  virtual int Priority() const = 0;

 protected:
  // Initialize data at 'index'. The meaning of 'index' is determined by the
  // ImpelProcessor implementation (most likely it is the index into one or
  // more data_ arrays though).
  // ImpelProcessor tries to keep the 'index' as low as possible, by
  // recycling ones that have been freed, and by providing a Defragment()
  // function to move later indices to indices that have been freed.
  virtual void InitializeIndex(const ImpelInit& init, ImpelIndex index,
                               ImpelEngine* engine) = 0;

  // Reset data at 'index'. See comment above InitializeIndex for meaning of
  // 'index'. If your ImpelProcessor stores data in a plain array, you
  // probably have nothing to do. But if you use dynamic memory per index,
  // (which you really shouldn't - too slow!), you should deallocate it here.
  // For debugging, it might be nice to invalidate the data.
  virtual void RemoveIndex(ImpelIndex index) = 0;

  // Move the data at 'old_index' into 'new_index'. Used by Defragment().
  // Note that 'new_index' is guaranteed to be inactive.
  virtual void MoveIndex(ImpelIndex old_index, ImpelIndex new_index) = 0;

  virtual void SetNumIndices(ImpelIndex num_indices) = 0;

  void MoveIndexBase(ImpelIndex old_index, ImpelIndex new_index);
  void SetNumIndicesBase(ImpelIndex num_indices);

  // When an index is moved, the Impeller that references that index is updated.
  // Can be called at the discretion of your ImpelProcessor, but normally called
  // at the beginning of your ImpelProcessor::AdvanceFrame.
  void Defragment() {
    index_allocator_.Defragment();
  }

 private:
  // Proxy callbacks from IndexAllocator into ImpelProcessor.
  class AllocatorCallbacks :
      public fpl::IndexAllocator<ImpelIndex>::CallbackInterface {
   public:
    AllocatorCallbacks(ImpelProcessor* processor) : processor_(processor) {}
    virtual void SetNumIndices(ImpelIndex num_indices) {
      processor_->SetNumIndicesBase(num_indices);
    }
    virtual void MoveIndex(ImpelIndex old_index, ImpelIndex new_index) {
      processor_->MoveIndexBase(old_index, new_index);
    }
   private:
    ImpelProcessor* processor_;
  };

  // Back-pointer to the Impellers for each index. The Impellers reference this
  // ImpelProcessor and a specific index into the ImpelProcessor, so when the
  // index is moved, or when the ImpelProcessor itself is destroyed, we need
  // to update the Impeller.
  // Note that we only keep a reference to a single Impeller per index. When
  // a copy of an Impeller is made, the old Impeller is Reset and the reference
  // here is updated.
  std::vector<Impeller*> impellers_;

  // Proxy calbacks into ImpelProcessor. The other option is to derive
  // ImpelProcessor from IndexAllocator::CallbackInterface, but that would
  // create a messier API, and not be great OOP.
  // This member should be initialized before index_allocator_ is initialized.
  AllocatorCallbacks allocator_callbacks_;

  // When an index is freed, we keep track of it here. When an index is
  // allocated, we use one off this array, if one exists.
  // When Defragment() is called, we empty this array by filling all the
  // unused indices with the highest allocated indices. This reduces the total
  // size of the data arrays.
  fpl::IndexAllocator<ImpelIndex> index_allocator_;
};


// Interface for impeller types that drive a single float value.
// That is, for ImpelProcessors that interface with Impeller1f's.
class ImpelProcessor1f : public ImpelProcessor {
 public:
  virtual int Dimensions() const { return 1; }

  // Get current impeller values from the processor.
  virtual float Value(ImpelIndex index) const = 0;
  virtual float Velocity(ImpelIndex index) const = 0;
  virtual float TargetValue(ImpelIndex index) const = 0;
  virtual float TargetVelocity(ImpelIndex index) const = 0;
  virtual float Difference(ImpelIndex index) const = 0;
  virtual float TargetTime(ImpelIndex index) const = 0;

  // At least one of these should be implemented. Otherwise, there will be
  // no way to drive the Impeller towards a target.
  virtual void SetTarget(ImpelIndex /*index*/, const ImpelTarget1f& /*t*/) {}
  virtual void SetWaypoints(ImpelIndex /*index*/,
                            const fpl::CompactSpline& /*waypoints*/,
                            float /*start_time*/) {}
};


// Interface for impeller types that drive a 4x4 float matrix.
// That is, for ImpelProcessors that interface with ImpellerMatrix4f's.
class ImpelProcessorMatrix4f : public ImpelProcessor {
 public:
  virtual int Dimensions() const { return 16; }

  // Get the current matrix value from the processor.
  virtual const mathfu::mat4& Value(ImpelIndex index) const = 0;

  // Set child values. Matrices are composed from child components.
  virtual void SetChildTarget1f(ImpelIndex /*index*/,
                                ImpelChildIndex /*child_index*/,
                                ImpelTarget1f& /*t*/) {}
  virtual void SetChildValue1f(ImpelIndex /*index*/,
                               ImpelChildIndex /*child_index*/,
                               float /*value*/) {}
};


// Static functions in ImpelProcessor-derived classes.
typedef ImpelProcessor* ImpelProcessorCreateFn();
typedef void ImpelProcessorDestroyFn(ImpelProcessor* p);

struct ImpelProcessorFunctions {
  ImpelProcessorCreateFn* create;
  ImpelProcessorDestroyFn* destroy;

  ImpelProcessorFunctions(ImpelProcessorCreateFn* create,
                          ImpelProcessorDestroyFn* destroy)
      : create(create), destroy(destroy) {
  }
};


} // namespace impel

#endif // IMPEL_PROCESSOR_H_
