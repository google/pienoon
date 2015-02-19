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

// Impeller
// ========
// An Impeller drives a value towards a target.
//
// The value can be one-dimensional (e.g. a float), or multi-dimensional
// (e.g. a matrix). The dimension is determined by the sub-class:
// Impeller1f drives a float, ImpellerMatrix4f drives a 4x4 float matrix.
// The Impeller's current value can be queried with Value().
//
// The way an Impeller's value moves towards its target is determined by the
// **type** of an impeller. The type is specified in Impeller::Initialize().
//
// Note that an Impeller does not store any data itself. It is a handle into
// an ImpelProcessor. Each ImpelProcessor holds all data for impellers
// of a given **type**. Only one Impeller can hold a handle to specific data.
// Therefore, you can copy an Impeller, but the original impeller will become
// invalid.
//
class Impeller {
 public:
  Impeller() : processor_(nullptr), index_(kImpelIndexInvalid) {}
  Impeller(const ImpelInit& init, ImpelEngine* engine)
      : processor_(nullptr), index_(kImpelIndexInvalid) {
    Initialize(init, engine);
  }

  // Allow Impellers to be copied so that they can be put into vectors.
  // Transfer ownership of impeller to new impeller. Old impeller is reset and
  // can no longer be used.
  Impeller(const Impeller& original) {
    if (original.Valid()) {
      original.processor_->TransferImpeller(original.index_, this);
    } else {
      processor_ = nullptr;
      index_ = kImpelIndexInvalid;
    }
  }
  Impeller& operator=(const Impeller& original) {
    Invalidate();
    original.processor_->TransferImpeller(original.index_, this);
    return *this;
  }

  // Remove ourselves from the ImpelProcessor when we're deleted.
  ~Impeller() { Invalidate(); }

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

  // Return the type of Impeller we've been initilized to.
  // An Impeller can take on any type.
  ImpellerType Type() const { return processor_->Type(); }

  // The number of floats (or doubles) that this Impeller is driving.
  // For example, if this Impeller is driving a position in 3D space, then
  // we will return 3 here.
  int Dimensions() const { return processor_->Dimensions(); }

 protected:
  // The ImpelProcessor uses the functions below. It does not modify data
  // directly.
  friend ImpelProcessor;

  // These should only be called by ImpelProcessor!
  void Init(ImpelProcessor* processor, ImpelIndex index) {
    processor_ = processor;
    index_ = index;
  }
  void Reset() { Init(nullptr, kImpelIndexInvalid); }
  const ImpelProcessor* Processor() const { return processor_; }

  // All calls to an Impeller are proxied to an ImpellerProcessor. Impeller
  // data and processing is centralized to allow for scalable optimizations
  // (e.g. SIMD or parallelization).
  ImpelProcessor* processor_;

  // An ImpelProcessor processes one ImpellerType, and hosts every Impeller of
  // that type. The id here uniquely identifies this Impeller to the
  // ImpelProcessor.
  ImpelIndex index_;
};

// Drive a float value towards a target.
//
// The current and target values and velocities can be specified by SetTarget()
// or SetWaypoints().
class Impeller1f : public Impeller {
 public:
  Impeller1f() {}
  Impeller1f(const ImpelInit& init, ImpelEngine* engine)
      : Impeller(init, engine) {}
  Impeller1f(const ImpelInit& init, ImpelEngine* engine, const ImpelTarget1f& t)
      : Impeller(init, engine) {
    SetTarget(t);
  }
  void InitializeWithTarget(const ImpelInit& init, ImpelEngine* engine,
                            const ImpelTarget1f& t) {
    Initialize(init, engine);
    SetTarget(t);
  }

  // Return current impeller values.
  float Value() const { return Processor().Value(index_); }
  float Velocity() const { return Processor().Velocity(index_); }
  float TargetValue() const { return Processor().TargetValue(index_); }
  float TargetVelocity() const { return Processor().TargetVelocity(index_); }

