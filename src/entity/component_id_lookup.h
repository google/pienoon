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

#ifndef FPL_COMPONENT_ID_LOOKUP_H_
#define FPL_COMPONENT_ID_LOOKUP_H_

#include "entity_common.h"

namespace fpl {
namespace entity {

// A templated struct for holding type-dependent data, and a macro
// for declaring it.
template <typename T> struct ComponentIdLookup {};

// Each component needs to use this macro somewhere in its header, in order
// to declare the necessary constants for lookups.  Note that since the macro
// includes its own namespace definitions, this should ideally be called outside
// of any namespaces.
// Arguments:
// ComponentType is the name of the component class.
// DataType is the name of the struct that holds the component data.  (i. e. the
// type that the component was specialized for.)
// Id is the unique identifier that is associated with the component.
#define FPL_ENTITY_REGISTER_COMPONENT(ComponentType, DataType, Id) \
  FPL_ENTITY_REGISTER_COMPONENT_ID(ComponentType, Id) \
  FPL_ENTITY_REGISTER_COMPONENT_ID(DataType, Id)

// This macro handles the lower level job of generating code to associate data
// with a type.  It is usually invoked by FPL_REGISTER_COMPONENT, rather than
// invoking it directly.  (Since registration of a component requires multiple
// datatype/ID registrations.)
#define FPL_ENTITY_REGISTER_COMPONENT_ID(DataType, Id) \
  namespace fpl { \
  namespace entity { \
  template<> struct ComponentIdLookup<DataType> { \
    static const int kComponentId = Id; \
  }; \
  } \
  }

}  // entity
}  // fpl
#endif  // FPL_COMPONENT_ID_LOOKUP_H_
