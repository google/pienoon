/*

Name:  MDRIVER.C

Description:
These routines are used to access the available soundcard drivers.

Portability:
All systems - all compilers

*/

#include "mikmod.h"

MDRIVER *firstdriver = NULL, *md_driver = &drv_nos;
extern UNIMOD     *pf;           /* <- this modfile is being played */

UWORD md_device         = 0;
UWORD md_mixfreq        = 44100;
UWORD md_mode           = DMODE_STEREO | DMODE_16BITS | DMODE_SURROUND;
UWORD md_dmabufsize     = 50;
UBYTE md_pansep         = 128;      /* 128 == 100% (full left/right) */

UBYTE md_reverb         = 6;       /* Reverb */

UBYTE md_volume         = 96;       /* Global sound volume (0-128) */
UBYTE md_musicvolume    = 128;      /* volume of song */
UBYTE md_sndfxvolume    = 128;      /* volume of sound effects */

UBYTE md_bpm            = 125;


/* Do not modify the numchn variables yourself!  use MD_SetVoices() */

UBYTE md_numchn = 0,  md_sngchn = 0,  md_sfxchn = 0;
UBYTE md_hardchn = 0, md_softchn = 0;


void (*md_player)(void) = Player_HandleTick;
static BOOL  isplaying = 0, initialized = 0;
static UBYTE *sfxinfo;
static int   sfxpool;

static SAMPLE **md_sample = NULL;

/* Backup variables.  This way, the end programmer can fiddle with the */
/* main globals without mikmod blowing up. */

static UWORD idevice, imixfreq, imode, idmabufsize;


static void LimitHardVoices(int limit)

/* Limits the number of hardware voices to the specified amount. */
/* This function should only be used by the low-level drivers. */

{
    int t = 0;

    if(!(md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn > limit)) md_sfxchn = limit;
    if(!(md_mode & DMODE_SOFT_MUSIC) && (md_sngchn > limit)) md_sngchn = limit;

    if(!(md_mode & DMODE_SOFT_SNDFX))
       md_hardchn = md_sfxchn;
    else
       md_hardchn = 0;

    if(!(md_mode & DMODE_SOFT_MUSIC))
       md_hardchn += md_sngchn;
    
    while(md_hardchn > limit)
    {
        if(++t & 1)
            if(!(md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn > 4)) md_sfxchn--;
        else
            if(!(md_mode & DMODE_SOFT_MUSIC) && (md_sngchn > 8)) md_sngchn--;

        if(!(md_mode & DMODE_SOFT_SNDFX))
           md_hardchn = md_sfxchn;
        else
           md_hardchn = 0;

        if(!(md_mode & DMODE_SOFT_MUSIC))
           md_hardchn += md_sngchn;
    }

    md_numchn = md_hardchn + md_softchn;
}


static void LimitSoftVoices(int limit)

/* Limits the number of hardware voices to the specified amount. */
/* This function should only be used by the low-level drivers. */

{
    int t = 0;

    if((md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn > limit)) md_sfxchn = limit;
    if((md_mode & DMODE_SOFT_MUSIC) && (md_sngchn > limit)) md_sngchn = limit;

    if(md_mode & DMODE_SOFT_SNDFX)
       md_softchn = md_sfxchn;
    else
       md_softchn = 0;

    if(md_mode & DMODE_SOFT_MUSIC)
       md_softchn += md_sngchn;
    
    while(md_softchn > limit)
    {
        if(++t & 1)
            if((md_mode & DMODE_SOFT_SNDFX) && (md_sfxchn > 4)) md_sfxchn--;
        else
            if((md_mode & DMODE_SOFT_MUSIC) && (md_sngchn > 8)) md_sngchn--;

        if(!(md_mode & DMODE_SOFT_SNDFX))
           md_softchn = md_sfxchn;
        else
           md_softchn = 0;

        if(!(md_mode & DMODE_SOFT_MUSIC))
           md_softchn += md_sngchn;
    }

    md_numchn = md_hardchn + md_softchn;
}


/* Note: 'type' indicates whether the returned value should be for music */
/*       or for sound effects. */

