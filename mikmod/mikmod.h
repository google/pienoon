#ifndef _MIKMOD_H
#define _MIKMOD_H

#include "mmio.h"

#ifdef __cplusplus
extern "C" {
#endif


#define MUTE_EXCLUSIVE  32000
#define MUTE_INCLUSIVE  32001

#define PAN_LEFT           0
#define PAN_CENTER       128
#define PAN_RIGHT        255
#define PAN_SURROUND     512         /* panning value for Dolby Surround */


#define MikMod_RegisterDriver(x) MD_RegisterDriver(&x)
#define MikMod_RegisterLoader(x) ML_RegisterLoader(&x)
#define MikMod_RegisterErrorHandler(x) _mm_RegisterErrorHandler(x)


/* The following #define macros are for retaining API compatiability */
/* with the beta version of MikMod 3.0, and are TEMPORARY!  They WILL */
/* be removed in the future! */

#define MD_RegisterPlayer(x)      MikMod_RegisterPlayer(x)
#define MD_Init                   MikMod_Init
#define MD_Exit                   MikMod_Exit
#define MD_Update                 MikMod_Update
#define ML_Free(x)                MikMod_FreeSong(x)
#define MD_SetNumChannels(x,y)    MikMod_SetNumVoices(x,y)
#define MD_SetNumVoices(x,y)      MikMod_SetNumVoices(x,y)
#define MD_PlayStart              MikMod_EnableOutput
#define MD_PlayStop               MikMod_DisableOutput

#define MD_VoiceSetVolume(x,y)    Voice_SetVolume(x,y)
#define MD_VoiceSetFrequency(x,y) Voice_SetFrequency(x,y)
#define MD_VoiceSetPanning(x,y)   Voice_SetPanning(x,y)
#define MD_VoicePlay(x,y,z)       Voice_Play(x,y,z)
#define MD_VoiceStop(x)           Voice_Stop(x)
#define MD_VoiceReleaseSustain(x) Voice_ReleaseSustain(x)
#define MD_VoiceStopped(x)        Voice_Stopped(x)
#define MD_VoiceGetPosition(x)    Voice_GetPosition(x)
#define MD_VoiceRealVolume(x)     Voice_RealVolume(x)


#define SFX_CRITICAL  1


/**************************************************************************
****** mikmod types: ******************************************************
**************************************************************************/

/* Sample format [loading and in-memory] flags: */
#define SF_16BITS       1
#define SF_SIGNED       2
#define SF_STEREO       4
#define SF_DELTA        8
#define SF_BIG_ENDIAN   16

/* General Playback flags */

#define SF_LOOP         32
#define SF_BIDI         64
#define SF_SUSTAIN      128
#define SF_REVERSE      256

/* Module-only Playback Flags */

#define SF_OWNPAN       512
#define SF_UST_LOOP     1024


typedef struct SAMPLE
{   ULONG  speed;            /* Base playing speed/frequency of note (Middle C in player) */
    UBYTE  volume;           /* volume 0-64 */
    UWORD  panning;          /* panning (0-255 or PAN_SURROUND) */
    ULONG  length;           /* length of sample (in samples!) */
    ULONG  loopstart;        /* repeat position (relative to start, in samples) */
    ULONG  loopend;          /* repeat end */
    ULONG  susbegin;         /* sustain loop begin (in samples) \  Not Supported */
    ULONG  susend;           /* sustain loop end                /      Yet! */

    UWORD  flags;            /* sample format in memory */

/* Variables used by the module player only! (ignored for sound effects) */

    UBYTE  globvol;          /* global volume */
    UBYTE  vibflags;         /* autovibrato flag stuffs */
    UBYTE  vibtype;          /* Vibratos moved from INSTRUMENT to SAMPLE */
    UBYTE  vibsweep;
    UBYTE  vibdepth;
    UBYTE  vibrate;

    CHAR   *samplename;      /* name of the sample */

/* Values used internally only (not saved in disk formats) */

    UWORD  avibpos;          /* autovibrato pos [player use] */
    UBYTE  divfactor;        /* for sample scaling (maintains proper period slides) */
    ULONG  seekpos;          /* seek position in file */
    SWORD  handle;           /* sample handle used by individual drivers */
} SAMPLE;



/* --> Struct : SAMPLOAD */
/* This is a handle of sorts attached to any sample registered with */
/* SL_RegisterSample.  Generally, this only need be used or changed by the */
/* loaders and drivers of mikmod. */

typedef struct SAMPLOAD
{   struct SAMPLOAD *next;

    ULONG  length;           /* length of sample (in samples!) */
    ULONG  loopstart;        /* repeat position (relative to start, in samples) */
    ULONG  loopend;          /* repeat end */

    UWORD   infmt, outfmt;
    int     scalefactor;
    SAMPLE  *sample;
    FILE    *fp;
} SAMPLOAD;

extern void SL_HalveSample(SAMPLOAD *s);
extern void SL_Sample8to16(SAMPLOAD *s);
extern void SL_Sample16to8(SAMPLOAD *s);
extern void SL_SampleSigned(SAMPLOAD *s);
extern void SL_SampleUnsigned(SAMPLOAD *s);
extern BOOL SL_LoadSamples(void);                      /* Returns 1 on error! */
extern SAMPLOAD *SL_RegisterSample(SAMPLE *s, int type, FILE *fp);    /* Returns 1 on error! */
extern void SL_Load(void *buffer, SAMPLOAD *smp, int length);
extern BOOL SL_Init(SAMPLOAD *s);
extern void SL_Exit(SAMPLOAD *s);


/**************************************************************************
****** Wavload stuff: *****************************************************
**************************************************************************/

SAMPLE *WAV_LoadFP(FILE *fp);
SAMPLE *WAV_LoadFN(CHAR *filename);
void WAV_Free(SAMPLE *si);


#include "ptform.h"


/**************************************************************************
****** Driver stuff: ******************************************************
**************************************************************************/

/* max. number of handles a driver has to provide. (not strict) */

#define MAXSAMPLEHANDLES 384


enum
{   MD_MUSIC = 0,
    MD_SNDFX
};

enum
{   MD_HARDWARE = 0,
    MD_SOFTWARE
};


/* possible mixing mode bits: */
/* -------------------------- */
/* These take effect only after MikMod_Init or MikMod_Reset. */

#define DMODE_16BITS       1         /* enable 16 bit output */
#define DMODE_SURROUND     2         /* enable Dolby surround sound (not yet supported) */
#define DMODE_SOFT_SNDFX   4         /* Process sound effects via software mixer (not yet supported) */
#define DMODE_SOFT_MUSIC   8         /* Process music via software mixer (not yet supported) */

/* These take effect immidiately. */

#define DMODE_STEREO      16         /* enable stereo output */
#define DMODE_REVERSE     32         /* reverse stereo */
#define DMODE_INTERP      64         /* enable interpolation (not yet supported) */


/* driver structure: */

typedef struct MDRIVER
{   struct MDRIVER *next;
    CHAR    *Name;
    CHAR    *Version;
    UBYTE   HardVoiceLimit,       /* Limit of hardware mixer voices for this driver */
            SoftVoiceLimit;       /* Limit of software mixer voices for this driver */

    BOOL    (*IsPresent)          (void);
    SWORD   (*SampleLoad)         (SAMPLOAD *s, int type);
    void    (*SampleUnLoad)       (SWORD handle);
    ULONG   (*FreeSampleSpace)    (int type);
    ULONG   (*RealSampleLength)   (int type, SAMPLE *s);
    BOOL    (*Init)               (void);
    void    (*Exit)               (void);
    BOOL    (*Reset)              (void);
    BOOL    (*SetNumVoices)       (void);
    BOOL    (*PlayStart)          (void);
    void    (*PlayStop)           (void);
    void    (*Update)             (void);
    void    (*VoiceSetVolume)     (UBYTE voice, UWORD vol);
    void    (*VoiceSetFrequency)  (UBYTE voice, ULONG frq);
    void    (*VoiceSetPanning)    (UBYTE voice, ULONG pan);
    void    (*VoicePlay)          (UBYTE voice, SWORD handle, ULONG start, ULONG size, ULONG reppos, ULONG repend, UWORD flags);
    void    (*VoiceStop)          (UBYTE voice);
    BOOL    (*VoiceStopped)       (UBYTE voice);
    void    (*VoiceReleaseSustain)(UBYTE voice);
    SLONG   (*VoiceGetPosition)   (UBYTE voice);
    ULONG   (*VoiceRealVolume)    (UBYTE voice);

    BOOL    (*StreamInit)         (ULONG speed, UWORD flags);
    void    (*StreamExit)         (void);
    void    (*StreamSetSpeed)     (ULONG speed);
    SLONG   (*StreamGetPosition)  (void);
    void    (*StreamLoadFP)       (FILE *fp);
} MDRIVER;


/* These variables can be changed at ANY time and results */
/* will be immidiate: */

extern UBYTE md_bpm;            /* current song / hardware BPM rate */
extern UBYTE md_volume;         /* Global sound volume (0-128) */
extern UBYTE md_musicvolume;    /* volume of song */
extern UBYTE md_sndfxvolume;    /* volume of sound effects */
extern UBYTE md_reverb;         /* 0 = none;  15 = chaos */
extern UBYTE md_pansep;         /* 0 = mono;  128 == 100% (full left/right) */


/* The variables below can be changed at any time, but changes will */
/* not be implimented until MikMod_Reset is called.  A call to */
/* MikMod_Reset may result in a skip or pop in audio (depending on */
/* the soundcard driver and the settings changed). */

extern UWORD md_device;         /* Device.  0 = autodetect, other # depend on driver register order. */
extern UWORD md_mixfreq;        /* mixing frequency.  Valid 5000 -> 44100 */
extern UWORD md_dmabufsize;     /* DMA buffer size.  Valid 512 -> 32000 */
extern UWORD md_mode;           /* Mode.  See DMODE_? flags above */


/* Variables below can be changed via MD_SetNumVoices at any time. */
/*  However, a call to MD_SetNumVoicess while the driver */
/* is active will cause the sound to skip slightly. */

extern UBYTE md_numchn,         /* number of song + sound effects voices */
             md_sngchn,         /* number of song voices */
             md_sfxchn,         /* number of sound effects voices */
             md_hardchn,        /* number of hardware mixed voices */
             md_softchn;        /* number of software mixed voices */


/* Following variables should not be changed! */
extern MDRIVER *md_driver;      /* Current driver in use.  See MDRIVER struct */
                                /* above for structure info contents. */

/* This is for use by the hardware drivers only.  It points to the */
/* registered tickhandler function. */
extern void (*md_player)(void);


/* main driver prototypes: */

extern void   MikMod_RegisterAllDrivers(void);
extern void   MikMod_RegisterAllLoaders(void);

extern BOOL   MikMod_Init(void);
extern void   MikMod_Exit(void);
extern BOOL   MikMod_Reset(void);
extern int    MikMod_PlaySample(SAMPLE *s, ULONG start, UBYTE flags);
extern BOOL   MikMod_SetNumVoices(int music, int sndfx);
extern BOOL   MikMod_Active(void);
extern BOOL   MikMod_EnableOutput(void);
extern void   MikMod_DisableOutput(void);
extern void   MikMod_RegisterPlayer(void (*plr)(void));
extern void   MikMod_Update(void);

extern void   Voice_SetVolume(int voice, UWORD ivol);
extern void   Voice_SetFrequency(int voice, ULONG frq);
extern void   Voice_SetPanning(int voice, ULONG pan);
extern void   Voice_Play(int voice,SAMPLE *s, ULONG start);
extern void   Voice_Stop(int voice);
extern void   Voice_ReleaseSustain(int voice);
extern BOOL   Voice_Stopped(int voice);
extern SLONG  Voice_GetPosition(int voice);
extern ULONG  Voice_RealVolume(int voice);

extern void   MD_InfoDriver(void);
extern void   MD_RegisterDriver(MDRIVER *drv);
extern SWORD  MD_SampleLoad(SAMPLOAD *s, int type, FILE *fp);
extern void   MD_SampleUnLoad(SWORD handle);
extern void   MD_SetBPM(UBYTE bpm);
extern ULONG  MD_SampleSpace(int type);
extern ULONG  MD_SampleLength(int type, SAMPLE *s);

/* Declare external drivers: */

extern MDRIVER drv_sdl;      /* Simple DirectMedia Layer driver */
extern MDRIVER drv_nos;      /* nosound driver */


/**************************************************************************
****** Streaming Audio stuff: *********************************************
**************************************************************************/


typedef struct MSTREAM
{   struct MSTREAM *next;
    CHAR    *type;
    CHAR    *version;
    BOOL    (*Init)(void);
    BOOL    (*Test)(void);
    BOOL    (*Load)(void);
    void    (*Cleanup)(void);
} MSTREAM;


extern int   stream_bufsize;
extern FILE  *stream_fp;
extern SLONG stream_seekpos;
extern SLONG stream_reppos;


/**************************************************************************
****** Virtual channel stuff: *********************************************
**************************************************************************/

extern BOOL    VC_Init(void);
extern void    VC_Exit(void);
extern BOOL    VC_SetNumVoices(void);
extern ULONG   VC_SampleSpace(int type);
extern ULONG   VC_SampleLength(int type, SAMPLE *s);

extern BOOL    VC_PlayStart(void);
extern void    VC_PlayStop(void);

/* extern SWORD   VC_SampleLoad(SAMPLOAD *sload, int type, FILE *fp); */
extern SWORD   VC_SampleLoad(SAMPLOAD *sload, int type);
extern void    VC_SampleUnload(SWORD handle);

extern void    VC_WriteSamples(SBYTE *buf,ULONG todo);
extern ULONG   VC_WriteBytes(SBYTE *buf,ULONG todo);
extern void    VC_SilenceBytes(SBYTE *buf,ULONG todo);

extern void    VC_VoiceSetVolume(UBYTE voice, UWORD vol);
extern void    VC_VoiceSetFrequency(UBYTE voice, ULONG frq);
extern void    VC_VoiceSetPanning(UBYTE voice, ULONG pan);
extern void    VC_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags);

