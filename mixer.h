/*
    MIXERLIB:  An audio mixer library based on the SDL library
    Copyright (C) 1997-1999  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

#ifndef _MIXER_H_
#define _MIXER_H_

#include <SDL/SDL_types.h>
#include <SDL/SDL_rwops.h>
#include <SDL/SDL_audio.h>

#include <SDL/begin_code.h>

/* The default mixer has 8 simultaneous mixing channels */
#ifndef MIX_CHANNELS
#define MIX_CHANNELS	8
#endif

/* Good default values for a PC soundcard */
#define MIX_DEFAULT_FREQUENCY	22050
#define MIX_DEFAULT_FORMAT	AUDIO_S16
#define MIX_DEFAULT_CHANNELS	2
#define MIX_MAX_VOLUME		128	/* Volume of a chunk */

/* The internal format for an audio chunk */
typedef struct {
	int allocated;
	Uint8 *abuf;
	Uint32 alen;
	Uint8 volume;		/* Per-sample volume, 0-128 */
} Mix_Chunk;

/* The internal format for a music chunk interpreted via mikmod */
typedef struct _Mix_Music Mix_Music;

/* Open the mixer with a certain audio format */
extern int Mix_OpenAudio(int frequency, Uint16 format, int channels,
							int chunksize);

/* Find out what the actual audio device parameters are.
   This function returns 1 if the audio has been opened, 0 otherwise.
 */
extern int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels);

/* Load a wave file or a music (.mod .s3m .it .xm) file */
extern Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc);
#define Mix_LoadWAV(file)	Mix_LoadWAV_RW(SDL_RWFromFile(file, "rb"), 1)
extern Mix_Music *Mix_LoadMUS(const char *file);

/* Load a wave file of the mixer format from a memory buffer */
extern Mix_Chunk *Mix_QuickLoad_WAV(Uint8 *mem);

/* Free an audio chunk previously loaded */
extern void Mix_FreeChunk(Mix_Chunk *chunk);
extern void Mix_FreeMusic(Mix_Music *music);

/* Add your own music player or additional mixer function.
   If 'mix_func' is NULL, the default music player is re-enabled.
 */
extern void Mix_HookMusic(void (*mix_func)(void *udata, Uint8 *stream, int len),
                                                                void *arg);

/* Get a pointer to the user data for the current music hook */
extern void *Mix_GetMusicHookData(void);

/* Reserve the first channels (0 -> n-1) for the application, i.e. don't allocate
   them dynamically to the next sample if requested with a -1 value below.
   Returns the number of reserved channels.
 */
extern int Mix_ReserveChannels(int num);

/* Play an audio chunk on a specific channel.
   If the specified channel is -1, play on the first free channel.
   If 'loops' is greater than zero, loop the sound that many times.
   If 'loops' is -1, loop inifinitely (~65000 times).
   Returns which channel was used to play the sound.
*/
extern int Mix_PlayChannel(int channel, Mix_Chunk *chunk, int loops);
extern int Mix_PlayMusic(Mix_Music *music, int loops);

/* Set the volume in the range of 0-128 of a specific channel or chunk.
   If the specified channel is -1, set volume for all channels.
   Returns the original volume.
   If the specified volume is -1, just return the current volume.
*/
extern int Mix_Volume(int channel, int volume);
extern int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
extern int Mix_VolumeMusic(int volume);

/* Halt playing of a particular channel */
extern int Mix_HaltChannel(int channel);
extern int Mix_HaltMusic(void);

/* Pause/Resume a particular channel */
extern void Mix_Pause(int channel);
extern void Mix_Resume(int channel);

/* Pause/Resume the music stream */
extern void Mix_PauseMusic(void);
extern void Mix_ResumeMusic(void);
extern void Mix_RewindMusic(void);

/* Check the status of a specific channel.
   If the specified channel is -1, check all channels.
*/
extern int Mix_Playing(int channel);
extern int Mix_PlayingMusic(void);

/* Stop music and set external music playback command */
extern int Mix_SetMusicCMD(const char *command);

/* Close the mixer, halting all playing audio */
extern void Mix_CloseAudio(void);

/* We'll use SDL for reporting errors */
#define Mix_SetError	SDL_SetError
#define Mix_GetError	SDL_GetError

#include <SDL/close_code.h>

#endif /* _MIXER_H_ */
