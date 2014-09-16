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

#ifndef IMPEL_COMMON_H_
#define IMPEL_COMMON_H_

#include <cstdint>

#include "mathfu/glsl_mappings.h"


namespace impel {


class ImpelProcessorBase;

// Impeller type is used for run-time type information. It's implemented as a
// pointer to a string, in each derivation of ImpelInit.
typedef const char** ImpellerType;
static const ImpellerType kImpelTypeInvalid = nullptr;

// The ImpelId identifies an Impeller inside an ImpelProcessor. The
// ImpelProcessor holds all Impellers of its type. Calls to Impellers are
// proxied to the ImpelProcessor.
typedef int16_t ImpelId;
static const ImpelId kImpelIdInvalid = -1;

// Time units are defined by the user. We use integer instead of floating
// point to avoid a loss of precision as time accumulates.
typedef int ImpelTime;

// Base class for Impeller parameterization. Every impeller type has a different
// set of parameters that define its movement. Every impeller type derives its
// own Init class from ImpelInit, to define those parameters.
struct ImpelInit {
  ImpellerType type;

  // The derived class's constructor should set 'type'.
  explicit ImpelInit(ImpellerType type) : type(type) {}
};

// Add this to the public interface of your derivation of ImpelInit. It defines
// a unique identifier for this type as kType. Your derivation's constructor
// should construct base class with ImpelInit(kType).
#define IMPEL_INIT_REGISTER() \
    static const char* kName; \
    static const ImpellerType kType

// Add this to your derivation's source file. It instantiates the static
// variables declared in IMPEL_INIT_REGISTER. Example usage,
//    IMPEL_INIT_INSTANTIATE(AwesomeImpelInit, "Awesome");
#define IMPEL_INIT_INSTANTIATE(Type) \
    const char* Type::kName = #Type; \
    const ImpellerType Type::kType = &Type::kName


template<class C> struct ValueDetails {};
template<> struct ValueDetails<float> {
  static const int kDimensions = 1;
};
template<> struct ValueDetails<mathfu::vec2> {
  static const int kDimensions = 2;
};
template<> struct ValueDetails<mathfu::vec3> {
  static const int kDimensions = 3;
};
template<> struct ValueDetails<mathfu::vec4> {
  static const int kDimensions = 4;
};
template<> struct ValueDetails<mathfu::mat4> {
  static const int kDimensions = 16;
};


} // namespace impel

#endif // IMPEL_COMMON_H_
