/*

 Name: LOAD_MOD.C

 Description:
 Generic MOD loader (Protracker, StarTracker, FastTracker, etc)

 Portability:
 All systems - all compilers (hopefully)

 If this module is found to not be portable to any particular platform,
 please contact Jake Stine at dracoirs@epix.net (see MIKMOD.TXT for
 more information on contacting the author).

*/

#include <string.h>
#include "mikmod.h"

/*************************************************************************
*************************************************************************/


typedef struct MSAMPINFO                  /* sample header as it appears in a module */
{   CHAR  samplename[22];
    UWORD length;
    UBYTE finetune;
    UBYTE volume;
    UWORD reppos;
    UWORD replen;
} MSAMPINFO;


typedef struct MODULEHEADER              /* verbatim module header */
{   CHAR       songname[20];             /* the songname.. */
    MSAMPINFO  samples[31];              /* all sampleinfo */
    UBYTE      songlength;               /* number of patterns used */
    UBYTE      magic1;                   /* should be 127 */
    UBYTE      positions[128];           /* which pattern to play at pos */
    UBYTE      magic2[4];                /* string "M.K." or "FLT4" or "FLT8" */
} MODULEHEADER;

#define MODULEHEADERSIZE 1084


typedef struct MODTYPE              /* struct to identify type of module */
{   CHAR    id[5];
    UBYTE   channels;
    CHAR    *name;
} MODTYPE;


typedef struct MODNOTE
{   UBYTE a,b,c,d;
} MODNOTE;


/*************************************************************************
*************************************************************************/


CHAR protracker[]   = "Protracker";
CHAR startracker[]  = "Startracker";
CHAR fasttracker[]  = "Fasttracker";
CHAR ins15tracker[] = "15-instrument";
CHAR oktalyzer[]    = "Oktalyzer";
CHAR taketracker[]  = "TakeTracker";


MODTYPE modtypes[] =
{   "M.K.",4,protracker,      /* protracker 4 channel */
    "M!K!",4,protracker,      /* protracker 4 channel */
    "FLT4",4,startracker,     /* startracker 4 channel */
    "2CHN",2,fasttracker,     /* fasttracker 2 channel */
    "4CHN",4,fasttracker,     /* fasttracker 4 channel */
    "6CHN",6,fasttracker,     /* fasttracker 6 channel */
    "8CHN",8,fasttracker,     /* fasttracker 8 channel */
    "10CH",10,fasttracker,    /* fasttracker 10 channel */
    "12CH",12,fasttracker,    /* fasttracker 12 channel */
    "14CH",14,fasttracker,    /* fasttracker 14 channel */
    "16CH",16,fasttracker,    /* fasttracker 16 channel */
    "18CH",18,fasttracker,    /* fasttracker 18 channel */
    "20CH",20,fasttracker,    /* fasttracker 20 channel */
    "22CH",22,fasttracker,    /* fasttracker 22 channel */
    "24CH",24,fasttracker,    /* fasttracker 24 channel */
    "26CH",26,fasttracker,    /* fasttracker 26 channel */
    "28CH",28,fasttracker,    /* fasttracker 28 channel */
    "30CH",30,fasttracker,    /* fasttracker 30 channel */
    "32CH",32,fasttracker,    /* fasttracker 32 channel */
    "CD81",8,oktalyzer,       /* atari oktalyzer 8 channel */
    "OKTA",8,oktalyzer,       /* atari oktalyzer 8 channel */
    "16CN",16,taketracker,    /* taketracker 16 channel */
    "32CN",32,taketracker,    /* taketracker 32 channel */
    "    ",4,ins15tracker     /* 15-instrument 4 channel     */
};

static MODULEHEADER *mh = NULL;        /* raw as-is module header */
static MODNOTE *patbuf = NULL;
static int modtype = 0;

BOOL MOD_Test(void)
{
    UBYTE id[4];

    _mm_fseek(modfp,MODULEHEADERSIZE-4,SEEK_SET);
    if(!fread(id,4,1,modfp)) return 0;

    /* find out which ID string */

    for(modtype=0; modtype<23; modtype++)
        if(!memcmp(id,modtypes[modtype].id,4)) return 1;

    return 0;
}


BOOL MOD_Init(void)
{
    if(!(mh=(MODULEHEADER *)_mm_calloc(1,sizeof(MODULEHEADER)))) return 0;
    return 1;
}


void MOD_Cleanup(void)
{
    if(mh!=NULL) free(mh);
    if(patbuf!=NULL) free(patbuf);

    mh = NULL;
    patbuf = NULL;
}


/*
Old (amiga) noteinfo:

 _____byte 1_____   byte2_    _____byte 3_____   byte4_
/                \ /      \  /                \ /      \
0000          0000-00000000  0000          0000-00000000

Upper four    12 bits for    Lower four    Effect command.
bits of sam-  note period.   bits of sam-
ple number.                  ple number.

*/


