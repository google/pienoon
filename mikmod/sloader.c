/*
 --> Sample Loaders and Sample Processing

Name: sloader.c

Description:
Routines for loading samples.  The sample loader utilizes the routines
provided by the "registered" sample loader.  See SAMPLELOADER in
MIKMOD.H for the sample loader structure.

Portability:
All systems - all compilers

*/

#include "mikmod.h"


static int   sl_rlength;
static SWORD sl_old;
static SWORD *sl_buffer = NULL;
static SAMPLOAD *musiclist = NULL,
                *sndfxlist = NULL;



BOOL SL_Init(SAMPLOAD *s)        /* returns 0 on error! */
{
    if(sl_buffer==NULL)
         if((sl_buffer=_mm_malloc(4100)) == NULL) return 0;

    sl_rlength = s->length;
    if(s->infmt & SF_16BITS) sl_rlength>>=1;
    sl_old = 0;

    return 1;
}


void SL_Exit(SAMPLOAD *s)
{
    if(sl_rlength > 0) _mm_fseek(s->fp,sl_rlength,SEEK_CUR);
}


void SL_Reset(void)
{
    sl_old = 0;
}


void SL_Load(void *buffer, SAMPLOAD *smp, int length)
{
    UWORD infmt = smp->infmt, outfmt = smp->outfmt;
    SBYTE *bptr = (SBYTE *)buffer;
    SWORD *wptr = (SWORD *)buffer;
    int   stodo;
    int   t, u;

    while(length)
    {   stodo = (length<2048) ? length : 2048;

        if(infmt & SF_16BITS)
        {   if(infmt & SF_BIG_ENDIAN)
                _mm_read_M_SWORDS(sl_buffer,stodo,smp->fp);
            else
                _mm_read_I_SWORDS(sl_buffer,stodo,smp->fp);
        } else
        {   SBYTE  *s;
            SWORD  *d;

            fread(sl_buffer,sizeof(SBYTE),stodo,smp->fp);

            s  = (SBYTE *)sl_buffer;
            d  = sl_buffer;
            s += stodo;
            d += stodo;

            for(t=0; t<stodo; t++)
            {   s--;
                d--;
                *d = (*s)<<8;
            }
        }

        if(infmt & SF_DELTA)
        {   for(t=0; t<stodo; t++)
            {   sl_buffer[t] += sl_old;
                sl_old = sl_buffer[t];
            }
        }

        if((infmt^outfmt) & SF_SIGNED)
        {   for(t=0; t<stodo; t++)
               sl_buffer[t] ^= 0x8000;
        }

        if(smp->scalefactor)
        {   int   idx = 0;
            SLONG scaleval;

            /* Sample Scaling... average values for better results. */
            t = 0;
            while(t<stodo && length)
            {   scaleval = 0;
                for(u=smp->scalefactor; u && t<stodo; u--, t++)
                    scaleval += sl_buffer[t];
                sl_buffer[idx++] = scaleval / (smp->scalefactor-u);
                length--;
            }
            sl_rlength -= stodo;
            stodo = idx;
        } else
        {   length -= stodo;
            sl_rlength -= stodo;
        }

        if(outfmt & SF_16BITS)
        {   for(t=0; t<stodo; t++) *(wptr++) = sl_buffer[t];
        } else
        {   for(t=0; t<stodo; t++) *(bptr++) = sl_buffer[t]>>8;
        }
    }
}


void SL_LoadStream(void *buffer, UWORD infmt, UWORD outfmt, int length, FILE *fp)

/* This is like SL_Load, but does not perform sample scaling, and does not */
/* require calls to SL_Init / SL_Exit. */

{
    SBYTE *bptr = (SBYTE *)buffer;
    SWORD *wptr = (SWORD *)buffer;
    int   stodo;
    int   t;

    /* compute number of samples to load */

    if(sl_buffer==NULL)
         if((sl_buffer=_mm_malloc(4100)) == NULL) return;

    while(length)
    {   stodo = (length<2048) ? length : 2048;

        if(infmt & SF_16BITS)
        {   if(infmt & SF_BIG_ENDIAN)
                _mm_read_M_SWORDS(sl_buffer,stodo,fp);
            else
                _mm_read_I_SWORDS(sl_buffer,stodo,fp);
        } else
        {   SBYTE  *s;
            SWORD  *d;

            fread(sl_buffer,sizeof(SBYTE),stodo,fp);

            s  = (SBYTE *)sl_buffer;
            d  = sl_buffer;
            s += stodo;
            d += stodo;

            for(t=0; t<stodo; t++)
            {   s--;
                d--;
                *d = (*s)<<8;
            }
        }

        if(infmt & SF_DELTA)
        {   for(t=0; t<stodo; t++)
            {   sl_buffer[t] += sl_old;
                sl_old = sl_buffer[t];
            }
        }

        if((infmt^outfmt) & SF_SIGNED)
        {   for(t=0; t<stodo; t++)
               sl_buffer[t] ^= 0x8000;
        }

        length -= stodo;

        if((infmt & SF_STEREO) && !(outfmt & SF_STEREO))
        {   /* Dither stereo to mono .. average together every two samples */
            SLONG avgval;
            int   idx = 0;

            t = 0;
            while(t<stodo && length)
            {   avgval  = sl_buffer[t++];
                avgval += sl_buffer[t++];
                sl_buffer[idx++] = avgval >> 1; 
                length-=2;
            }
            stodo = idx;
        }

        if(outfmt & SF_16BITS)
        {   for(t=0; t<stodo; t++) *(wptr++) = sl_buffer[t];
        } else
        {   for(t=0; t<stodo; t++) *(bptr++) = sl_buffer[t]>>8;
        }
    }
}