ULONG MD_SampleSpace(int type)
{
    if(type==MD_MUSIC)
       type = (md_mode & DMODE_SOFT_MUSIC) ? MD_SOFTWARE : MD_HARDWARE;
    else if(type==MD_SNDFX)
       type = (md_mode & DMODE_SOFT_SNDFX) ? MD_SOFTWARE : MD_HARDWARE;

    return md_driver->FreeSampleSpace(type);
}


ULONG MD_SampleLength(int type, SAMPLE *s)
{
    if(type==MD_MUSIC)
       type = (md_mode & DMODE_SOFT_MUSIC) ? MD_SOFTWARE : MD_HARDWARE;
    else if(type==MD_SNDFX)
       type = (md_mode & DMODE_SOFT_SNDFX) ? MD_SOFTWARE : MD_HARDWARE;

    return md_driver->RealSampleLength(type, s);
}


UWORD MD_SetDMA(int secs)

/* Converts the given number of 1/10th seconds into the number of bytes of */
/* audio that a sample # 1/10th seconds long would require at the current md_* */
/* settings. */

{
    ULONG result;

    result = (md_mixfreq * ((md_mode & DMODE_STEREO) ? 2 : 1) *
             ((md_mode & DMODE_16BITS) ? 2 : 1) * secs) * 10;

    if(result > 32000) result = 32000;
    return(md_dmabufsize = (result & ~3));  /* round it off to an 8 byte boundry */
}


void MD_InfoDriver(void)
{
    int t;
    MDRIVER *l;

    /* list all registered devicedrivers: */
    for(t=1,l=firstdriver; l!=NULL; l=l->next, t++)
        printf("%d. %s\n",t,l->Version);
}


void MD_RegisterDriver(MDRIVER *drv)
{
    MDRIVER *cruise = firstdriver;

    if(cruise!=NULL)
    {   while(cruise->next!=NULL)  cruise = cruise->next;
        cruise->next = drv;
    } else
        firstdriver = drv; 
}


SWORD MD_SampleLoad(SAMPLOAD *s, int type, FILE *fp)
/*  type - sample type .. MD_MUSIC or MD_SNDFX */
{
    SWORD result;

    if(type==MD_MUSIC)
       type = (md_mode & DMODE_SOFT_MUSIC) ? MD_SOFTWARE : MD_HARDWARE;
    else if(type==MD_SNDFX)
       type = (md_mode & DMODE_SOFT_SNDFX) ? MD_SOFTWARE : MD_HARDWARE;

    SL_Init(s);
    result = md_driver->SampleLoad(s, type);
    SL_Exit(s);

    return result;
}


void MD_SampleUnLoad(SWORD handle)
{
    md_driver->SampleUnLoad(handle);
}


void MD_SetBPM(UBYTE bpm)
{
    md_bpm = bpm;
}


void MikMod_RegisterPlayer(void (*player)(void))
{
    md_player = player;
}


void MikMod_Update(void)
{
    if(isplaying && !(pf->forbid)) md_driver->Update();
}


void Voice_SetVolume(int voice, UWORD vol)
{
    ULONG  tmp;

    if((voice<0) || (voice>=md_numchn)) return;
    tmp = (ULONG)vol * (ULONG)md_volume * ((voice < md_sngchn) ? (ULONG)md_musicvolume : (ULONG)md_sndfxvolume);
    md_driver->VoiceSetVolume(voice,tmp/16384UL);
}


void Voice_SetFrequency(int voice, ULONG frq)
{
    if((voice < 0) || (voice >= md_numchn)) return;
    if(md_sample[voice]!=NULL && md_sample[voice]->divfactor!=0) frq/=md_sample[voice]->divfactor;
    md_driver->VoiceSetFrequency(voice, frq);
}


void Voice_SetPanning(int voice, ULONG pan)
{
    if((voice < 0) || (voice >= md_numchn)) return;
    if(pan!=PAN_SURROUND)
    {   if(md_mode & DMODE_REVERSE) pan = 255-pan;
        pan = (((SWORD)(pan-128)*md_pansep) / 128)+128;
    }
    md_driver->VoiceSetPanning(voice, pan);
}


