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

#include "precompiled.h"

#include <algorithm>
#include "audio_config_generated.h"
#include "audio_engine.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"
#include "sound_assets_generated.h"
#include "utilities.h"
#include "SDL_mixer.h"

namespace fpl {

const ChannelId kInvalidChannel = -1;

// Special value for SDL_Mixer that indicates an operation should be applied to
// all channels.
const ChannelId kAllChannels = -1;

AudioEngine::~AudioEngine() {
  for (size_t i = 0; i < collections_.size(); ++i) {
    collections_[i].Unload();
  }
  Mix_CloseAudio();
}

bool AudioEngine::Initialize(const AudioConfig* config) {
  // Initialize audio engine.
  if (Mix_OpenAudio(config->output_frequency(),
                    AUDIO_S16LSB,
                    config->output_channels(),
                    config->output_buffer_size()) != 0) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't open audio stream\n");
    return false;
  }

  // Initialize Ogg support. Returns a bitmask of formats that were successfully
  // initialized, so make sure ogg support was successfully loaded.
  if (Mix_Init(MIX_INIT_OGG) != MIX_INIT_OGG) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Error initializing Ogg support\n");
  }

  // Number of sound that can be played simutaniously.
  Mix_AllocateChannels(config->mixer_channels());

  // We do our own tracking of audio channels so that when a new sound is played
  // we can determine if one of the currently playing channels is lower priority
  // so that we can drop it.
  playing_sounds_.reserve(config->mixer_channels());

  // Load the audio buses.
  if (!LoadFile("buses.bin", &buses_source_)) {
    return false;
  }

  // Load the list of assets.
  std::string sound_assets_source;
  if (!LoadFile("sound_assets.bin", &sound_assets_source)) {
    return false;
  }

  // Create a Sound for each sound def
  const SoundAssets* sound_assets = GetSoundAssets(sound_assets_source.c_str());
  unsigned int sound_count = sound_assets->sounds()->Length();
  collections_.resize(sound_count);
  for (unsigned int i = 0; i < sound_count; ++i) {
    const char* filename = sound_assets->sounds()->Get(i)->c_str();
    if (!collections_[i].LoadSoundCollectionDefFromFile(filename)) {
      return false;
    }
  }

  return true;
}

SoundCollection* AudioEngine::GetSoundCollection(SoundId sound_id) {
  if (sound_id >= collections_.size()) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play audio sample: invalid sound_id\n");
    return nullptr;
  }
  return &collections_[sound_id];
}

static int AllocatedChannelCount() {
  // Passing negative values returns the number of allocated channels.
  return Mix_AllocateChannels(-1);
}

static int PlayingChannelCount() {
  // Passing negative values returns the number of playing channels.
  return Mix_Playing(-1);
}

static ChannelId FindFreeChannel() {
  const int allocated_channels = AllocatedChannelCount();
  if (PlayingChannelCount() < allocated_channels) {
    for (int i = 0; i < allocated_channels; ++i) {
      if (!Mix_Playing(i)) {
        return i;
      }
    }
  }
  return kInvalidChannel;
}

// Sort by priority. In the case of two sounds with the same priority, sort
// the newer one as being higher priority. Higher priority elements have lower
// indicies.
int AudioEngine::PriorityComparitor::operator()(
    const AudioEngine::PlayingSound& a, const AudioEngine::PlayingSound& b) {
  const float priority_a = a.sound_collection_def->priority();
  const float priority_b = b.sound_collection_def->priority();
  if (priority_a != priority_b) {
    return (priority_b - priority_a) < 0 ? -1 : 1;
  } else {
    return b.start_time - a.start_time;
  }
}

// Sort channels with highest priority first.
void AudioEngine::PrioritizeChannels(
    const std::vector<SoundCollection>& sounds,
    std::vector<PlayingSound>* playing_sounds) {
  std::sort(playing_sounds->begin(), playing_sounds->end(),
            PriorityComparitor(&sounds));
}

// Return true if the given AudioEngine::PlayingSound has finished playing.
bool AudioEngine::CheckFinishedPlaying(
    const AudioEngine::PlayingSound& playing_sound) {
  return !Mix_Playing(playing_sound.channel_id);
}

// Remove all sounds that are no longer playing.
void AudioEngine::ClearFinishedSounds() {
  playing_sounds_.erase(std::remove_if(playing_sounds_.begin(),
                                       playing_sounds_.end(),
                                       CheckFinishedPlaying),
                        playing_sounds_.end());
}

void AudioEngine::PlayStream(SoundCollection* collection) {
  // Attempt to play the stream.
  collection->Select()->Play(0, collection->GetSoundCollectionDef()->loop());
}

void AudioEngine::PlayBuffer(SoundCollection* collection) {
  const SoundCollectionDef* def = collection->GetSoundCollectionDef();

  // Prune sounds that have finished playing.
  ClearFinishedSounds();

  // Prepare a new AudioEngine::PlayingSound with a tentative channel.
  AudioEngine::PlayingSound new_sound(def, FindFreeChannel(), world_time_);

  // If there are no empty channels, clear out the one with the lowest priority.
  if (new_sound.channel_id == kInvalidChannel) {
    PrioritizeChannels(collections_, &playing_sounds_);
    // If the lowest priority sound is lower than the new one, halt it and
    // remove it from our list. Otherwise, do nothing.
    PriorityComparitor comparitor(&collections_);
    if (comparitor(new_sound, playing_sounds_.back()) < 0) {
      // Use the channel of the sound we're replacing.
      new_sound.channel_id = playing_sounds_.back().channel_id;
      // Dispose of the lowest priority sound.
      Mix_HaltChannel(new_sound.channel_id);
      playing_sounds_.pop_back();
    } else {
      // The sound was lower priority than all currently playing sounds; do
      // nothing.
      return;
    }
  }

  // At this point we should not have an invalid channel.
  assert(new_sound.channel_id != kInvalidChannel);

  // Attempt to play the sound.
  if (collection->Select()->Play(new_sound.channel_id, def->loop())) {
    playing_sounds_.push_back(new_sound);
  }
}

void AudioEngine::PlaySound(SoundId sound_id) {
  SoundCollection* collection = GetSoundCollection(sound_id);
  if (collection) {
    if (collection->GetSoundCollectionDef()->stream()) {
      PlayStream(collection);
    } else {
      PlayBuffer(collection);
    }
  }
}

void AudioEngine::Mute(bool mute) {
  if (mute) {
    Mix_Volume(kAllChannels, 0);
    Mix_VolumeMusic(0);
  } else {
    Mix_Volume(kAllChannels, MIX_MAX_VOLUME);
    Mix_VolumeMusic(MIX_MAX_VOLUME);
  }
}

void AudioEngine::Pause(bool pause) {
  if (pause) {
    Mix_Pause(kAllChannels);
    Mix_PauseMusic();
  } else {
    Mix_Resume(kAllChannels);
    Mix_ResumeMusic();
  }
}

void AudioEngine::AdvanceFrame(WorldTime world_time) {
  world_time_ = world_time;
  // TODO: Update audio volume per channel each frame. b/17316699
}

}  // namespace fpl

