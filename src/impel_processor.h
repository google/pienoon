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

#include "impel_common.h"
#include "impel_engine.h"

namespace impel {

class ImpellerBase;


// Basic creation and deletion functions, common across processors of any
// dimension.
class ImpelProcessorBase {
 public:
  virtual ~ImpelProcessorBase() {}

  // Advance the simulation by delta_time. Should only be called by ImpelEngine.
  virtual void AdvanceFrame(ImpelTime delta_time) = 0;

  // Creation an impeller and return a unique id representing it.
  // The 'engine' is required if the ImpelProcessor itself creates child
  // Impellers. This function should only be called by Impeller::Initialize().
  virtual ImpelId InitializeImpeller(const ImpelInit& init,
                                     ImpelEngine* engine) = 0;

  // Remove an impeller and return it's unique id to the pile of allocatable
  // ids.
  virtual void RemoveImpeller(ImpelId id) = 0;

  // Return GUID representing the Impeller's type. Must be implemented by
  // derived class.
  virtual ImpellerType Type() const = 0;

  // The number of floats (or doubles) being animated. For example, a position
  // in 3D space would return 3.
  virtual int Dimensions() const = 0;

  // Set simulation values. Some simulation values are independent of the
  // number of dimensions, so we put them in the base class.
  virtual void SetTargetTime(ImpelId /*id*/, float /*target_time*/) {}

  // For aggregate Impellers, get the sub-Impellers. See comments in Impeller
  // for details.
  virtual int ChildImpellerCount(ImpelId /*id*/) const { return 0; }
  virtual const ImpellerBase* ChildImpeller(ImpelId /*id*/, int /*i*/) const {
    return nullptr;
  }
  virtual ImpellerBase* ChildImpeller(ImpelId /*id*/, int /*i*/) {
    return nullptr;
  }
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
  virtual T Value(ImpelId /*id*/) const { return T(0.0f); }
  virtual T Velocity(ImpelId /*id*/) const { return T(0.0f); }
  virtual T TargetValue(ImpelId /*id*/) const { return T(0.0f); }
  virtual void SetValue(ImpelId /*id*/, const T& /*value*/) {}
  virtual void SetVelocity(ImpelId /*id*/, const T& /*velocity*/) {}
  virtual void SetTargetValue(ImpelId /*id*/, const T& /*target_value*/) {}
  virtual T Difference(ImpelId id) const { return TargetValue(id) - Value(id); }
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

// Add this to the public interface of your ImpelProcessors. These are required
// for the ImpelEngine to access impellers of your type.
#define IMPEL_PROCESSOR_REGISTER(Type, InitType) \
    static ImpelProcessorBase* Create() { return new Type(); } \
    static void Destroy(ImpelProcessorBase* p) { return delete p; } \
    static void Register() { \
      const ImpelProcessorFunctions functions(Create, Destroy); \
      ImpelEngine::RegisterProcessorFactory(InitType::kType, functions); \
    }


} // namespace impel

#endif // IMPEL_PROCESSOR_H_
