/*

Name:  MLOADER.C

Description:
 These routines are used to access the available module loaders

Portability:
 All systems - all compilers

*/

#include <string.h>
#include "mikmod.h"


FILE   *modfp;
UNIMOD of;

static MLOADER *firstloader = NULL;

UWORD finetune[16] =
{   8363,   8413,   8463,   8529,   8581,   8651,   8723,   8757,
    7895,   7941,   7985,   8046,   8107,   8169,   8232,   8280
};


void ML_InfoLoader(void)
{
    int      t;
    MLOADER *l;

    /* list all registered devicedrivers: */

    for(t=1,l=firstloader; l!=NULL; l=l->next, t++)
        printf("%d. %s\n",t,l->version);
}


void ML_RegisterLoader(MLOADER *ldr)
{
    MLOADER *cruise = firstloader;

    if(cruise!=NULL)
    {   while(cruise->next!=NULL)  cruise = cruise->next;
        cruise->next = ldr;
    } else
        firstloader = ldr; 
}


BOOL ReadComment(UWORD len)
{
    /*int t; */

    if(len)
    {   if(!(of.comment=(CHAR *)_mm_malloc(len+1))) return 0;
        fread(of.comment,len,1,modfp);
        of.comment[len] = 0;
    }
    return 1;
}


BOOL AllocPositions(int total)
{
    if((of.positions = _mm_calloc(total,sizeof(UWORD))) == NULL) return 0;
    return 1;
}


BOOL AllocPatterns(void)
{
    int s,t,tracks = 0;

    /* Allocate track sequencing array */

    if(!(of.patterns = (UWORD *)_mm_calloc((ULONG)(of.numpat+1)*of.numchn,sizeof(UWORD)))) return 0;
    if(!(of.pattrows = (UWORD *)_mm_calloc(of.numpat+1,sizeof(UWORD)))) return 0;

    for(t=0; t<of.numpat+1; t++)
    {   of.pattrows[t] = 64;
        for(s=0; s<of.numchn; s++)
            of.patterns[(t*of.numchn)+s] = tracks++;
    }

    return 1;
}


BOOL AllocTracks(void)
{
    if(!(of.tracks=(UBYTE **)_mm_calloc(of.numtrk,sizeof(UBYTE *)))) return 0;
    return 1;
}


BOOL AllocInstruments(void)
{
    int t,n;

    if((of.instruments=(INSTRUMENT *)_mm_calloc(of.numins,sizeof(INSTRUMENT)))==NULL) return 0;

    for(t=0; t<of.numins; t++)
    {  for(n=0; n<120; n++)     /* Init note / sample lookup table */
       {  of.instruments[t].samplenote[n]   = n;
          of.instruments[t].samplenumber[n] = t;
       }   
       of.instruments[t].globvol = 64;
    }
    return 1;
}


BOOL AllocSamples(void)
{
    UWORD u;

    if((of.samples = (SAMPLE *)_mm_calloc(of.numsmp,sizeof(SAMPLE)))==NULL) return 0;

    for(u=0; u<of.numsmp; u++)
    {   of.samples[u].panning = 128;
        of.samples[u].handle  = -1;
        of.samples[u].globvol = 64;
        of.samples[u].volume  = 64;
    }
    return 1;
}


BOOL ML_LoadSamples(void)
{
    SAMPLE *s;
    int    u;

    for(u=of.numsmp, s=of.samples; u; u--, s++)
        if(s->length) SL_RegisterSample(s,MD_MUSIC,modfp);

    return 1;
}


CHAR *DupStr(CHAR *s, UWORD len)
/*  Creates a CSTR out of a character buffer of 'len' bytes, but strips */
/*  any terminating non-printing characters like 0, spaces etc. */
{
    UWORD t;
    CHAR  *d = NULL;

    /* Scan for first printing char in buffer [includes high ascii up to 254] */
    while(len)
    {   if(s[len-1] > 0x20) break;
        len--;
    }

    /* When the buffer wasn't completely empty, allocate */
    /* a cstring and copy the buffer into that string, except */
    /* for any control-chars */

#ifdef __GNUC__
    if(len<16) len = 16;
#endif

    if((d=(CHAR *)_mm_malloc(len+1)) != NULL)
    {   for(t=0; t<len; t++) d[t] = (s[t]<32) ? ' ' : s[t];
        d[t] = 0;
    }

    return d;
}


static void ML_XFreeSample(SAMPLE *s)
{
   if(s->handle>=0)
      MD_SampleUnLoad(s->handle);
   if(s->samplename!=NULL) free(s->samplename);
}


static void ML_XFreeInstrument(INSTRUMENT *i)
{
    if(i->insname!=NULL) free(i->insname);
}


