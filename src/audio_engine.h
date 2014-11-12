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

#ifndef FPL_AUDIO_ENGINE_H_
#define FPL_AUDIO_ENGINE_H_

#include "precompiled.h"
#include <vector>
#include "bus.h"
#include "common.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
#include "gtest/gtest.h"
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

namespace fpl {

struct AudioConfig;
struct BusDefList;

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

  // Immediately halts a sound.
  static void Halt(ChannelId channel_id);

  // Checks if the sound playing on a given channel is playing
  static bool Playing(ChannelId channel_id);

  // Stop a channel.
  void Stop(ChannelId channel_id);

  // Returns the audio collection associated with the given sound_id.
  SoundCollection* GetSoundCollection(SoundId sound_id);

  // master_volumes the audio engine completely.
  void set_master_gain(float master_gain) { master_gain_ = master_gain; }
  float master_gain() { return master_gain_; }

  // Mutes the audio engine completely.
  void set_mute(bool mute) { mute_ = mute; }
  bool mute() { return mute_; }

  // Pauses all playing sounds and streams.
  void Pause(bool pause);

  // TODO: Update audio volume per channel each frame. b/17316699
  void AdvanceFrame(WorldTime world_time);

  // Find a bus by the given name. Returns a nullptr if no bus by that name
  // exists.
  Bus* FindBus(const char* name);

 private:
#ifdef FPL_AUDIO_ENGINE_UNIT_TESTS
  FRIEND_TEST(AudioEngineTests, SamePriorityDifferentStartTimes);
  FRIEND_TEST(AudioEngineTests, IncreasingPriority);
#endif  // FPL_AUDIO_ENGINE_UNIT_TESTS

  // Represents a sample that is playing on a channel.
  struct PlayingSound {
    PlayingSound(SoundCollection* collection, ChannelId cid, WorldTime time);
    PlayingSound(const PlayingSound& other);
    PlayingSound& operator=(const PlayingSound& other);
    ~PlayingSound();

    SoundCollection* sound_collection;
    ChannelId channel_id;
    WorldTime start_time;
  };

  // Get the bus definitions.
  const BusDefList* GetBusDefList() const;

  // Populate a Bus's child or duck buses.
  typedef flatbuffers::Vector<flatbuffers::Offset<flatbuffers::String>>
      BusNameList;
  bool PopulateBuses(const char* list_name, const BusNameList* child_name_list,
                     std::vector<Bus*>* output);

  // Set the volume of a channel.
  static void SetChannelGain(ChannelId channel_id, float volume);

  // Remove all sounds that are no longer playing.
  void EraseFinishedSounds();

  // Remove all streams.
  void EraseStreams();

  // Compares the priority of two PlayingSounds.
  static int PriorityComparitor(const PlayingSound& a, const PlayingSound& b);

  // Sort all playing sounds based on the PriorityComparitor.
  static void PrioritizeChannels(std::vector<PlayingSound>* playing_sounds);

  // Play a source selected from a collection on the specified channel.
  static bool PlaySource(SoundSource* const source, ChannelId channel_id,
                         const SoundCollection& collection);

  // Hold the audio bus list.
  std::string buses_source_;

  // The state of the buses.
  std::vector<Bus> buses_;

  // The master bus, cached to prevent needless lookups.
  Bus* master_bus_;

  // The gain applied to all buses.
  float master_gain_;

  // If true, the master gain is ignored and all channels have a gain of 0.
  bool mute_;

  // Hold the sounds.
  SoundCollections collections_;

  // A list of the currently playing sounds.
  std::vector<PlayingSound> playing_sounds_;

  WorldTime world_time_;
};

}  // namespace fpl

#endif  // FPL_AUDIO_ENGINE_H_

