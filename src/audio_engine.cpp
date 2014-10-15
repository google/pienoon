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
#include "SDL_mixer.h"
#include "audio_config_generated.h"
#include "audio_engine.h"
#include "bus.h"
#include "buses_generated.h"
#include "common.h"
#include "sound.h"
#include "sound_assets_generated.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"
#include "utilities.h"

namespace fpl {

const int kChannelFadeOutRateMs = 10;

// Special value for SDL_Mixer that indicates an operation should be applied to
// all channels.
const ChannelId kAllChannels = -1;

// Special value representing an audio stream.
static const ChannelId kStreamChannel = -100;

const ChannelId AudioEngine::kInvalidChannel = -1;

AudioEngine::PlayingSound::PlayingSound(
    SoundCollection* collection, ChannelId cid, WorldTime time)
    : sound_collection(collection),
      channel_id(cid),
      start_time(time) {
  Bus* bus = sound_collection->bus();
  if (bus) {
    bus->IncrementSoundCounter();
  }
}

AudioEngine::PlayingSound::PlayingSound(const AudioEngine::PlayingSound& other)
    : sound_collection(other.sound_collection),
      channel_id(other.channel_id),
      start_time(other.start_time) {
  Bus* bus = sound_collection->bus();
  if (bus) {
    bus->IncrementSoundCounter();
  }
}

AudioEngine::PlayingSound::~PlayingSound() {
  Bus* bus = sound_collection->bus();
  if (bus) {
    bus->DecrementSoundCounter();
  }
}

AudioEngine::PlayingSound& AudioEngine::PlayingSound::operator=(
    const AudioEngine::PlayingSound& other) {
  Bus* bus = sound_collection->bus();
  if (bus) {
    bus->DecrementSoundCounter();
  }
  Bus* other_bus = other.sound_collection->bus();
  if (other_bus) {
    other_bus->IncrementSoundCounter();
  }
  sound_collection = other.sound_collection;
  channel_id = other.channel_id;
  start_time = other.start_time;
  return *this;
}

AudioEngine::~AudioEngine() {
  for (size_t i = 0; i < collections_.size(); ++i) {
    collections_[i]->Unload();
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

  // We do our own tracking of audio channels so that when a new sound is
  // played we can determine if one of the currently playing channels is lower
  // priority so that we can drop it.
  playing_sounds_.reserve(config->mixer_channels());

  // Load the audio buses.
  if (!LoadFile("buses.bin", &buses_source_)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Can't load audio bus file.\n");
    return false;
  }
  const BusDefList* bus_def_list = GetBusDefList();
  buses_.reserve(bus_def_list->buses()->Length());
  for (size_t i = 0; i < bus_def_list->buses()->Length(); ++i) {
    buses_.push_back(Bus(bus_def_list->buses()->Get(i)));
  }

  // Set up the children and ducking pointers.
  for (size_t i = 0; i < buses_.size(); ++i) {
    Bus& bus = buses_[i];
    const BusDef* def = bus.bus_def();
    if (!PopulateBuses("child_buses", def->child_buses(), &bus.child_buses())) {
      return false;
    }
    if (!PopulateBuses("duck_buses", def->duck_buses(), &bus.duck_buses())) {
      return false;
    }
  }

  master_bus_ = FindBus("master");
  if (!master_bus_) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No master bus specified.\n");
    return false;
  }

  // Load the list of assets.
  std::string sound_assets_source;
  if (!LoadFile("sound_assets.bin", &sound_assets_source)) {
    return false;
  }

  // Create a SoundCollection for each SoundCollectionDef
  const SoundAssets* sound_assets = GetSoundAssets(sound_assets_source.c_str());
  unsigned int sound_count = sound_assets->sounds()->Length();
  collections_.resize(sound_count);
  bool success = true;
  for (unsigned int i = 0; i < sound_count; ++i) {
    const char* filename = sound_assets->sounds()->Get(i)->c_str();
    collections_[i].reset(new SoundCollection());
    if (!collections_[i]->LoadSoundCollectionDefFromFile(filename, this)) {
      // Don't return false if a sound collection fails to load, just null it
      // out and move on to the next one.
      collections_[i].reset(nullptr);
    }
  }

  mute_ = false;
  master_gain_ = 1.0f;

  return success;
}

class BusNameMatcher {
 public:
  BusNameMatcher(const char* name) : name_(name) {}
  bool operator()(const Bus& bus) {
    return strcmp(bus.bus_def()->name()->c_str(), name_) == 0;
  }
 private:
  const char* name_;
};

Bus* AudioEngine::FindBus(const char* name) {
  auto it = std::find_if(buses_.begin(), buses_.end(), BusNameMatcher(name));
  if (it != buses_.end()) {
    return &*it;
  }
  else {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "No bus named \"%s\"\n", name);
    return nullptr;
  }
}