void Voice_Play(int voice, SAMPLE *s, ULONG start)
{
    ULONG  repend;
    
    if((voice < 0) || (voice >= md_numchn) || (start >= s->length)) return;

    md_sample[voice] = s;
    repend = s->loopend;

    if(s->flags&SF_LOOP)
       if(repend > s->length) repend = s->length;    /* repend can't be bigger than size */

    md_driver->VoicePlay(voice,s->handle,start,s->length,s->loopstart,repend,s->flags);
}


void Voice_Stop(int voice)
{
    if((voice < 0) || (voice >= md_numchn)) return;
    if(voice >= md_sngchn)
    {   /* It is a sound effects channel, so flag the voice as non-critical! */
        sfxinfo[voice-md_sngchn] = 0;
    }

    md_driver->VoiceStop(voice);
}

 
BOOL Voice_Stopped(int voice)
{
    if((voice < 0) || (voice >= md_numchn)) return 0;
    return(md_driver->VoiceStopped(voice));
}


SLONG Voice_GetPosition(int voice)
{
    if((voice < 0) || (voice >= md_numchn)) return 0;
    return(md_driver->VoiceGetPosition(voice));
}


ULONG Voice_RealVolume(int voice)
{
    if((voice < 0) || (voice >= md_numchn)) return 0;
    return(md_driver->VoiceRealVolume(voice));
}


/* ================================ */
/*  Functions prefixed with MikMod */
/* ================================ */

BOOL MikMod_Init(void)
{
    UWORD t;

    _mm_critical = 1;

    /* if md_device==0, try to find a device number */

    if(md_device==0)
    {   for(t=1,md_driver=firstdriver; md_driver!=NULL; md_driver=md_driver->next, t++)
        {   if(md_driver->IsPresent()) break;
        }

        if(md_driver==NULL)
        {   _mm_errno = MMERR_DETECTING_DEVICE;
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            md_driver = &drv_nos;
            return 1;
        }

        md_device = t;
    } else
    {   /* if n>0 use that driver */
        for(t=1,md_driver=firstdriver; (md_driver!=NULL) && (t!=md_device); md_driver=md_driver->next, t++);

        if(md_driver==NULL)
        {   _mm_errno = MMERR_INVALID_DEVICE;
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            md_driver = &drv_nos;
            return 1;
        }
    
        if(!md_driver->IsPresent())
        {   _mm_errno = MMERR_DETECTING_DEVICE;
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            md_driver = &drv_nos;
            return 1;
        }
    }

    if(md_driver->Init())
    {   MikMod_Exit();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return 1;
    }

    idevice  = md_device;   imode       = md_mode;
    imixfreq = md_mixfreq;  idmabufsize = md_dmabufsize;
    initialized = 1;
    _mm_critical = 0;

    return 0;
}


void MikMod_Exit(void)
{
    MikMod_DisableOutput();
    md_driver->Exit();
    md_numchn = md_sfxchn = md_sngchn = 0;
    md_driver = &drv_nos;
    initialized = 0;
}


BOOL MikMod_Reset(void)

/* Reset the driver using the new global variable settings. */
/* If the driver has not been initialized, it will be now. */

{
    if(!initialized) return MikMod_Init();
    if((md_driver->Reset == NULL) || (md_device != idevice))
    {   /* md_driver->Reset was NULL, or md_device was changed, */
        /* so do a full reset of the driver. */

        if(isplaying) md_driver->PlayStop();
        md_driver->Exit();
        if(MikMod_Init())
        {   MikMod_Exit();
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            return 1;
        }
        if(isplaying) md_driver->PlayStart();
    } else
    {   if(md_driver->Reset())
        {   MikMod_Exit();
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            return 1;
        }
    }
    
    return 0;
}


BOOL MikMod_SetNumVoices(int music, int sfx)

/* If either parameter is -1, the current set value will be retained. */

