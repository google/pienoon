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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL/SDL_mutex.h>
#include <SDL/SDL_endian.h>

#include "mixer.h"

static int audio_opened = 0;

static SDL_AudioSpec mixer;
static SDL_mutex *mixer_lock;
static struct _Mix_Channel {
	Mix_Chunk *chunk;
	int playing;
	int paused;
	Uint8 *samples;
	int volume;
	int looping;
} channel[MIX_CHANNELS];

#define MUSIC_VOL	64		/* Music volume 0-64 */

/* Support for user defined music functions, plus the default one */
extern int music_active;
extern void music_mixer(void *udata, Uint8 *stream, int len);
static void (*mix_music)(void *udata, Uint8 *stream, int len) = music_mixer;
static int num_channels; /* Holds the number of channels actually mixed, for debugging */
static int reserved_channels = 0;
void *music_data = NULL;

/* Mixing function */
static void mix_channels(void *udata, Uint8 *stream, int len)
{
	int i, mixable, volume;

	/* Grab the channels we need to mix */
	SDL_mutexP(mixer_lock);
	num_channels = 0;
	for ( i=0; i<MIX_CHANNELS; ++i ) {
		if ( channel[i].playing && !channel[i].paused ) {
		    ++ num_channels;
			mixable = channel[i].playing;
			if ( mixable > len ) {
				mixable = len;
			}
			volume = (channel[i].volume*channel[i].chunk->volume) /
								MIX_MAX_VOLUME;
			SDL_MixAudio(stream,channel[i].samples,mixable,volume);
			channel[i].samples += mixable;
			channel[i].playing -= mixable;
			if ( ! channel[i].playing && channel[i].looping ) {
				if ( --channel[i].looping ) {
				    channel[i].samples = channel[i].chunk->abuf;
				    channel[i].playing = channel[i].chunk->alen;
				}
			}
		}
	}
	SDL_mutexV(mixer_lock);

	/* Mix the music */
	if ( music_active ) {
		mix_music(music_data, stream, len);
	}
}

static void PrintFormat(char *title, SDL_AudioSpec *fmt)
{
	printf("%s: %d bit %s audio (%s) at %u Hz\n", title, (fmt->format&0xFF),
			(fmt->format&0x8000) ? "signed" : "unsigned",
			(fmt->channels > 1) ? "stereo" : "mono", fmt->freq);
}

/* Open the mixer with a certain desired audio format */
int Mix_OpenAudio(int frequency, Uint16 format, int channels, int chunksize)
{
	int i;
	SDL_AudioSpec desired;

	/* If the mixer is already opened, increment open count */
	if ( audio_opened ) {
	    ++audio_opened;
	    return(0);
	}

	/* Set the desired format and frequency */
	desired.freq = frequency;
	desired.format = format;
	desired.channels = channels;
	desired.samples = chunksize;
	desired.callback = mix_channels;
	desired.userdata = NULL;

	/* Accept nearly any audio format */
	if ( SDL_OpenAudio(&desired, &mixer) < 0 ) {
		return(-1);
	}
#if 0
	PrintFormat("Audio device", &mixer);
#endif

	/* Create the channel lock mutex */
	mixer_lock = SDL_CreateMutex();
	if ( mixer_lock == NULL ) {
		SDL_CloseAudio();
		SDL_SetError("Unable to create mixer lock");
		return(-1);
	}

	/* Initialize the music players */
	if ( open_music(&mixer) < 0 ) {
		SDL_CloseAudio();
		SDL_DestroyMutex(mixer_lock);
		return(-1);
	}
	
	/* Clear out the audio channels */
	for ( i=0; i<MIX_CHANNELS; ++i ) {
		channel[i].chunk = NULL;
		channel[i].playing = 0;
		channel[i].looping = 0;
		channel[i].volume = SDL_MIX_MAXVOLUME;
	}
	Mix_VolumeMusic(SDL_MIX_MAXVOLUME);
	audio_opened = 1;
	SDL_PauseAudio(0);
	return(0);
}

