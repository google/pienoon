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

/* This file supports streaming WAV files, without volume adjustment */

#include <stdlib.h>
#include <string.h>

#include "SDL_audio.h"
#include "SDL_mutex.h"
#include "SDL_rwops.h"
#include "SDL_endian.h"

#include "wave.h"
#include "wavestream.h"

/* Currently we only support a single stream at a time */
static WAVStream *theWave = NULL;

/* This is initialized by the music mixer */
static SDL_mutex *music_lock = NULL;

/* This is the format of the audio mixer data */
static SDL_AudioSpec mixer;

/* Function to load the WAV/AIFF stream */
static FILE *LoadWAVStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop);
static FILE *LoadAIFFStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop);

/* Initialize the WAVStream player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
int WAVStream_Init(SDL_AudioSpec *mixerfmt)
{
	/* FIXME: clean up the mutex, or move it into music.c */
	music_lock = SDL_CreateMutex();
	if ( music_lock == NULL ) {
		return(-1);
	}
	mixer = *mixerfmt;
	return(0);
}

/* Unimplemented */
extern void WAVStream_SetVolume(int volume)
{
}

/* Load a WAV stream from the given file */
extern WAVStream *WAVStream_LoadSong(const char *file, const char *magic)
{
	WAVStream *wave;
	SDL_AudioSpec wavespec;

	if ( ! mixer.format ) {
		SDL_SetError("WAV music output not started");
		return(NULL);
	}
	wave = (WAVStream *)malloc(sizeof *wave);
	if ( wave ) {
		memset(wave, 0, (sizeof *wave));
		if ( strcmp(magic, "RIFF") == 0 ) {
			wave->wavefp = LoadWAVStream(file, &wavespec,
					&wave->start, &wave->stop);
		} else
		if ( strcmp(magic, "FORM") == 0 ) {
			wave->wavefp = LoadAIFFStream(file, &wavespec,
					&wave->start, &wave->stop);
		}
		if ( wave->wavefp == NULL ) {
			free(wave);
			return(NULL);
		}
		SDL_BuildAudioCVT(&wave->cvt,
			wavespec.format, wavespec.channels, wavespec.freq,
			mixer.format, mixer.channels, mixer.freq);
	}
	return(wave);
}

/* Start playback of a given WAV stream */
extern void WAVStream_Start(WAVStream *wave)
{
	SDL_mutexP(music_lock);
	clearerr(wave->wavefp);
	fseek(wave->wavefp, wave->start, SEEK_SET);
	theWave = wave;
	SDL_mutexV(music_lock);
}

/* Play some of a stream previously started with WAVStream_Start()
   The music_lock is held while this function is called.
 */
extern void WAVStream_PlaySome(Uint8 *stream, int len)
{
	long pos;

	SDL_mutexP(music_lock);
	if ( theWave && ((pos=ftell(theWave->wavefp)) < theWave->stop) ) {
		if ( theWave->cvt.needed ) {
			int original_len;

			original_len=(int)((double)len/theWave->cvt.len_ratio);
			if ( theWave->cvt.len != original_len ) {
				int worksize;
				if ( theWave->cvt.buf != NULL ) {
					free(theWave->cvt.buf);
				}
				worksize = original_len*theWave->cvt.len_mult;
				theWave->cvt.buf=(Uint8 *)malloc(worksize);
				if ( theWave->cvt.buf == NULL ) {
					SDL_mutexV(music_lock);
					return;
				}
				theWave->cvt.len = original_len;
			}
			if ( (theWave->stop - pos) < original_len ) {
				original_len = (theWave->stop - pos);
			}
			theWave->cvt.len = original_len;
			fread(theWave->cvt.buf,original_len,1,theWave->wavefp);
			SDL_ConvertAudio(&theWave->cvt);
			memcpy(stream, theWave->cvt.buf, theWave->cvt.len_cvt);
		} else {
			if ( (theWave->stop - pos) < len ) {
				len = (theWave->stop - pos);
			}
			fread(stream, len, 1, theWave->wavefp);
		}
	}
	SDL_mutexV(music_lock);
}

/* Stop playback of a stream previously started with WAVStream_Start() */
extern void WAVStream_Stop(void)
{
	SDL_mutexP(music_lock);
	theWave = NULL;
	SDL_mutexV(music_lock);
}

