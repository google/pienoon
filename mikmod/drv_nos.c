/*

Name:
DRV_NOS.C

Description:
Mikmod driver for no output on any soundcard, monitor, keyboard, or whatever :)

Portability:
All systems - All compilers

*/

#include "mikmod.h"


static BOOL NS_IsThere(void)
{
    return 1;
}


static SWORD NS_SampleLoad(SAMPLOAD *s, int type)
{
    return 0;
}


static void NS_SampleUnload(SWORD h)
{
}


static ULONG NS_SampleSpace(int type)
{
    return 0;
}


static ULONG NS_SampleLength(int type, SAMPLE *s)
{
    return s->length;
}


static BOOL NS_Init(void)
{
    return 0;
}


static void NS_Exit(void)
{
}


static BOOL NS_Reset(void)
{
    return 0;
}


static BOOL NS_PlayStart(void)
{
    return 0;
}


static void NS_PlayStop(void)
{
}


static void NS_Update(void)
{
}


static BOOL NS_SetNumVoices(void)
{
    return 0;
}


static void NS_VoiceSetVolume(UBYTE voice,UWORD vol)
{
}


static void NS_VoiceSetFrequency(UBYTE voice,ULONG frq)
{
}


static void NS_VoiceSetPanning(UBYTE voice,ULONG pan)
{
}


static void NS_VoicePlay(UBYTE voice,SWORD handle,ULONG start,ULONG size,ULONG reppos,ULONG repend,UWORD flags)
{
}


static void NS_VoiceStop(UBYTE voice)
{
}


static BOOL NS_VoiceStopped(UBYTE voice)
{
   return 0;
}


static void NS_VoiceReleaseSustain(UBYTE voice)
{
}


static SLONG NS_VoiceGetPosition(UBYTE voice)
{
   return 0;
}


static ULONG NS_VoiceRealVolume(UBYTE voice)
{
   return 0;
}



MDRIVER drv_nos =
{   NULL,
    "No Sound",
    "Nosound Driver v2.0 - (c) Creative Silence",
    255,255,
    NS_IsThere,
    NS_SampleLoad,
    NS_SampleUnload,
    NS_SampleSpace,
    NS_SampleLength,
    NS_Init,
    NS_Exit,
    NS_Reset,
    NS_SetNumVoices,
    NS_PlayStart,
    NS_PlayStop,
    NS_Update,
    NS_VoiceSetVolume,
    NS_VoiceSetFrequency,
    NS_VoiceSetPanning,
    NS_VoicePlay,
    NS_VoiceStop,
    NS_VoiceStopped,
    NS_VoiceReleaseSustain,
    NS_VoiceGetPosition,
    NS_VoiceRealVolume
};

