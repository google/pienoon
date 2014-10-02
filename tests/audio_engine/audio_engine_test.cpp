// Copyright (c) 2014 Google, Inc.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#define FPL_AUDIO_ENGINE_UNIT_TESTS

#include <vector>
#include "SDL_mixer.h"
#include "audio_engine.h"
#include "gtest/gtest.h"
#include "sound.h"
#include "sound_collection.h"
#include "sound_collection_def_generated.h"

// Stubs for SDL_mixer functions which are not actually part of the tests being
// run.
extern "C" {
Mix_Chunk* Mix_LoadWAV_RW(SDL_RWops*, int) { return NULL; }
Mix_Music* Mix_LoadMUS(const char*) { return NULL; }
const char* SDL_GetError(void) { return NULL; }
int Mix_AllocateChannels(int) { return 0; }
int Mix_HaltChannel(int) { return 0; }
int Mix_Init(int) { return 0; }
int Mix_OpenAudio(int, Uint16, int, int) { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk*, int, int) { return 0; }
int Mix_PlayMusic(Mix_Music*, int) { return 0; }
int Mix_Playing(int) { return 0; }
void Mix_CloseAudio() {}
void Mix_FreeChunk(Mix_Chunk*) {}
void Mix_FreeMusic(Mix_Music*) {}
}

namespace fpl {

bool LoadFile(const char*, std::string*) { return false; }

class AudioEngineTests : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Make a bunch of sound defs with various priorities.
    for (uint16_t i = 0; i < 6; ++i) {
      flatbuffers::FlatBufferBuilder builder;
      auto id = static_cast<splat::SoundId>(i);
      float priority = static_cast<float>(i);
      auto sound_def_buffer = fpl::CreateSoundCollectionDef(builder,
                                                            id, priority);
      builder.Finish(sound_def_buffer);
      collections_.push_back(fpl::SoundCollection());
      collections_.back().LoadSoundCollectionDef(
          std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                      builder.GetSize()));
    }
  }
  virtual void TearDown() {}

 protected:
  std::vector<fpl::SoundCollection> collections_;
};

TEST_F(AudioEngineTests, IncreasingPriority) {
  std::vector<fpl::AudioEngine::PlayingSound> playing_sounds;
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[0].GetSoundCollectionDef(), 0, 0));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[1].GetSoundCollectionDef(), 1, 1));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[2].GetSoundCollectionDef(), 2, 2));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[3].GetSoundCollectionDef(), 3, 3));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[4].GetSoundCollectionDef(), 4, 4));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[5].GetSoundCollectionDef(), 5, 5));
  fpl::AudioEngine::PrioritizeChannels(collections_, &playing_sounds);
  EXPECT_EQ(0, playing_sounds[5].channel_id);
  EXPECT_EQ(1, playing_sounds[4].channel_id);
  EXPECT_EQ(2, playing_sounds[3].channel_id);
  EXPECT_EQ(3, playing_sounds[2].channel_id);
  EXPECT_EQ(4, playing_sounds[1].channel_id);
  EXPECT_EQ(5, playing_sounds[0].channel_id);
}

TEST_F(AudioEngineTests, SamePriorityDifferentStartTimes) {
  std::vector<AudioEngine::PlayingSound> playing_sounds;
  // Sounds with the same priority but later start times should be higher
  // priority.
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[0].GetSoundCollectionDef(), 0, 1));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[0].GetSoundCollectionDef(), 1, 0));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[1].GetSoundCollectionDef(), 2, 1));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[1].GetSoundCollectionDef(), 3, 0));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[2].GetSoundCollectionDef(), 4, 1));
  playing_sounds.push_back(fpl::AudioEngine::PlayingSound(
      collections_[2].GetSoundCollectionDef(), 5, 0));
  fpl::AudioEngine::PrioritizeChannels(collections_, &playing_sounds);
  EXPECT_EQ(0, playing_sounds[5].channel_id);
  EXPECT_EQ(1, playing_sounds[4].channel_id);
  EXPECT_EQ(2, playing_sounds[3].channel_id);
  EXPECT_EQ(3, playing_sounds[2].channel_id);
  EXPECT_EQ(4, playing_sounds[1].channel_id);
  EXPECT_EQ(5, playing_sounds[0].channel_id);
}

}  // namespace fpl

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