  // Returns TargetValue() minus Value(). If we're impelling a
  // modular type (e.g. an angle), this may not be the naive subtraction.
  float Difference() const { return Processor().Difference(index_); }
  float TargetTime() const { return Processor().TargetTime(index_); }

  // Set current impeller values in the processor. Processors may choose to
  // ignore whichever values make sense for them to ignore.
  void SetTarget(const ImpelTarget1f& t) { Processor().SetTarget(index_, t); }
  void SetWaypoints(const fpl::CompactSpline& waypoints, float start_time) {
    Processor().SetWaypoints(index_, waypoints, start_time);
  }

 private:
  ImpelProcessor1f& Processor() {
    return *static_cast<ImpelProcessor1f*>(processor_);
  }
  const ImpelProcessor1f& Processor() const {
    return *static_cast<const ImpelProcessor1f*>(processor_);
  }
};

// Drive a 4x4 float matrix from a series of basic transformations.
//
// The underlying basic transformations can be altered with SetChildTarget1f()
// and SetChildValue1f().
//
// Internally, we use mathfu::mat4 as our matrix type, but external we allow
// any matrix type to be specified via the Matrix4f template parameter.
//
template <class VectorConverter>
class ImpellerMatrix4fTemplate : public Impeller {
  typedef VectorConverter C;
  typedef typename VectorConverter::ExternalMatrix4 Mat4;
  typedef typename VectorConverter::ExternalVector3 Vec3;

 public:
  ImpellerMatrix4fTemplate() {}
  ImpellerMatrix4fTemplate(const ImpelInit& init, ImpelEngine* engine)
      : Impeller(init, engine) {}

  // Return the current value of the Impeller. The processor returns a
  // vector-aligned matrix, so the cast should be valid for any user-defined
  // matrix type.
  const Mat4& Value() const { return C::To(Processor().Value(index_)); }

  float ChildValue1f(ImpelChildIndex child_index) const {
    return Processor().ChildValue1f(index_, child_index);
  }
  Vec3 ChildValue3f(ImpelChildIndex child_index) const {
    return C::To(Processor().ChildValue3f(index_, child_index));
  }

  // Set the target for a child impeller. Each basic matrix transformations
  // can be driven by a child impeller. This call lets us control each
  // transformation.
  void SetChildTarget1f(ImpelChildIndex child_index, const ImpelTarget1f& t) {
    Processor().SetChildTarget1f(index_, child_index, t);
  }

  // Set the constant value of a child. Each basic matrix transformation
  // can be driven by a constant value. This call lets us set those constant
  // values.
  void SetChildValue1f(ImpelChildIndex child_index, float value) {
    Processor().SetChildValue1f(index_, child_index, value);
  }
  void SetChildValue3f(ImpelChildIndex child_index, const Vec3& value) {
    Processor().SetChildValue3f(index_, child_index, C::From(value));
  }

 private:
  ImpelProcessorMatrix4f& Processor() {
    return *static_cast<ImpelProcessorMatrix4f*>(processor_);
  }
  const ImpelProcessorMatrix4f& Processor() const {
    return *static_cast<const ImpelProcessorMatrix4f*>(processor_);
  }
};

// External types are also mathfu in this converter. Create your own converter
// if you'd like to use your own vector types in ImpelMatrix's external API.
class PassThroughVectorConverter {
 public:
  typedef mathfu::mat4 ExternalMatrix4;
  typedef mathfu::vec3 ExternalVector3;
  static const ExternalMatrix4& To(const mathfu::mat4& m) { return m; }
  static ExternalVector3 To(const mathfu::vec3& v) { return v; }
  static const mathfu::vec3& From(const ExternalVector3& v) { return v; }
};

typedef ImpellerMatrix4fTemplate<PassThroughVectorConverter> ImpellerMatrix4f;

}  // namespace impel

#endif  // IMPELLER_H
