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

#ifndef IMPEL_FLATBUFFERS_H_
#define IMPEL_FLATBUFFERS_H_

namespace impel {

class OvershootImpelInit;
struct OvershootParameters;
class SmoothImpelInit;
struct SmoothParameters;
struct Settled1f;
struct Settled1fParameters;

void OvershootInitFromFlatBuffers(const OvershootParameters& params,
                                  OvershootImpelInit* init);

void SmoothInitFromFlatBuffers(const SmoothParameters& params,
                               SmoothImpelInit* init);

void Settled1fFromFlatBuffers(const Settled1fParameters& params,
                              Settled1f* settled);

}  // namespace impel

#endif  // IMPEL_FLATBUFFERS_H_
