/*

Name:
DRV_SDL.C

Description:
Mikmod driver for output using the Simple DirectMedia Layer

*/

#include "mikmod.h"


static BOOL SDRV_IsThere(void)
{
    return 1;
}


static BOOL SDRV_Init(void)
{
    md_mode |= DMODE_SOFT_MUSIC | DMODE_SOFT_SNDFX;
    return(VC_Init());
}


static void SDRV_Exit(void)
{
    VC_Exit();
}


static void SDRV_Update(void)
{
    /* does nothing, buffers are updated in the background */
}


static BOOL SDRV_Reset(void)
{
    return 0;
}


MDRIVER drv_sdl =
{   NULL,
    "SDL",
    "MikMod Simple DirectMedia Layer driver v1.0",
    0,255,
    SDRV_IsThere,
    VC_SampleLoad,
    VC_SampleUnload,
    VC_SampleSpace,
    VC_SampleLength,
    SDRV_Init,
    SDRV_Exit,
    SDRV_Reset,
    VC_SetNumVoices,
    VC_PlayStart,
    VC_PlayStop,
    SDRV_Update,
    VC_VoiceSetVolume,
    VC_VoiceSetFrequency,
    VC_VoiceSetPanning,
    VC_VoicePlay,
    VC_VoiceStop,
    VC_VoiceStopped,
    VC_VoiceReleaseSustain,
    VC_VoiceGetPosition,
    VC_VoiceRealVolume
};