/* Return the actual mixer parameters */
int Mix_QuerySpec(int *frequency, Uint16 *format, int *channels)
{
	if ( audio_opened ) {
		if ( frequency ) {
			*frequency = mixer.freq;
		}
		if ( format ) {
			*format = mixer.format;
		}
		if ( channels ) {
			*channels = mixer.channels;
		}
	}
	return(audio_opened);
}

/* Load a wave file */
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *src, int freesrc)
{
	Mix_Chunk *chunk;
	SDL_AudioSpec wavespec;
	SDL_AudioCVT wavecvt;

	/* Make sure audio has been opened */
	if ( ! audio_opened ) {
		SDL_SetError("Audio device hasn't been opened");
		if ( freesrc ) {
			SDL_RWclose(src);
		}
		return(NULL);
	}

	/* Allocate the chunk memory */
	chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
	if ( chunk == NULL ) {
		SDL_SetError("Out of memory");
		if ( freesrc ) {
			SDL_RWclose(src);
		}
		return(NULL);
	}

	/* Load the WAV file into the chunk */
	if ( SDL_LoadWAV_RW(src, freesrc,
		&wavespec, (Uint8 **)&chunk->abuf, &chunk->alen) == NULL ) {
		free(chunk);
		return(NULL);
	}
#if 0
	PrintFormat("Audio device", &mixer);
	PrintFormat("-- Wave file", &wavespec);
#endif

	/* Build the audio converter and create conversion buffers */
	if ( SDL_BuildAudioCVT(&wavecvt,
			wavespec.format, wavespec.channels, wavespec.freq,
			mixer.format, mixer.channels, mixer.freq) < 0 ) {
		SDL_FreeWAV(chunk->abuf);
		free(chunk);
		return(NULL);
	}
	wavecvt.len = chunk->alen;
	wavecvt.buf = (Uint8 *)malloc(wavecvt.len*wavecvt.len_mult);
	if ( wavecvt.buf == NULL ) {
		SDL_SetError("Out of memory");
		SDL_FreeWAV(chunk->abuf);
		free(chunk);
		return(NULL);
	}
	memcpy(wavecvt.buf, chunk->abuf, chunk->alen);
	SDL_FreeWAV(chunk->abuf);

	/* Run the audio converter */
	if ( SDL_ConvertAudio(&wavecvt) < 0 ) {
		free(wavecvt.buf);
		free(chunk);
		return(NULL);
	}
	chunk->abuf = wavecvt.buf;
	chunk->alen = wavecvt.len_cvt;
	chunk->volume = MIX_MAX_VOLUME;
	return(chunk);
}

/* Free an audio chunk previously loaded */
void Mix_FreeChunk(Mix_Chunk *chunk)
{
	int i;

	/* Caution -- if the chunk is playing, the mixer will crash */
	if ( chunk ) {
		/* Guarantee that this chunk isn't playing */
		SDL_mutexP(mixer_lock);
		for ( i=0; i<MIX_CHANNELS; ++i ) {
			if ( chunk == channel[i].chunk ) {
				channel[i].playing = 0;
			}
		}
		SDL_mutexV(mixer_lock);

		/* Actually free the chunk */
		free(chunk->abuf);
		free(chunk);
	}
}

/* Add your own music player or mixer function.
   If 'mix_func' is NULL, the default music player is re-enabled.
 */
void Mix_HookMusic(void (*mix_func)(void *udata, Uint8 *stream, int len),
                                                                void *arg)
{
	SDL_LockAudio();
	if ( mix_func != NULL ) {
		music_data = arg;
		mix_music = mix_func;
	} else {
		music_data = NULL;
		mix_music = music_mixer;
	}
	SDL_UnlockAudio();
}

void *Mix_GetMusicHookData(void)
{
	return(music_data);
}

/* Reserve the first channels (0 -> n-1) for the application, i.e. don't allocate
   them dynamically to the next sample if requested with a -1 value below.
   Returns the number of reserved channels.
 */
int Mix_ReserveChannels(int num)
{
	if (num > MIX_CHANNELS)
		num = MIX_CHANNELS;
	reserved_channels = num;
	return num;
}

