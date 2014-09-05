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

#ifndef SPLAT_AUDIO_ENGINE_H_
#define SPLAT_AUDIO_ENGINE_H_

#include "sound.h"
#include "splat_common_generated.h"
#include "common.h"

#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
#include "gtest/gtest.h"
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

namespace fpl {

struct AudioConfig;

typedef unsigned int SoundId;
typedef int ChannelId;

class AudioEngine {
 public:
  ~AudioEngine();

  bool Initialize(const AudioConfig* config);

  // Play a sound associated with the given sound_id.
  void PlaySoundId(SoundId sound_id);

  // Returns the sound associated with the given sound_id.
  Sound* GetSound(SoundId sound_id);

  // TODO: Update audio volume per channel each frame. b/17316699
  void AdvanceFrame(WorldTime world_time);

 private:
#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
  FRIEND_TEST(AudioEngineTests, SamePriorityDifferentStartTimes);
  FRIEND_TEST(AudioEngineTests, IncreasingPriority);
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

  // Represents a sample that is playing on a channel.
  struct PlayingSound {
    PlayingSound(SoundId sid, ChannelId cid, WorldTime time)
        : sound_id(sid),
          channel_id(cid),
          start_time(time) {
    }

    SoundId sound_id;
    ChannelId channel_id;
    WorldTime start_time;
  };

  class PriorityComparitor {
   public:
    PriorityComparitor(const std::vector<Sound>* sounds) : sounds_(sounds) {}
    int operator()(const PlayingSound& a, const PlayingSound& b);
   private:
    const std::vector<Sound>* sounds_;
  };

  // Return true if the given AudioEngine::PlayingSound has finished playing.
  static bool CheckFinishedPlaying(const PlayingSound& playing_sound);

  // Remove all sounds that are no longer playing.
  void ClearFinishedSounds();

  static void PrioritizeChannels(
    const std::vector<Sound>& sounds,
    std::vector<PlayingSound>* playing_sounds);

  // Hold the audio bus list.
  std::string buses_source_;

  // Hold the sounds.
  std::vector<Sound> sounds_;

  // The number of sounds currently playing.
  std::vector<PlayingSound> playing_sounds_;

  WorldTime world_time_;
};

}  // namespace fpl

#endif  // SPLAT_AUDIO_ENGINE_H_
