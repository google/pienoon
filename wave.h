/*
    Taken with permission from SDL_wave.h, part of the SDL library,
    available at: http://www.devolution.com/~slouken/SDL 
    and placed under the same license as this mixer library.
*/

/* WAVE files are little-endian */

/*******************************************/
/* Define values for Microsoft WAVE format */
/*******************************************/
#define RIFF		0x46464952		/* "RIFF" */
#define WAVE		0x45564157		/* "WAVE" */
#define FACT		0x74636166		/* "fact" */
#define LIST		0x5453494c		/* "LIST" */
#define FMT		0x20746D66		/* "fmt " */
#define DATA		0x61746164		/* "data" */
#define PCM_CODE	1
#define ADPCM_CODE	2
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* Normally, these three chunks come consecutively in a WAVE file */
typedef struct WaveFMT {
/* Not saved in the chunk we read:
	Uint32	FMTchunk;
	Uint32	fmtlen;
*/
	Uint16	encoding;	
	Uint16	channels;		/* 1 = mono, 2 = stereo */
	Uint32	frequency;		/* One of 11025, 22050, or 44100 Hz */
	Uint32	byterate;		/* Average bytes per second */
	Uint16	blockalign;		/* Bytes per sample block */
	Uint16	bitspersample;		/* One of 8, 12, 16, or 4 for ADPCM */
} WaveFMT;

/* The general chunk found in the WAVE file */
typedef struct Chunk {
	Uint32 magic;
	Uint32 length;
	Uint8 *data;			/* Data includes magic and length */
} Chunk;

/*********************************************/
/* Define values for AIFF (IFF audio) format */
/*********************************************/
#define FORM		0x4d524f46		/* "FORM" */
#define AIFF		0x46464941		/* "AIFF" */
#define SSND		0x444e5353		/* "SSND" */
#define COMM		0x4d4d4f43		/* "COMM" */

