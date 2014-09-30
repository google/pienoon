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

#ifndef FPL_AUDIO_COLLECTION
#define FPL_AUDIO_COLLECTION

#include <vector>
#include <string>
#include <memory>

namespace fpl {

struct SoundDef;
class AudioSource;

// AudioCollection represent an abstract sound (like a 'whoosh'), which contains
// a number of pieces of audio with weighted probabilities to choose between
// randomly when played. It holds objects of type `Audio`, which can be either
// Sounds or Music
class AudioCollection {
 public:
  // Load the given flatbuffer data representing a SoundDef.
  bool LoadAudioCollectionDef(const std::string& source);

  // Load the given flatbuffer binary file containing a SoundDef.
  bool LoadAudioCollectionDefFromFile(const char* filename);

  // Unload the data associated with this Sound.
  void Unload();

  // Return the SoundDef.
  const SoundDef* GetSoundDef() const;

  // Return a random piece of audio from the set of audio for this sound.
  AudioSource* Select() const;

 private:
  std::string source_;
  std::vector<std::unique_ptr<AudioSource>> audio_sources_;
  float sum_of_probabilities_;
};

}  // namespace fpl

#endif  // FPL_AUDIO_COLLECTION
