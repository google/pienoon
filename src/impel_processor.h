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

#include "impel_common.h"
#include "fplutil/index_allocator.h"


namespace impel {

class ImpellerBase;
class ImpelEngine;


// Basic creation and deletion functions, common across processors of any
// dimension.
class ImpelProcessorBase {
 public:
  ImpelProcessorBase() :
      allocator_callbacks_(this),
      index_allocator_(allocator_callbacks_) {
  }
  virtual ~ImpelProcessorBase();

  // Instantiate impeller data inside the ImpelProcessor, and initialize
  // 'impeller' as a reference to that data.
  // The 'engine' is required if the ImpelProcessor itself creates child
  // Impellers. This function should only be called by Impeller::Initialize().
  void InitializeImpeller(const ImpelInit& init, ImpelEngine* engine,
                          ImpellerBase* impeller);

  // Remove an impeller and return its index to the pile of allocatable indices.
  // Should only be called by Impeller::Invalidate().
  void RemoveImpeller(ImpelIndex index);

  // Transfer ownership of the impeller at 'index' to 'new_impeller'.
  // Resets the Impeller that currently owns 'index' and initializes
  // 'new_impeller'.
  // Should only be called by Impeller copy operations.
  void TransferImpeller(ImpelIndex index, ImpellerBase* new_impeller);

  // Returns true if 'index' is currently driving an impeller.
  bool ValidIndex(ImpelIndex index) const;

  // Returns true if 'index' is currently driving 'impeller'.
  bool ValidImpeller(ImpelIndex index, const ImpellerBase* impeller) const {
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

  // Set simulation values. Some simulation values are independent of the
  // number of dimensions, so we put them in the base class.
  virtual void SetTargetTime(ImpelIndex /*index*/, float /*target_time*/) {}

  // For aggregate Impellers, get the sub-Impellers. See comments in Impeller
  // for details.
  virtual int ChildImpellerCount(ImpelIndex /*index*/) const { return 0; }
  virtual const ImpellerBase* ChildImpeller(ImpelIndex /*index*/,
                                            int /*child_index*/) const {
    return nullptr;
  }
  virtual ImpellerBase* ChildImpeller(ImpelIndex /*index*/,
                                      int /*child_index*/) {
    return nullptr;
  }

 protected:
  // Initialize data at 'index'. The meaning of 'index' is determined by the
  // ImpelProcessor implementation (most likely it is the index into one or
  // more data_ arrays though).
  // ImpelProcessorBase tries to keep the 'index' as low as possible, by
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
  // Proxy callbacks from IndexAllocator into ImpelProcessorBase.
  class AllocatorCallbacks :
      public fpl::IndexAllocator<ImpelIndex>::CallbackInterface {
   public:
    AllocatorCallbacks(ImpelProcessorBase* processor) : processor_(processor) {}
    virtual void SetNumIndices(ImpelIndex num_indices) {
      processor_->SetNumIndicesBase(num_indices);
    }
    virtual void MoveIndex(ImpelIndex old_index, ImpelIndex new_index) {
      processor_->MoveIndexBase(old_index, new_index);
    }
   private:
    ImpelProcessorBase* processor_;
  };

  // Back-pointer to the Impellers for each index. The Impellers reference this
  // ImpelProcessor and a specific index into the ImpelProcessor, so when the
  // index is moved, or when the ImpelProcessor itself is destroyed, we need
  // to update the Impeller.
  // Note that we only keep a reference to a single Impeller per index. When
  // a copy of an Impeller is made, the old Impeller is Reset and the reference
  // here is updated.
  std::vector<ImpellerBase*> impellers_;

  // Proxy calbacks into ImpelProcessorBase. The other option is to derive
  // ImpelProcessorBase from IndexAllocator::CallbackInterface, but that would
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
template<class T>
class ImpelProcessor : public ImpelProcessorBase {
 public:
  virtual ~ImpelProcessor() {}
  virtual int Dimensions() const { return ValueDetails<T>::kDimensions; }
  virtual T Value(ImpelIndex /*index*/) const { return T(0.0f); }
  virtual T Velocity(ImpelIndex /*index*/) const { return T(0.0f); }
  virtual T TargetValue(ImpelIndex /*index*/) const { return T(0.0f); }
  virtual T Difference(ImpelIndex index) const {
    return TargetValue(index) - Value(index);
  }
  virtual void SetValue(ImpelIndex /*index*/, const T& /*value*/) {}
  virtual void SetVelocity(ImpelIndex /*index*/, const T& /*velocity*/) {}
  virtual void SetTargetValue(ImpelIndex /*index*/, const T& /*target_value*/){}
};

// ImpelProcessors of various dimensions. All ImpelProcessors operate with
// mathfu types in their API. In the Impeller interface, you can convert from
// mathfu to your own vector types if you wish. See MathfuConverter for a
// example.
typedef ImpelProcessor<float> ImpelProcessor1f;
typedef ImpelProcessor<mathfu::vec2> ImpelProcessor2f;
typedef ImpelProcessor<mathfu::vec3> ImpelProcessor3f;
typedef ImpelProcessor<mathfu::vec4> ImpelProcessor4f;
typedef ImpelProcessor<mathfu::mat4> ImpelProcessorMatrix4f;


// Static functions in ImpelProcessor-derived classes.
typedef ImpelProcessorBase* ImpelProcessorCreateFn();
typedef void ImpelProcessorDestroyFn(ImpelProcessorBase* p);

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
