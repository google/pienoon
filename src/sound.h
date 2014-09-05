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

#ifndef SPLAT_SOUND_H_
#define SPLAT_SOUND_H_

#include <vector>
#include <string>

struct Mix_Chunk;

namespace fpl {

struct SoundDef;

class Sample {
 public:
  Sample() : chunk_(nullptr) {}
  ~Sample();

  // Load the given .wav file.
  bool LoadSample(const char* filename);

  Mix_Chunk* chunk() { return chunk_; }

 private:

  Mix_Chunk* chunk_;
};

// Sounds represent an abstract sound (like a 'whoosh'). The Sound object
// contains a number of samples with weighted probabilities to choose between
// randomly when played.
class Sound {
 public:
  // Load the given flatbuffer binary file containing a SoundDef.
  bool LoadSoundFromFile(const char* filename);

  // Load the given flatbuffer data representing a SoundDef.
  bool LoadSound(const std::string& sound_def_source);

  // Unload the data associated with this Sound.
  void Unload();

  // Return the SoundDef.
  const SoundDef* GetSoundDef() const;

  // Return a random Mix_Chunk from the set of samples for this sound.
  Mix_Chunk* SelectChunk();

 private:
  std::string source_;
  std::vector<Sample> samples_;
  float sum_of_probabilities_;
};

}  // namespace fpl

#endif  // SPLAT_SOUND_H_
