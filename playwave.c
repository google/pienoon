/*
    PLAYWAVE:  A test application for the SDL mixer library.
    Copyright (C) 1997-1999  Sam Lantinga

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    5635-34 Springhouse Dr.
    Pleasanton, CA 94588 (USA)
    slouken@devolution.com
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#ifdef unix
#include <unistd.h>
#endif

#include "SDL.h"
#include "SDL_mixer.h"


static int audio_open = 0;
static Mix_Chunk *wave = NULL;

void CleanUp(void)
{
	if ( wave ) {
		Mix_FreeChunk(wave);
		wave = NULL;
	}
	if ( audio_open ) {
		Mix_CloseAudio();
		audio_open = 0;
	}
	SDL_Quit();
}

void Usage(char *argv0)
{
	fprintf(stderr, "Usage: %s [-8] [-r rate] [-l] [-m] <wavefile>\n", argv0);
}


/*#define TEST_MIX_CHANNELFINISHED*/
#ifdef TEST_MIX_CHANNELFINISHED  /* rcg06072001 */
static volatile int channel_is_done = 0;
static void channel_complete_callback(int chan)
{
	Mix_Chunk *done_chunk = Mix_GetChunk(chan);
	printf("We were just alerted that Mixer channel #%d is done.\n", chan);
	printf("Channel's chunk pointer is (%p).\n", done_chunk);
	printf(" Which %s correct.\n", (wave == done_chunk) ? "is" : "is NOT");
	channel_is_done = 1;
}
#endif


main(int argc, char *argv[])
{
	int audio_rate;
	Uint16 audio_format;
	int audio_channels;
	int loops = 0;
	int i;

	/* Initialize variables */
	audio_rate = MIX_DEFAULT_FREQUENCY;
	audio_format = MIX_DEFAULT_FORMAT;
	audio_channels = 2;

	/* Check command line usage */
	for ( i=1; argv[i] && (*argv[i] == '-'); ++i ) {
		if ( (strcmp(argv[i], "-r") == 0) && argv[i+1] ) {
			++i;
			audio_rate = atoi(argv[i]);
		} else
		if ( strcmp(argv[i], "-m") == 0 ) {
			audio_channels = 1;
		} else
		if ( strcmp(argv[i], "-l") == 0 ) {
			loops = -1;
		} else
		if ( strcmp(argv[i], "-8") == 0 ) {
			audio_format = AUDIO_U8;
		} else {
			Usage(argv[0]);
			return(1);
		}
	}
	if ( ! argv[i] ) {
		Usage(argv[0]);
		return(1);
	}

	/* Initialize the SDL library */
	if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		return(255);
	}
	atexit(CleanUp);
	signal(SIGINT, exit);
	signal(SIGTERM, exit);

	/* Open the audio device */
	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, 4096) < 0) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return(2);
	} else {
		Mix_QuerySpec(&audio_rate, &audio_format, &audio_channels);
		printf("Opened audio at %d Hz %d bit %s", audio_rate,
			(audio_format&0xFF),
			(audio_channels > 1) ? "stereo" : "mono");
		if ( loops ) {
		  printf(" (looping)\n");
		} else {
		  putchar('\n');
		}
	}
	audio_open = 1;

	/* Load the requested wave file */
	wave = Mix_LoadWAV(argv[i]);
	if ( wave == NULL ) {
		fprintf(stderr, "Couldn't load %s: %s\n",
						argv[i], SDL_GetError());
		return(2);
	}

#ifdef TEST_MIX_CHANNELFINISHED  /* rcg06072001 */
	setbuf(stdout, NULL);
	Mix_ChannelFinished(channel_complete_callback);
#endif

	/* Play and then exit */
	Mix_PlayChannel(0, wave, loops);

#ifdef TEST_MIX_CHANNELFINISHED  /* rcg06072001 */
	while (!channel_is_done) {
		SDL_Delay(100);
	}
#else
	while ( Mix_Playing(0) ) {
		SDL_Delay(100);
	}
#endif

	return(0);
}