/* Play an audio chunk on a specific channel.
   If the specified channel is -1, play on the first free channel.
   Returns which channel was used to play the sound.
*/
int Mix_PlayChannel(int which, Mix_Chunk *chunk, int loops)
{
	int i;

	/* Don't play null pointers :-) */
	if ( chunk == NULL ) {
		return(-1);
	}

	/* Lock the mixer while modifying the playing channels */
	SDL_mutexP(mixer_lock);
	{
		/* If which is -1, play on the first free channel */
		if ( which == -1 ) {
			for ( i=reserved_channels; i<MIX_CHANNELS; ++i ) {
				if ( channel[i].playing <= 0 )
					break;
			}
			if ( i == MIX_CHANNELS ) {
				which = -1;
			} else {
				which = i;
			}
		}

		/* Queue up the audio data for this channel */
		if ( which >= 0 ) {
			channel[which].samples = chunk->abuf;
			channel[which].playing = chunk->alen;
			channel[which].looping = loops;
			channel[which].chunk = chunk;
			channel[which].paused = 0;
		}
	}
	SDL_mutexV(mixer_lock);

	/* Return the channel on which the sound is being played */
	return(which);
}

/* Set volume of a particular channel */
int Mix_Volume(int which, int volume)
{
	int i;
	int prev_volume;

	if ( which == -1 ) {
		prev_volume = 0;
		for ( i=0; i<MIX_CHANNELS; ++i ) {
			prev_volume += Mix_Volume(i, volume);
		}
		prev_volume /= MIX_CHANNELS;
	} else {
		prev_volume = channel[which].volume;
		if ( volume >= 0 ) {
			if ( volume > SDL_MIX_MAXVOLUME ) {
				volume = SDL_MIX_MAXVOLUME;
			}
			channel[which].volume = volume;
		}
	}
	return(prev_volume);
}
/* Set volume of a particular chunk */
int Mix_VolumeChunk(Mix_Chunk *chunk, int volume)
{
	int prev_volume;

	prev_volume = chunk->volume;
	if ( volume >= 0 ) {
		if ( volume > MIX_MAX_VOLUME ) {
			volume = MIX_MAX_VOLUME;
		}
		chunk->volume = volume;
	}
	return(prev_volume);
}

/* Halt playing of a particular channel */
int Mix_HaltChannel(int which)
{
	int i;

	if ( which == -1 ) {
		for ( i=0; i<MIX_CHANNELS; ++i ) {
			Mix_HaltChannel(i);
		}
	} else {
		SDL_mutexP(mixer_lock);
		channel[which].playing = 0;
		SDL_mutexV(mixer_lock);
	}
	return(0);
}

/* Check the status of a specific channel.
   If the specified channel is -1, check all channels.
*/
int Mix_Playing(int which)
{
	int status;

	status = 0;
	if ( which == -1 ) {
		int i;

		for ( i=0; i<MIX_CHANNELS; ++i ) {
			if ( channel[i].playing > 0 ) {
				++status;
			}
		}
	} else {
		if ( channel[which].playing > 0 ) {
			++status;
		}
	}
	return(status);
}

/* Close the mixer, halting all playing audio */
void Mix_CloseAudio(void)
{
	if ( audio_opened ) {
		if ( audio_opened == 1 ) {
			close_music();
			Mix_HaltChannel(-1);
			SDL_CloseAudio();
			SDL_DestroyMutex(mixer_lock);
		}
		--audio_opened;
	}
}

/* Pause a particular channel (or all) */
void Mix_Pause(int which)
{
	if ( which == -1 ) {
		int i;

		for ( i=0; i<MIX_CHANNELS; ++i ) {
			if ( channel[i].playing > 0 ) {
				channel[i].paused = 1;
			}
		}
	} else {
		if ( channel[which].playing > 0 ) {
			channel[which].paused = 1;
		}
	}
}

/* Resume a paused channel */
void Mix_Resume(int which)
{
	if ( which == -1 ) {
		int i;

		for ( i=0; i<MIX_CHANNELS; ++i ) {
			if ( channel[i].playing > 0 ) {
				channel[i].paused = 0;
			}
		}
	} else {
		if ( channel[which].playing > 0 ) {
			channel[which].paused = 0;
		}
	}
}
