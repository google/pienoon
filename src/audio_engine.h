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

#ifndef PIE_NOON_AUDIO_ENGINE_H_
#define PIE_NOON_AUDIO_ENGINE_H_

#include "common.h"
#include "sound.h"
#include "sound_collection.h"
#include "pie_noon_common_generated.h"
#include "sound_collection_def_generated.h"

#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
#include "gtest/gtest.h"
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

namespace fpl {

struct AudioConfig;

// TODO(amablue): Remove pie_noon dependency.
// What's the right thing to do when SoundId is defined in pie_noon_common.fbs?
using pie_noon::SoundId;

class AudioEngine {
 public:
  typedef std::vector<std::unique_ptr<SoundCollection>> SoundCollections;

  static const ChannelId kInvalidChannel;

  ~AudioEngine();

  bool Initialize(const AudioConfig* config);

  // Play a sound associated with the given sound_id.
  // Returns the channel the sound is played on.
  ChannelId PlaySound(SoundId sound_id);

  // Checks if the sound playing on a given channel is playing
  bool IsPlaying(ChannelId channel_id) const;

  // Stop a channel.
  void Stop(ChannelId channel_id);

  // Returns the audio collection associated with the given sound_id.
  SoundCollection* GetSoundCollection(SoundId sound_id);

  // Mutes the audio engine completely.
  void Mute(bool mute);

  // Pauses all playing sounds and streams.
  void Pause(bool pause);

  // TODO: Update audio volume per channel each frame. b/17316699
  void AdvanceFrame(WorldTime world_time);

 private:
#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
  FRIEND_TEST(AudioEngineTests, SamePriorityDifferentStartTimes);
  FRIEND_TEST(AudioEngineTests, IncreasingPriority);
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

  // Represents a sample that is playing on a channel.
  struct PlayingSound {
    PlayingSound(const SoundCollectionDef* def, ChannelId cid, WorldTime time)
        : sound_collection_def(def),
          channel_id(cid),
          start_time(time) {
    }

    const SoundCollectionDef* sound_collection_def;
    ChannelId channel_id;
    WorldTime start_time;
  };

  // Play a buffer associated with the given sound_id.
  ChannelId PlayBuffer(SoundCollection* sound);

  // Play a stream associated with the given sound_id.
  ChannelId PlayStream(SoundCollection* sound);

  class PriorityComparitor {
   public:
    PriorityComparitor(const SoundCollections* collections)
        : collections_(collections) {}
    int operator()(const PlayingSound& a, const PlayingSound& b);
   private:
    const SoundCollections* collections_;
  };

  // Return true if the given AudioEngine::PlayingSound has finished playing.
  static bool CheckFinishedPlaying(const PlayingSound& playing_sound);

  // Remove all sounds that are no longer playing.
  void ClearFinishedSounds();

  static void PrioritizeChannels(
    const SoundCollections& collections,
    std::vector<PlayingSound>* playing_sounds);

  // Play a source selected from a collection on the specified channel.
  static bool PlaySource(SoundSource* const source, ChannelId channel_id,
                         const SoundCollection& collection);

  // Hold the audio bus list.
  std::string buses_source_;

  // Hold the sounds.
  SoundCollections collections_;

  // The number of sounds currently playing.
  std::vector<PlayingSound> playing_sounds_;

  WorldTime world_time_;
};

}  // namespace fpl

#endif  // PIE_NOON_AUDIO_ENGINE_H_
