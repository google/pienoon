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
int Mix_HaltMusic() { return 0; }
int Mix_Init(int) { return 0; }
int Mix_OpenAudio(int, Uint16, int, int) { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk*, int, int) { return 0; }
int Mix_FadeOutChannel(int, int) { return 0; }
int Mix_PlayMusic(Mix_Music*, int) { return 0; }
int Mix_Playing(int) { return 0; }
int Mix_PlayingMusic() { return 0; }
int Mix_Volume(int, int) { return 0; }
int Mix_VolumeMusic(int) { return 0; }
void Mix_CloseAudio() {}
void Mix_FreeChunk(Mix_Chunk*) {}
void Mix_FreeMusic(Mix_Music*) {}
void Mix_Pause(int) {}
void Mix_PauseMusic() {}
int Mix_FadeOutMusic(int) { return 0; }
void Mix_Resume(int) {}
void Mix_ResumeMusic() {}
}

namespace fpl {

bool LoadFile(const char*, std::string*) { return false; }

class AudioEngineTests : public ::testing::Test {
 protected:
  virtual void SetUp() {
    // Make a bunch of sound defs with various priorities.
    for (uint16_t i = 0; i < 6; ++i) {
      flatbuffers::FlatBufferBuilder builder;
      auto id = static_cast<pie_noon::SoundId>(i);
      float priority = static_cast<float>(i);
      auto sound_def_buffer = fpl::CreateSoundCollectionDef(builder,
                                                            id, priority);
      builder.Finish(sound_def_buffer);
      collections_.push_back(std::unique_ptr<fpl::SoundCollection>(
          new fpl::SoundCollection()));
      collections_.back()->LoadSoundCollectionDef(
          std::string(reinterpret_cast<const char*>(builder.GetBufferPointer()),
                      builder.GetSize()),
          nullptr);
    }
  }
  virtual void TearDown() {}

 protected:
  fpl::AudioEngine::SoundCollections collections_;
};

TEST_F(AudioEngineTests, IncreasingPriority) {
  std::vector<fpl::AudioEngine::PlayingSound> sounds;
  sounds.push_back(AudioEngine::PlayingSound(collections_[0].get(), 0, 0));
  sounds.push_back(AudioEngine::PlayingSound(collections_[1].get(), 1, 1));
  sounds.push_back(AudioEngine::PlayingSound(collections_[2].get(), 2, 2));
  sounds.push_back(AudioEngine::PlayingSound(collections_[3].get(), 3, 3));
  sounds.push_back(AudioEngine::PlayingSound(collections_[4].get(), 4, 4));
  sounds.push_back(AudioEngine::PlayingSound(collections_[5].get(), 5, 5));
  AudioEngine::PrioritizeChannels(&sounds);
  EXPECT_EQ(0, sounds[5].channel_id);
  EXPECT_EQ(1, sounds[4].channel_id);
  EXPECT_EQ(2, sounds[3].channel_id);
  EXPECT_EQ(3, sounds[2].channel_id);
  EXPECT_EQ(4, sounds[1].channel_id);
  EXPECT_EQ(5, sounds[0].channel_id);
}

TEST_F(AudioEngineTests, SamePriorityDifferentStartTimes) {
  std::vector<AudioEngine::PlayingSound> sounds;
  // Sounds with the same priority but later start times should be higher
  // priority.
  sounds.push_back(AudioEngine::PlayingSound(collections_[0].get(), 0, 1));
  sounds.push_back(AudioEngine::PlayingSound(collections_[0].get(), 1, 0));
  sounds.push_back(AudioEngine::PlayingSound(collections_[1].get(), 2, 1));
  sounds.push_back(AudioEngine::PlayingSound(collections_[1].get(), 3, 0));
  sounds.push_back(AudioEngine::PlayingSound(collections_[2].get(), 4, 1));
  sounds.push_back(AudioEngine::PlayingSound(collections_[2].get(), 5, 0));
  AudioEngine::PrioritizeChannels(&sounds);
  EXPECT_EQ(0, sounds[5].channel_id);
  EXPECT_EQ(1, sounds[4].channel_id);
  EXPECT_EQ(2, sounds[3].channel_id);
  EXPECT_EQ(3, sounds[2].channel_id);
  EXPECT_EQ(4, sounds[1].channel_id);
  EXPECT_EQ(5, sounds[0].channel_id);
}

}  // namespace fpl

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