SAMPLOAD *SL_RegisterSample(SAMPLE *s, int type, FILE *fp)    /* Returns 1 on error! */

/* Registers a sample for loading when SL_LoadSamples() is called. */
/*   type  -  type of sample to be loaded .. */
/*             MD_MUSIC, MD_SNDFX */

{
    SAMPLOAD *news, **samplist, *cruise;

    if(type==MD_MUSIC)
    {   samplist = & musiclist;
        cruise = musiclist;
    } else
    {   samplist = &sndfxlist;
        cruise = sndfxlist;
    }

    /* Allocate and add structure to the END of the list */

    if((news=(SAMPLOAD *)_mm_calloc(1,sizeof(SAMPLOAD))) == NULL) return NULL;

    if(cruise!=NULL)
    {   while(cruise->next!=NULL)  cruise = cruise->next;
        cruise->next = news;
    } else
        *samplist = news;

    news->infmt     = s->flags & 31;
    news->outfmt    = news->infmt;
    news->fp        = fp;
    news->sample    = s;
    news->length    = s->length;
    news->loopstart = s->loopstart;
    news->loopend   = s->loopend;

    return news;
}


static void FreeSampleList(SAMPLOAD *s)
{
    SAMPLOAD *old;

    while(s!=NULL)
    {   old = s;
        s = s->next;
        free(old);
    }
}


static ULONG SampleTotal(SAMPLOAD *samplist, int type)
/* Returns the total amount of memory required by the samplelist queue. */
{
    int total = 0;

    while(samplist!=NULL)
    {   samplist->sample->flags = (samplist->sample->flags&~31) | samplist->outfmt;
        total += MD_SampleLength(type,samplist->sample);
        samplist = samplist->next;
    }

    return total;
}


static ULONG RealSpeed(SAMPLOAD *s)
{
    return(s->sample->speed / ((s->scalefactor==0) ? 1 : s->scalefactor));
}    


static BOOL DitherSamples(SAMPLOAD *samplist, int type)
{
    SAMPLOAD  *c2smp;
    ULONG     maxsize, speed;
    SAMPLOAD  *s;

    if(samplist==NULL) return 0;

    /* make sure the samples will fit inside available RAM */
    if((maxsize = MD_SampleSpace(type)*1024) != 0)
    {   while(SampleTotal(samplist, type) > maxsize)
        {   /* First Pass - check for any 16 bit samples */
            s = samplist;
            while(s!=NULL)
            {   if(s->outfmt & SF_16BITS)
                {   SL_Sample16to8(s);
                    break;
                }
                s = s->next;
            }
    
            /* Second pass (if no 16bits found above) is to take the sample */
            /* with the highest speed and dither it by half. */
            if(s==NULL)
            {   s = samplist;
                speed = 0;
                while(s!=NULL)
                {   if((s->sample->length) && (RealSpeed(s) > speed))
                    {   speed = RealSpeed(s);
                        c2smp = s;
                    }
                    s = s->next;
                }
                SL_HalveSample(c2smp);
            }
        }
    }


    /* Samples dithered, now load them! */
    /* ================================ */

    s = samplist;
    while(s != NULL)
    {   /* sample has to be loaded ? -> increase number of */
        /* samples, allocate memory and load sample. */

        if(s->sample->length)
        {   if(s->sample->seekpos)
               _mm_fseek(s->fp, s->sample->seekpos, SEEK_SET);

            /* Call the sample load routine of the driver module. */
            /* It has to return a 'handle' (>=0) that identifies */
            /* the sample. */

            s->sample->handle = MD_SampleLoad(s, type, s->fp);
            s->sample->flags  = (s->sample->flags & ~31) | s->outfmt;
            if(s->sample->handle < 0)
            {   FreeSampleList(samplist);
                if(_mm_errorhandler!=NULL) _mm_errorhandler();
                return 1;
            }
        }
        s = s->next;
    }

    FreeSampleList(samplist);
    return 0;
}


BOOL SL_LoadSamples(void)      /* Returns 1 on error! */
{
    BOOL ok;

    _mm_critical = 0;

    if((musiclist==NULL) && (sndfxlist==NULL)) return 0;
/*    MikMod_Exit(); */
/*    exit(1); */
    ok = DitherSamples(musiclist,MD_MUSIC) || DitherSamples(sndfxlist,MD_SNDFX);

    musiclist = sndfxlist = NULL;

    return ok;
}


void SL_Sample16to8(SAMPLOAD *s)
{
    s->outfmt &= ~SF_16BITS;
    s->sample->flags = (s->sample->flags&~31) | s->outfmt;
}


void SL_Sample8to16(SAMPLOAD *s)
{
    s->outfmt |= SF_16BITS;
    s->sample->flags = (s->sample->flags&~31) | s->outfmt;
}


void SL_SampleSigned(SAMPLOAD *s)
{
    s->outfmt |= SF_SIGNED;
    s->sample->flags = (s->sample->flags&~31) | s->outfmt;
}


void SL_SampleUnsigned(SAMPLOAD *s)
{
    s->outfmt &= ~SF_SIGNED;
    s->sample->flags = (s->sample->flags&~31) | s->outfmt;
}


void SL_HalveSample(SAMPLOAD *s)
{
    if(s->scalefactor)
        s->scalefactor++;
    else
        s->scalefactor = 2;

    s->sample->divfactor = s->scalefactor;
    s->sample->length    = s->length / s->scalefactor;
    s->sample->loopstart = s->loopstart / s->scalefactor;
    s->sample->loopend   = s->loopend / s->scalefactor;
}

