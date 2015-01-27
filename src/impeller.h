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

#ifndef IMPELLER_H
#define IMPELLER_H

#include "impel_processor.h"

namespace impel {

class ImpelEngine;


// Non-templatized base class of Impeller. Holds all the functions unrelated to
// the value being impelled. See comments above Impeller for details on
// Impellers.
class ImpellerBase {
 public:
  ImpellerBase() : processor_(nullptr), index_(kImpelIndexInvalid) {}
  ImpellerBase(const ImpelInit& init, ImpelEngine* engine) {
    Initialize(init, engine);
  }

  // Allow Impellers to be copied so that they can be put into vectors.
  // Transfer ownership of impeller to new impeller. Old impeller is reset and
  // can no longer be used.
  ImpellerBase(const ImpellerBase& original) {
    if (original.Valid()) {
      original.processor_->TransferImpeller(original.index_, this);
    } else {
      processor_ = nullptr;
      index_ = kImpelIndexInvalid;
    }
  }
  ImpellerBase& operator=(const ImpellerBase& original) {
    Invalidate();
    original.processor_->TransferImpeller(original.index_, this);
    return *this;
  }

  // Remove ourselves from the ImpelProcessor when we're deleted.
  ~ImpellerBase() { Invalidate(); }

  // Initialize this Impeller to the type specified in init.type.
  void Initialize(const ImpelInit& init, ImpelEngine* engine);

  // Detatch this Impeller from its ImpelProcessor. Functions other than
  // Initialize can no longer be called after Invalidate has been called.
  void Invalidate() {
    if (processor_ != nullptr) {
      processor_->RemoveImpeller(index_);
    }
  }

  // Return true if this Impeller is currently being driven by an
  // ImpelProcessor. That is, it has been successfully initialized.
  // Also check for a consistent internal state.
  bool Valid() const {
    return processor_ != nullptr && processor_->ValidImpeller(index_, this);
  }

  // Return the GUID representing the type Impeller we've been initilized to.
  // An Impeller can take on any type, provided the dimensions match.
  ImpellerType Type() const { return processor_->Type(); }

  // The number of floats (or doubles) that this Impeller is driving.
  // For example, if this Impeller is driving a position in 3D space, then
  // we will return 3 here. The dimension of an ImpellerBase is not fixed, but
  // once we apply a templatization in Impeller<T>, then the dimension becomes
  // fixed.
  int Dimensions() const { return processor_->Dimensions(); }

  // Set simulation values. Some simulation values are independent of the
  // number of dimensions, so we put them in the base class.
  void SetTargetTime(float target_time) {
    assert(Valid());
    return processor_->SetTargetTime(index_, target_time);
  }

  // More complicated impellers are aggregates of impellers. For example, a
  // composite impeller might emit a 3D vector by aggregating three 1D
  // impellers. These functions allow you to traverse the tree of impellers.
  int ChildImpellerCount() const {
    assert(Valid());
    return processor_->ChildImpellerCount(index_);
  }
  const ImpellerBase* ChildImpeller(int i) const {
    assert(Valid());
    return processor_->ChildImpeller(index_, i);
  }
  ImpellerBase* ChildImpeller(int i) {
    assert(Valid());
    return processor_->ChildImpeller(index_, i);
  }

 protected:
  // The processor uses the functions below. Does not modify data directly.
  friend ImpelProcessorBase;

  // These should only be called by ImpelProcessorBase!
  void Init(ImpelProcessorBase* processor, ImpelIndex index) {
    processor_ = processor;
    index_ = index;
  }
  void Reset() { Init(nullptr, kImpelIndexInvalid); }
  const ImpelProcessorBase* Processor() const { return processor_; }

  // All calls to an Impeller are proxied to an ImpellerProcessor. Impeller
  // data and processing is centralized to allow for scalable optimizations
  // (e.g. SIMD or parallelization).
  ImpelProcessorBase* processor_;