extern void    VC_VoiceStop(UBYTE voice);
extern BOOL    VC_VoiceStopped(UBYTE voice);
extern void    VC_VoiceReleaseSustain(UBYTE voice);
extern SLONG   VC_VoiceGetPosition(UBYTE voice);
extern ULONG   VC_VoiceRealVolume(UBYTE voice);


extern BOOL    VC2_Init(void);
extern void    VC2_Exit(void);
extern BOOL    VC2_SetNumVoices(void);
extern ULONG   VC2_SampleSpace(int type);
extern ULONG   VC2_SampleLength(int type, SAMPLE *s);

extern BOOL    VC2_PlayStart(void);
extern void    VC2_PlayStop(void);

extern SWORD   VC2_SampleLoad(SAMPLOAD *sload, int type, FILE *fp);
extern void    VC2_SampleUnload(SWORD handle);

extern void    VC2_WriteSamples(SBYTE *buf,ULONG todo);
extern ULONG   VC2_WriteBytes(SBYTE *buf,ULONG todo);
extern void    VC2_SilenceBytes(SBYTE *buf,ULONG todo);

extern void    VC2_VoiceSetVolume(UBYTE voice, UWORD vol);
extern void    VC2_VoiceSetFrequency(UBYTE voice, ULONG frq);
extern void    VC2_VoiceSetPanning(UBYTE voice, ULONG pan);
extern void    VC2_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags);

extern void    VC2_VoiceStop(UBYTE voice);
extern BOOL    VC2_VoiceStopped(UBYTE voice);
extern void    VC2_VoiceReleaseSustain(UBYTE voice);
extern SLONG   VC2_VoiceGetPosition(UBYTE voice);

#ifdef __cplusplus
}
#endif

#endif