bool AudioEngine::PopulateBuses(const char* list_name,
                                const BusNameList* child_name_list,
                                std::vector<Bus*>* output) {
  for (size_t i = 0; child_name_list && i < child_name_list->Length(); ++i) {
    const char* bus_name = child_name_list->Get(i)->c_str();
    Bus* bus = FindBus(bus_name);
    if (bus) {
      output->push_back(bus);
    } else {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Unknown bus \"%s\" listed in %s.\n", bus_name, list_name);
      return false;
    }
  }
  return true;
}

SoundCollection* AudioEngine::GetSoundCollection(SoundId sound_id) {
  if (sound_id >= static_cast<SoundId>(collections_.size())) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                 "Can't play audio sample: invalid sound_id (%d)\n", sound_id);
    return nullptr;
  }
  return collections_[sound_id].get();
}

static int AllocatedChannelCount() {
  // Passing negative values returns the number of allocated channels.
  return Mix_AllocateChannels(-1);
}

static int PlayingChannelCount() {
  // Passing negative values returns the number of playing channels.
  return Mix_Playing(-1);
}

static ChannelId FindFreeChannel(bool stream) {
  if (stream) {
    return kStreamChannel;
  }
  const int allocated_channels = AllocatedChannelCount();
  if (PlayingChannelCount() < allocated_channels) {
    for (int i = 0; i < allocated_channels; ++i) {
      if (!Mix_Playing(i)) {
        return i;
      }
    }
  }
  return AudioEngine::kInvalidChannel;
}

static int SoundCollectionDefComparitor(const SoundCollectionDef* a,
                                        const SoundCollectionDef* b) {
  // Since there can only be one stream playing, and that stream is independent
  // of the buffer channels, always make it highest priority.
  if (a->stream() || b->stream()) {
    return b->stream() - a->stream();
  } else {
    return (b->priority()- a->priority()) < 0 ? -1 : 1;
  }
}

// Sort by priority. In the case of two sounds with the same priority, sort
// the newer one as being higher priority. Higher priority elements have lower
// indicies.
int AudioEngine::PriorityComparitor(
    const AudioEngine::PlayingSound& a, const AudioEngine::PlayingSound& b) {
  int result = SoundCollectionDefComparitor(
      a.sound_collection->GetSoundCollectionDef(),
      b.sound_collection->GetSoundCollectionDef());
  if (result == 0) {
    return b.start_time - a.start_time;
  }
  return result;
}

// Sort channels with highest priority first.
void AudioEngine::PrioritizeChannels(
    std::vector<PlayingSound>* playing_sounds) {
  std::sort(playing_sounds->begin(), playing_sounds->end(), PriorityComparitor);
}

// Return true if the given AudioEngine::PlayingSound has finished playing.
bool AudioEngine::CheckFinishedPlaying(
    const AudioEngine::PlayingSound& playing_sound) {
  return !AudioEngine::Playing(playing_sound.channel_id);
}

// Remove all sounds that are no longer playing.
void AudioEngine::EraseFinishedSounds() {
  playing_sounds_.erase(std::remove_if(playing_sounds_.begin(),
                                       playing_sounds_.end(),
                                       CheckFinishedPlaying),
                        playing_sounds_.end());
}

// Return true if the given AudioEngine::PlayingSound is a stream.
bool AudioEngine::CheckIfStream(
    const AudioEngine::PlayingSound& playing_sound) {
  return playing_sound.channel_id == kStreamChannel;
}

// Remove all streams.
void AudioEngine::EraseStreams() {
  playing_sounds_.erase(std::remove_if(playing_sounds_.begin(),
                                       playing_sounds_.end(),
                                       CheckIfStream),
                        playing_sounds_.end());
}

bool AudioEngine::PlaySource(SoundSource* const source, ChannelId channel_id,
                             const SoundCollection& collection) {
  const SoundCollectionDef& def = *collection.GetSoundCollectionDef();
  const float gain =
    source->audio_sample_set_entry().audio_sample()->gain() * def.gain();
  source->SetGain(channel_id, gain);
  if (source->Play(channel_id, def.loop() != 0)) {
    return true;
  }
  return false;
}