  // An ImpelProcessor processes one ImpellerType, and hosts every Impeller of
  // that type. The id here uniquely identifies this Impeller to the
  // ImpelProcessor.
  ImpelIndex index_;
};

// Impeller
// ========
// An Impeller drives a value of type Impeller::T. The value is driven towards
// a target that is set with Impeller::SetTargetValue(). The way the value moves
// towards the target is determined by the type of the Impeller, and its
// parameterization, which is set in Impeller::Initialize().
//
// The Impeller's current value can be queried via Impeller::Value().
// Additionally, some impeller types may also return Impeller::Velocity(), the
// derivative of value. Not every impeller type implements all functions,
// however. For example, the impeller type that drives 4x4 matrix
// transformations doesn't return a value for Velocity() because, for 4x4
// matrices the derivitive is not well defined.
//
//
// Converter
// =========
// The processors run on mathfu types, so the Converter is used to return
// values in your native math type.
//
// If you are using mathfu as your native math type, you can use the
// Impeller1f, Impeller2f, types defined below, and don't need to worry about
// Converter.
//
// If you have your own native math type that is byte-wise compatible with
// the mathfu types, you might use a union to convert between the two.
// Something like this:
//   template<class MathFuT, class NativeT>
//   class UnionConverter {
//     union Union {
//       MathFuT mathfu;
//       NativeT native;
//     };
//    public:
//     typedef NativeT ExternalApiType;
//     static NativeT To(const MathFuT& x) {
//       Union u;
//       u.mathfu = x;
//       return u.native;
//     }
//     static MathFuT From(const NativeT& x) {
//       Union u;
//       u.native = x;
//       return u.mathfu;
//     }
//   };
// Note that you should be careful when checking that your types are byte-wise
// compatible with mathfu types. If MATHFU_COMPILE_WITH_PADDING is defined,
// three-dimensional vectors will have a 4th component that must be set
// properly. Unless you're sure, you should use constructors provided by
// mathfu and your native types.
//
template<class Converter>
class Impeller : public ImpellerBase {
 public:
  typedef typename Converter::ExternalType T;

  Impeller() {}

  // Copy functions. Transfers ownership from 'original' to this.
  // Note that even though 'original' is const, the ImpelProcessor has a
  // non-const pointer to it, so it will be invalidated by the end of this call.
  Impeller(const Impeller& original) : ImpellerBase(original) {}
  Impeller& operator=(const Impeller& original) {
    ImpellerBase::operator=(original);
    return *this;
  }

  // Get current impeller values from the processor.
  T Value() const {
    return Converter::To(Processor().Value(index_));
  }
  T Velocity() const {
    return Converter::To(Processor().Velocity(index_));
  }
  T TargetValue() const {
    return Converter::To(Processor().TargetValue(index_));
  }

  // Return the TargetValue() minus the Value(). If we're impelling a modular
  // type (e.g. an angle), this may not be the naive subtraction.
  T Difference() const {
    return Converter::To(Processor().Difference(index_));
  }

  // Set current impeller values in the processor. Processors may choose to
  // ignore whichever values make sense for them to ignore.
  void SetValue(const T& value) {
    return Processor().SetValue(index_, Converter::From(value));
  }
  void SetVelocity(const T& velocity) {
    return Processor().SetVelocity(index_, Converter::From(velocity));
  }
  void SetTargetValue(const T& target_value) {
    return Processor().SetTargetValue(index_, Converter::From(target_value));
  }

 private:
  typedef typename Converter::ExternalType InternalType;
  typedef ImpelProcessor<InternalType> ProcessorType;

  ProcessorType& Processor() {
    assert(Valid());
    return *static_cast<ProcessorType*>(processor_);
  }

  const ProcessorType& Processor() const {
    assert(Valid());
    return *static_cast<const ProcessorType*>(processor_);
  }
};


// The processors run on mathfu types, so these Converters can be used to return
// values in your native math type. This degenerate converter just sticks with
// the original mathfu type. Define your own if you're not using mathfu.
template<class T>
class PassThroughConverter {
 public:
  typedef T ExternalType;
  typedef T InternalType;
  static T To(const T& x) { return x; }
  static T From(const T& x) { return x; }
};

// Define impellers that use mathfu types in the API. If you have your own
// vector library, you can define your own types with a suitable Converter.
// Note that ImpelProcessors use mathfu types, so the Converter is required to
// convert to and from your vector library to mathfu.
typedef Impeller<PassThroughConverter<float>> Impeller1f;
typedef Impeller<PassThroughConverter<mathfu::vec2>> Impeller2f;
typedef Impeller<PassThroughConverter<mathfu::vec3>> Impeller3f;
typedef Impeller<PassThroughConverter<mathfu::vec4>> Impeller4f;
typedef Impeller<PassThroughConverter<mathfu::mat4>> ImpellerMatrix4f;


} // namespace impel

#endif // IMPELLER_H