void ConvertNote(MODNOTE *n)
{
    UBYTE instrument,effect,effdat,note;
    UWORD period;

    /* extract the various information from the 4 bytes that */
    /* make up a single note */

    instrument = (n->a&0x10)|(n->c>>4);
    period     = (((UWORD)n->a&0xf)<<8)+n->b;
    effect     = n->c&0xf;
    effdat     = n->d;

    /* Convert the period to a note number */

    note=0;
    if(period!=0)
    {   for(note=0; note<60; note++)
            if(period >= npertab[note]) break;
        note++;
        if(note==61) note = 0;
    }

    if(instrument!=0) UniInstrument(instrument-1);
    if(note!=0) UniNote(note+23);

    /* Convert pattern jump from Dec to Hex */
    if(effect == 0xd)
        effdat = (((effdat&0xf0)>>4)*10)+(effdat&0xf);

    UniPTEffect(effect,effdat);
}


UBYTE *ConvertTrack(MODNOTE *n)
{
    int t;

    UniReset();
    for(t=0;t<64;t++)
    {   ConvertNote(n);
        UniNewline();
        n+=of.numchn;
    }
    return UniDup();
}


BOOL ML_LoadPatterns(void)
/*  Loads all patterns of a modfile and converts them into the */
/*  3 byte format. */
{
    int t,s,tracks = 0;

    if(!AllocPatterns()) return 0;
    if(!AllocTracks())   return 0;

    /* Allocate temporary buffer for loading */
    /* and converting the patterns */

    if(!(patbuf=(MODNOTE *)_mm_calloc(64U*of.numchn,sizeof(MODNOTE)))) return 0;

    for(t=0; t<of.numpat; t++)
    {   /* Load the pattern into the temp buffer */
        /* and convert it */

        for(s=0; s<(64U*of.numchn); s++)
        {   patbuf[s].a = _mm_read_UBYTE(modfp);
            patbuf[s].b = _mm_read_UBYTE(modfp);
            patbuf[s].c = _mm_read_UBYTE(modfp);
            patbuf[s].d = _mm_read_UBYTE(modfp);
        }

        for(s=0; s<of.numchn; s++)
            if(!(of.tracks[tracks++]=ConvertTrack(patbuf+s))) return 0;
    }

    return 1;
}


BOOL MOD_Load(void)
{
    int       t;
    SAMPLE    *q;
    MSAMPINFO *s;           /* old module sampleinfo */

    /* try to read module header */

    _mm_read_string((CHAR *)mh->songname,20,modfp);

    for(t=0; t<31; t++)
    {   s = &mh->samples[t];
        _mm_read_string(s->samplename,22,modfp);
        s->length   =_mm_read_M_UWORD(modfp);
        s->finetune =_mm_read_UBYTE(modfp);
        s->volume   =_mm_read_UBYTE(modfp);
        s->reppos   =_mm_read_M_UWORD(modfp);
        s->replen   =_mm_read_M_UWORD(modfp);
    }

    mh->songlength  =_mm_read_UBYTE(modfp);
    mh->magic1      =_mm_read_UBYTE(modfp);

    _mm_read_UBYTES(mh->positions,128,modfp);
    _mm_read_UBYTES(mh->magic2,4,modfp);

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    /* set module variables */

    of.initspeed = 6;
    of.inittempo = 125;
    of.numchn    = modtypes[modtype].channels;      /* get number of channels */
    of.modtype   = strdup(modtypes[modtype].name);  /* get ascii type of mod */
    of.songname  = DupStr(mh->songname,20);         /* make a cstr of songname */
    of.numpos    = mh->songlength;                  /* copy the songlength */

    if(!AllocPositions(of.numpos)) return 0;
    for(t=0; t<of.numpos; t++)
        of.positions[t] = mh->positions[t];

    /* Count the number of patterns */

    of.numpat = 0;

    for(t=0; t<of.numpos; t++)
    {   if(of.positions[t] > of.numpat)
            of.numpat = of.positions[t];
    }
    of.numpat++;
    of.numtrk = of.numpat*of.numchn;
    
    /* Finally, init the sampleinfo structures  */
    of.numins = of.numsmp = 31;

    if(!AllocSamples())     return 0;
    
    s = mh->samples;       /* init source pointer  */
    q = of.samples;
    
    for(t=0; t<of.numins; t++)
    {   /* convert the samplename */
        q->samplename = DupStr(s->samplename, 22);

        /* init the sampleinfo variables and */
        /* convert the size pointers to longword format */

        q->speed     = finetune[s->finetune & 0xf];
        q->volume    = s->volume;
        q->loopstart = (ULONG)s->reppos << 1;
        q->loopend   = q->loopstart + ((ULONG)s->replen << 1);
        q->length    = (ULONG)s->length << 1;

        q->flags     = SF_SIGNED;
        if(s->replen > 1) q->flags |= SF_LOOP;

        /* fix replen if repend > length */
        if(q->loopend > q->length) q->loopend = q->length;

        s++;    /* point to next source sampleinfo */
        q++;
    }

    if(!ML_LoadPatterns()) return 0;
    return 1;
}


CHAR *MOD_LoadTitle(void)
{
   CHAR s[20];

   _mm_fseek(modfp,0,SEEK_SET);
   if(!fread(s,20,1,modfp)) return NULL;
   
   return(DupStr(s,20));
}


MLOADER load_mod =
{   NULL,
    "Standard module",
    "Portable MOD loader v0.11",
    MOD_Init,
    MOD_Test,
    MOD_Load,
    MOD_Cleanup,
    MOD_LoadTitle
};