const BusDefList* AudioEngine::GetBusDefList() const {
  return fpl::GetBusDefList(buses_source_.c_str());
}

void AudioEngine::Halt(ChannelId channel_id) {
  assert(channel_id != kInvalidChannel);
  if (channel_id == kStreamChannel) {
    Mix_HaltMusic();
  } else {
    Mix_HaltChannel(channel_id);
  }
}

bool AudioEngine::Playing(ChannelId channel_id) {
  assert(channel_id != kInvalidChannel);
  if (channel_id == kStreamChannel) {
    return Mix_PlayingMusic() != 0;
  } else {
    return Mix_Playing(channel_id) != 0;
  }
}

void AudioEngine::SetChannelGain(ChannelId channel_id, float volume) {
  assert(channel_id != kInvalidChannel);
  int mix_volume = static_cast<int>(volume * MIX_MAX_VOLUME);
  if (channel_id == kStreamChannel) {
    Mix_VolumeMusic(mix_volume);
  } else {
    Mix_Volume(channel_id, mix_volume);
  }
}

ChannelId AudioEngine::PlaySound(SoundId sound_id) {
  SoundCollection* collection = GetSoundCollection(sound_id);
  if (!collection) {
    return kInvalidChannel;
  }

  // Prune sounds that have finished playing.
  EraseFinishedSounds();

  bool stream = collection->GetSoundCollectionDef()->stream();
  ChannelId new_channel = FindFreeChannel(stream);

  // If there are no empty channels, clear out the one with the lowest
  // priority.
  if (new_channel == kInvalidChannel) {
    PrioritizeChannels(&playing_sounds_);
    // If the lowest priority sound is lower than the new one, halt it and
    // remove it from our list. Otherwise, do nothing.
    const SoundCollectionDef* new_def = collection->GetSoundCollectionDef();
    const SoundCollectionDef* back_def =
        playing_sounds_.back().sound_collection->GetSoundCollectionDef();
    if (SoundCollectionDefComparitor(new_def, back_def) < 0) {
      // Use the channel of the sound we're replacing.
      new_channel = playing_sounds_.back().channel_id;
      // Dispose of the lowest priority sound.
      Halt(new_channel);
      playing_sounds_.pop_back();
    } else {
      // The sound was lower priority than all currently playing sounds; do
      // nothing.
      return kInvalidChannel;
    }
  } else if (new_channel == kStreamChannel) {
    // Halt the sound the may currently be playing on this channel.
    if (Playing(new_channel)) {
      Halt(new_channel);
      EraseStreams();
    }
  }

  // At this point we should not have an invalid channel.
  assert(new_channel != kInvalidChannel);

  // Attempt to play the sound.
  if (PlaySource(collection->Select(), new_channel, *collection)) {
    playing_sounds_.push_back(
        PlayingSound(collection, new_channel, world_time_));
  }

  return new_channel;
}

void AudioEngine::Stop(ChannelId channel_id) {
  assert(channel_id != kInvalidChannel);
  // Fade out rather than halting to avoid clicks.
  if (channel_id == kStreamChannel) {
    int return_value = Mix_FadeOutMusic(kChannelFadeOutRateMs);
    if (return_value != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Error stopping music: %s\n", Mix_GetError());
    }
  } else {
    int return_value = Mix_FadeOutChannel(channel_id, kChannelFadeOutRateMs);
    if (return_value != 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Error stopping channel %d: %s\n", channel_id,
                   Mix_GetError());
    }
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
  WorldTime delta_time = world_time - world_time_;
  world_time_ = world_time;
  for (size_t i = 0; i < buses_.size(); ++i) {
    buses_[i].ResetDuckGain();
  }
  for (size_t i = 0; i < buses_.size(); ++i) {
    buses_[i].UpdateDuckGain(delta_time);
  }
  master_bus_->UpdateGain(mute_ ? 0.0f : master_gain_);
  for (size_t i = 0; i < playing_sounds_.size(); ++i) {
    PlayingSound& playing_sound = playing_sounds_[i];
    SetChannelGain(playing_sound.channel_id,
                   playing_sound.sound_collection->bus()->gain());
  }
}

}  // namespace fpl