static void ML_FreeEx(UNIMOD *mf)
{
    UWORD t;

    if(mf->songname!=NULL) free(mf->songname);
    if(mf->composer!=NULL) free(mf->composer);
    if(mf->comment!=NULL)  free(mf->comment);

    if(mf->modtype!=NULL)   free(mf->modtype);
    if(mf->positions!=NULL) free(mf->positions);
    if(mf->patterns!=NULL)  free(mf->patterns);
    if(mf->pattrows!=NULL)  free(mf->pattrows);

    if(mf->tracks!=NULL)
    {   for(t=0; t<mf->numtrk; t++)
            if(mf->tracks[t]!=NULL) free(mf->tracks[t]);
        free(mf->tracks);
    }

    if(mf->instruments != NULL)
    {   for(t=0; t<mf->numins; t++)
            ML_XFreeInstrument(&mf->instruments[t]);
        free(mf->instruments);
    }

    if(mf->samples != NULL)
    {   for(t=0; t<mf->numsmp; t++)
            if(mf->samples[t].length) ML_XFreeSample(&mf->samples[t]);
        free(mf->samples);
    }

    memset(mf,0,sizeof(UNIMOD));
}


static UNIMOD *ML_AllocUniMod(void)
{
   UNIMOD *mf;
   
   if((mf=_mm_calloc(1,sizeof(UNIMOD))) == NULL) return NULL;
   return mf;
}


/******************************************

    Next are the user-callable functions

******************************************/


void MikMod_FreeSong(UNIMOD *mf)
{
   if(mf!=NULL)
   {   Player_Exit(mf);
       ML_FreeEx(mf);
   }
}


CHAR *MikMod_LoadSongTitle(CHAR *filename)
{
    MLOADER *l;
    CHAR    *retval;
    FILE    *fp;

    if((fp = _mm_fopen(filename,"rb"))==NULL) return NULL;

    _mm_errno = 0;
    _mm_critical = 0;
    _mm_iobase_setcur(modfp);

    /* Try to find a loader that recognizes the module */

    for(l=firstloader; l!=NULL; l=l->next)
    {   _mm_rewind(modfp);
        if(l->Test()) break;
    }

    if(l==NULL)
    {   _mm_errno = MMERR_NOT_A_MODULE;
        _mm_iobase_revert();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    retval = l->LoadTitle();

    fclose(fp);
    return(retval);
}


UNIMOD *MikMod_LoadSongFP(FILE *fp, int maxchan)

/* Loads a module given a file pointer. */
/* File is loaded from the current file seek position. */

{
    int      t;
    MLOADER *l;
    BOOL     ok;
    UNIMOD  *mf;

    modfp = fp;
    _mm_errno = 0;
    _mm_critical = 0;

    _mm_iobase_setcur(modfp);

    /* Try to find a loader that recognizes the module */

    for(l=firstloader; l!=NULL; l=l->next)
    {   _mm_rewind(modfp);
        if(l->Test()) break;
    }

    if(l==NULL)
    {   _mm_errno = MMERR_NOT_A_MODULE;
        _mm_iobase_revert();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    /* init unitrk routines */
    if(!UniInit())
    {   if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    /* load the song using the song's loader variable */
    memset(&of,0,sizeof(UNIMOD));
    of.initvolume = 128;

    /* init panning array */
    for(t=0; t<64; t++) of.panning[t] = ((t+1)&2) ? 255 : 0;
    for(t=0; t<64; t++) of.chanvol[t] = 64;

    /* init module loader and load the header / patterns */
    if(l->Init())
    {   _mm_rewind(modfp);
        ok = l->Load();
    } else ok = 0;

    /* free loader and unitrk allocations */
    l->Cleanup();
    UniCleanup();

    if(!ok)
    {   ML_FreeEx(&of);
        _mm_iobase_revert();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    if(!ML_LoadSamples())
    {   ML_FreeEx(&of);
        _mm_iobase_revert();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    if((mf=ML_AllocUniMod()) == NULL)
    {   ML_FreeEx(&of);
        _mm_iobase_revert();
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    /* Copy the static UNIMOD contents into the dynamic UNIMOD struct. */
    memcpy(mf,&of,sizeof(UNIMOD));

    _mm_iobase_revert();

    if(maxchan > 0)
    {   if(!(mf->flags & UF_NNA) && (mf->numchn < maxchan))
            maxchan = mf->numchn;
        else if((mf->numvoices!=0) && mf->numvoices < maxchan)
            maxchan = mf->numvoices;

        if(maxchan < mf->numchn) mf->flags |= UF_NNA;

        if(MikMod_SetNumVoices(maxchan,-1))
        {   MikMod_FreeSong(mf);
            return NULL;
        }
    }

    return mf;
}


UNIMOD *MikMod_LoadSong(CHAR *filename, int maxchan)

/* Open a module via it's filename.  The loader will initialize the specified */
/* song-player 'player'. */

{
    FILE   *fp;
    UNIMOD *mf;
    
    if((fp = _mm_fopen(filename,"rb"))==NULL) return NULL;
    if((mf = MikMod_LoadSongFP(fp, maxchan)) != NULL)
    {   if(SL_LoadSamples() || Player_Init(mf))
        {   MikMod_FreeSong(mf);
            mf = NULL;
        }
    }

    fclose(fp);
    return mf;
}