{
    BOOL resume = 0;
    int  t, oldchn = 0;

    if((music==0) && (sfx==0)) return 0;

    _mm_critical = 1;

    if(isplaying)
    {   MikMod_DisableOutput();
        oldchn = md_numchn;
        resume = 1;
    }

    if(sfxinfo!=NULL)   free(sfxinfo);
    if(md_sample!=NULL) free(md_sample);
    md_sample  = NULL;
    sfxinfo    = NULL;

    /*md_softchn = md_hardchn = 0;

    if(md_mode & DMODE_SOFT_SNDFX)
       md_softchn = sfx; else md_hardchn = sfx;

    if(md_mode & DMODE_SOFT_MUSIC)
       md_softchn += music; else md_hardchn += music;
    */

    if(music != -1) md_sngchn = music;
    if(sfx != -1)   md_sfxchn = sfx;

    md_numchn = md_sngchn + md_sfxchn;

    LimitHardVoices(md_driver->HardVoiceLimit);
    LimitSoftVoices(md_driver->SoftVoiceLimit);

    if(md_driver->SetNumVoices())
    {   MikMod_Exit();
        md_numchn = md_softchn = md_hardchn = md_sfxchn = md_sngchn = 0;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return 1;
    }

    if(md_sngchn || md_sfxchn)
        md_sample = (SAMPLE **)_mm_calloc(md_sngchn+md_sfxchn, sizeof(SAMPLE *));
    if(md_sfxchn)
        sfxinfo = (UBYTE *)_mm_calloc(md_sfxchn, sizeof(UBYTE));

    /* make sure the player doesn't start with garbage */
    for(t=oldchn; t<md_numchn; t++)  Voice_Stop(t);

    sfxpool = 0;

    if(resume) MikMod_EnableOutput();
    _mm_critical = 0;

    return 0;
}


BOOL MikMod_EnableOutput(void)
{
    /* safety valve, prevents entering */
    /* playstart twice: */

    _mm_critical = 1;
    if(!isplaying)
    {   if(md_driver->PlayStart()) return 1;
        isplaying = 1;
    }
    _mm_critical = 0;
    return 0;
}


void MikMod_DisableOutput(void)
{
    /* safety valve, prevents calling playStop when playstart */
    /* hasn't been called: */

    if(isplaying && md_driver!=NULL)
    {   isplaying = 0;
        md_driver->PlayStop();
    }
}


BOOL MikMod_Active(void)
{
    return isplaying;
}


int MikMod_PlaySample(SAMPLE *s, ULONG start, UBYTE flags)

/* Plays a sound effects sample.  Picks a voice from the number of voices */
/* allocated for use as sound effects (loops through voices, skipping all */
/* active criticals). */
/* */
/* Returns the voice that the sound is being played on. */

{
    int orig = sfxpool;    /* for cases where all channels are critical */
    int c;

    if(md_sfxchn==0) return -1;
    if(s->volume > 64) s->volume = 64;
    
    /* check the first location after sfxpool */
    do
    {   if(sfxinfo[sfxpool] & SFX_CRITICAL)
        {   if(md_driver->VoiceStopped(c=sfxpool+md_sngchn))
            {   sfxinfo[sfxpool] = flags;
                Voice_Play(c, s, start);
                md_driver->VoiceSetVolume(c,s->volume<<2);
                md_driver->VoiceSetPanning(c,s->panning);
                md_driver->VoiceSetFrequency(c,s->speed);
                sfxpool++;
                if(sfxpool >= md_sfxchn) sfxpool = 0;
                return c;
            }
        } else
        {   sfxinfo[sfxpool] = flags;
            Voice_Play(c=sfxpool+md_sngchn, s, start);
            md_driver->VoiceSetVolume(c,s->volume<<2);
            md_driver->VoiceSetPanning(c,s->panning);
            md_driver->VoiceSetFrequency(c,s->speed);
            sfxpool++;
            if(sfxpool >= md_sfxchn) sfxpool = 0;
            return c;
        }

        sfxpool++;
        if(sfxpool >= md_sfxchn) sfxpool = 0;
    } while(sfxpool!=orig);

    return -1;
}