/* Close the given WAV stream */
extern void WAVStream_FreeSong(WAVStream *wave)
{
	if ( wave ) {
		/* Remove song from the currently playing list */
		SDL_mutexP(music_lock);
		if ( wave == theWave ) {
			theWave = NULL;
		}
		SDL_mutexV(music_lock);

		/* Clean up associated data */
		if ( wave->wavefp ) {
			fclose(wave->wavefp);
		}
		if ( wave->cvt.buf ) {
			free(wave->cvt.buf);
		}
		free(wave);
	}
}

/* Return non-zero if a stream is currently playing */
extern int WAVStream_Active(void)
{
	int active;

	SDL_mutexP(music_lock);
	active = 0;
	if ( theWave && (ftell(theWave->wavefp) < theWave->stop) ) {
		active = 1;
	}
	SDL_mutexV(music_lock);

	return(active);
}

static int ReadChunk(SDL_RWops *src, Chunk *chunk, int read_data)
{
	chunk->magic	= SDL_ReadLE32(src);
	chunk->length	= SDL_ReadLE32(src);
	if ( read_data ) {
		chunk->data = (Uint8 *)malloc(chunk->length);
		if ( chunk->data == NULL ) {
			SDL_SetError("Out of memory");
			return(-1);
		}
		if ( SDL_RWread(src, chunk->data, chunk->length, 1) != 1 ) {
			SDL_SetError("Couldn't read chunk");
			free(chunk->data);
			return(-1);
		}
	} else {
		SDL_RWseek(src, chunk->length, SEEK_CUR);
	}
	return(chunk->length);
}

static FILE *LoadWAVStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop)
{
	int was_error;
	FILE *wavefp;
	SDL_RWops *src;
	Chunk chunk;
	int lenread;

	/* WAV magic header */
	Uint32 RIFFchunk;
	Uint32 wavelen;
	Uint32 WAVEmagic;

	/* FMT chunk */
	WaveFMT *format = NULL;

	/* Make sure we are passed a valid data source */
	was_error = 0;
	wavefp = fopen(file, "rb");
	src = NULL;
	if ( wavefp ) {
		src = SDL_RWFromFP(wavefp, 0);
	}
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Check the magic header */
	RIFFchunk	= SDL_ReadLE32(src);
	wavelen		= SDL_ReadLE32(src);
	WAVEmagic	= SDL_ReadLE32(src);
	if ( (RIFFchunk != RIFF) || (WAVEmagic != WAVE) ) {
		SDL_SetError("Unrecognized file type (not WAVE)");
		was_error = 1;
		goto done;
	}

	/* Read the audio data format chunk */
	chunk.data = NULL;
	do {
		/* FIXME! Add this logic to SDL_LoadWAV_RW() */
		if ( chunk.data ) {
			free(chunk.data);
		}
		lenread = ReadChunk(src, &chunk, 1);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
	} while ( (chunk.magic == FACT) || (chunk.magic == LIST) );

	/* Decode the audio data format */
	format = (WaveFMT *)chunk.data;
	if ( chunk.magic != FMT ) {
		free(chunk.data);
		SDL_SetError("Complex WAVE files not supported");
		was_error = 1;
		goto done;
	}
	switch (SDL_SwapLE16(format->encoding)) {
		case PCM_CODE:
			/* We can understand this */
			break;
		default:
			SDL_SetError("Unknown WAVE data format");
			was_error = 1;
			goto done;
	}
	memset(spec, 0, (sizeof *spec));
	spec->freq = SDL_SwapLE32(format->frequency);
	switch (SDL_SwapLE16(format->bitspersample)) {
		case 8:
			spec->format = AUDIO_U8;
			break;
		case 16:
			spec->format = AUDIO_S16;
			break;
		default:
			SDL_SetError("Unknown PCM data format");
			was_error = 1;
			goto done;
	}
	spec->channels = (Uint8) SDL_SwapLE16(format->channels);
	spec->samples = 4096;		/* Good default buffer size */

	/* Set the file offset to the DATA chunk data */
	chunk.data = NULL;
	do {
		*start = SDL_RWtell(src) + 2*sizeof(Uint32);
		lenread = ReadChunk(src, &chunk, 0);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
	} while ( chunk.magic != DATA );
	*stop = SDL_RWtell(src);

done:
	if ( format != NULL ) {
		free(format);
	}
	if ( src ) {
		SDL_RWclose(src);
	}
	if ( was_error ) {
		if ( wavefp ) {
			fclose(wavefp);
			wavefp = NULL;
		}
	}
	return(wavefp);
}

static double SANE_to_double(Uint32 l1, Uint32 l2, Uint16 s1)
{
	double d;
	struct almost_double {
		Uint32 hi, lo;
	} *dp = (struct almost_double *)&d;

	dp->hi = ((l1 << 4) & 0x3ff00000) | (l1 & 0xc0000000);
	dp->hi |= (l1 << 5) & 0xffff0;
	dp->hi |= (l2 >> 27) & 0x1f;
	dp->lo = (l2 << 5) & 0xffffffe0;
	dp->lo |= ((s1 >> 11) & 0x1f);
	return(d);
}

static FILE *LoadAIFFStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop)
{
	int was_error;
	FILE *wavefp;
	SDL_RWops *src;

	/* AIFF magic header */
	Uint32 FORMchunk;
	Uint32 chunklen;
	Uint32 AIFFmagic;
	/* SSND chunk        */
	Uint32 SSNDchunk;
	Uint32 ssndlen;
	Uint32 offset;
	Uint32 blocksize;
	/* COMM format chunk */
	Uint32 COMMchunk;
	Uint32 commlen;
	Uint16 channels;
	Uint32 numsamples;
	Uint16 samplesize;
	struct { /* plus a SANE format double precision number */
		Uint32 l1;
		Uint32 l2;
		Uint16 s1;
	} sane_freq;

	Uint32 frequency;


	/* Make sure we are passed a valid data source */
	was_error = 0;
	wavefp = fopen(file, "rb");
	src = NULL;
	if ( wavefp ) {
		src = SDL_RWFromFP(wavefp, 0);
	}
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Check the magic header */
	FORMchunk	= SDL_ReadLE32(src);
	chunklen	= SDL_ReadLE32(src);
	AIFFmagic	= SDL_ReadLE32(src);
	if ( (FORMchunk != FORM) || (AIFFmagic != AIFF) ) {
		SDL_SetError("Unrecognized file type (not AIFF)");
		was_error = 1;
		goto done;
	}

	/* Read the SSND data chunk */
	SSNDchunk	= SDL_ReadLE32(src);
	if ( SSNDchunk != SSND ) {
		SDL_SetError("Unrecognized AIFF chunk (not SSND)");
		was_error = 1;
		goto done;
	}
	ssndlen		= SDL_ReadLE32(src);
	offset		= SDL_ReadLE32(src);
	blocksize	= SDL_ReadLE32(src);

	/* Fill in start and stop pointers, then seek to format chunk */
	ssndlen -= (2*sizeof(Uint32));
	*start = SDL_RWtell(src) + offset;
	*stop = SDL_RWtell(src) + ssndlen;
	SDL_RWseek(src, *stop, SEEK_SET);

	/* Read the audio data format chunk */
	COMMchunk	= SDL_ReadLE32(src);
	if ( COMMchunk != COMM ) {
		SDL_SetError("Unrecognized AIFF chunk (not COMM)");
		was_error = 1;
		goto done;
	}
	commlen		= SDL_ReadLE32(src);
	channels	= SDL_ReadLE16(src);
	numsamples	= SDL_ReadLE32(src);
	samplesize	= SDL_ReadLE16(src);
	sane_freq.l1	= SDL_ReadLE32(src);
	sane_freq.l2	= SDL_ReadLE32(src);
	sane_freq.s1	= SDL_ReadLE16(src);
	frequency	= (Uint32)SANE_to_double(sane_freq.l1, sane_freq.l2,
								sane_freq.s1);

	/* Decode the audio data format */
	memset(spec, 0, (sizeof *spec));
	spec->freq = frequency;
	switch (samplesize) {
		case 8:
			spec->format = AUDIO_U8;
			break;
		case 16:
			spec->format = AUDIO_S16;
			break;
		default:
			SDL_SetError("Unknown samplesize in data format");
			was_error = 1;
			goto done;
	}
	spec->channels = (Uint8) channels;
	spec->samples = 4096;		/* Good default buffer size */

done:
	if ( src ) {
		SDL_RWclose(src);
	}
	if ( was_error ) {
		if ( wavefp ) {
			fclose(wavefp);
			wavefp = NULL;
		}
	}
	return(wavefp);
}
