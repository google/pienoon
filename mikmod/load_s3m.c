/*

 Name: LOAD_S3M.C

 Description:
 Screamtracker (S3M) module loader

 Portability:
 All systems - all compilers (hopefully)

 If this module is found to not be portable to any particular platform,
 please contact Jake Stine at dracoirs@epix.net (see MIKMOD.TXT for
 more information on contacting the author).

*/

#include <string.h>
#include "mikmod.h"

/**************************************************************************
**************************************************************************/

typedef struct S3MNOTE
{   UBYTE note,ins,vol,cmd,inf;
} S3MNOTE;

typedef S3MNOTE S3MTRACK[64];


/* Raw S3M header struct: */

typedef struct S3MHEADER
{   CHAR  songname[28];
    UBYTE t1a;
    UBYTE type;
    UBYTE unused1[2];
    UWORD ordnum;
    UWORD insnum;
    UWORD patnum;
    UWORD flags;
    UWORD tracker;
    UWORD fileformat;
    CHAR  scrm[4];
    UBYTE mastervol;
    UBYTE initspeed;
    UBYTE inittempo;
    UBYTE mastermult;
    UBYTE ultraclick;
    UBYTE pantable;
    UBYTE unused2[8];
    UWORD special;
    UBYTE channels[32];
} S3MHEADER;


/* Raw S3M sampleinfo struct: */

typedef struct S3MSAMPLE
{   UBYTE type;
    CHAR  filename[12];
    UBYTE memsegh;
    UWORD memsegl;
    ULONG length;
    ULONG loopbeg;
    ULONG loopend;
    UBYTE volume;
    UBYTE dsk;
    UBYTE pack;
    UBYTE flags;
    ULONG c2spd;
    UBYTE unused[12];
    CHAR  sampname[28];
    CHAR  scrs[4];
} S3MSAMPLE;

/**************************************************************************
**************************************************************************/


extern UBYTE *poslookup;       /* S3M/IT fix - removing blank patterns needs a */
                               /* lookup table to fix position-jump commands */
extern SBYTE  remap[64];       /* for removing empty channels */

static S3MNOTE   *s3mbuf  = NULL; /* pointer to a complete S3M pattern */
static S3MHEADER *mh      = NULL;
static UWORD     *paraptr = NULL; /* parapointer array (see S3M docs) */

CHAR  S3M_Version[] = "Screamtracker 3.xx";

BOOL S3M_Test(void)
{
    UBYTE id[4];
    
    _mm_fseek(modfp,0x2c,SEEK_SET);
    if(!_mm_read_UBYTES(id,4,modfp)) return 0;
    if(!memcmp(id,"SCRM",4)) return 1;
    return 0;
}

BOOL S3M_Init(void)
{
    if(!(s3mbuf    = (S3MNOTE *)_mm_malloc(16*64*sizeof(S3MNOTE)))) return 0;
    if(!(mh        = (S3MHEADER *)_mm_calloc(1,sizeof(S3MHEADER)))) return 0;
    if(!(poslookup = (UBYTE *)_mm_malloc(sizeof(UBYTE)*128))) return 0;

    return 1;
}

void S3M_Cleanup(void)
{
    if(s3mbuf!=NULL) free(s3mbuf);
    if(paraptr!=NULL) free(paraptr);
    if(poslookup!=NULL) free(poslookup);
    if(mh!=NULL) free(mh);

    paraptr   = NULL;
    s3mbuf    = NULL;
    poslookup = NULL;
    mh        = NULL;
}


BOOL S3M_GetNumChannels(void)

/* Because so many s3m files have 16 channels as the set number used, but really */
/* only use far less (usually 8 to 12 still), I had to make this function, */
/* which determines the number of channels that are actually USED by a pattern. */
/* */
/* For every channel that's used, it sets the appropriate array entry of the */
/* global varialbe 'isused' */
/* */
/* NOTE: You must first seek to the file location of the pattern before calling */
/*       this procedure. */
/* Returns 1 on fail. */

{
    int row=0,flag,ch;

    while(row<64)
    {   flag = _mm_read_UBYTE(modfp);

        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_PATTERN;
            return 1;
        }

        if(flag)
        {   ch = flag&31;
            if(mh->channels[ch] < 16) remap[ch] = 0;
            
            if(flag&32)
            {   _mm_read_UBYTE(modfp);
                _mm_read_UBYTE(modfp);
            }

            if(flag&64)
                _mm_read_UBYTE(modfp);

            if(flag&128)
            {   _mm_read_UBYTE(modfp);
                _mm_read_UBYTE(modfp);
            }
        } else row++;
    }

    return 0;
}    


BOOL S3M_ReadPattern(void)
{
    int row=0,flag,ch;
    S3MNOTE *n;
    S3MNOTE dummy;

    /* clear pattern data */
    memset(s3mbuf,255,16*64*sizeof(S3MNOTE));

    while(row<64)
    {   flag = _mm_read_UBYTE(modfp);

        if(flag==EOF)
        {   _mm_errno = MMERR_LOADING_PATTERN;
            return 0;
        }

        if(flag)
        {   ch = remap[flag&31];

            if(ch != -1)
                n = &s3mbuf[(64U*ch)+row];
            else
                n = &dummy;

            if(flag&32)
            {   n->note = _mm_read_UBYTE(modfp);
                n->ins  = _mm_read_UBYTE(modfp);
            }

            if(flag&64)
                n->vol = _mm_read_UBYTE(modfp);

            if(flag&128)
            {   n->cmd = _mm_read_UBYTE(modfp);
                n->inf = _mm_read_UBYTE(modfp);
            }
        } else row++;
    }
    return 1;
}


void S3MIT_ProcessCmd(UBYTE cmd, UBYTE inf, BOOL oldeffect);

UBYTE *S3M_ConvertTrack(S3MNOTE *tr)
{
    int t;

    UBYTE note,ins,vol;

    UniReset();
    for(t=0; t<64; t++)
    {
        note = tr[t].note;
        ins  = tr[t].ins;
        vol  = tr[t].vol;


        if(ins!=0 && ins!=255) UniInstrument(ins-1);
        if(note!=255)
        {   if(note==254) UniPTEffect(0xc,0);             /* <- note off command */
            else UniNote(((note>>4)*12)+(note&0xf));      /* <- normal note */
        }

        if(vol<255)
            UniPTEffect(0xc,vol);

        S3MIT_ProcessCmd(tr[t].cmd,tr[t].inf,1);
        UniNewline();
    }

    return UniDup();
}


BOOL S3M_Load(void)
{
    int    t,u,track = 0;
    SAMPLE *q;
    UBYTE  pan[32];

    /* try to read module header */

    _mm_read_string(mh->songname,28,modfp);
    mh->t1a         =_mm_read_UBYTE(modfp);
    mh->type        =_mm_read_UBYTE(modfp);
    _mm_read_UBYTES(mh->unused1,2,modfp);
    mh->ordnum      =_mm_read_I_UWORD(modfp);
    mh->insnum      =_mm_read_I_UWORD(modfp);
    mh->patnum      =_mm_read_I_UWORD(modfp);
    mh->flags       =_mm_read_I_UWORD(modfp);
    mh->tracker     =_mm_read_I_UWORD(modfp);
    mh->fileformat  =_mm_read_I_UWORD(modfp);
    _mm_read_string(mh->scrm,4,modfp);

    mh->mastervol   =_mm_read_UBYTE(modfp);
    mh->initspeed   =_mm_read_UBYTE(modfp);
    mh->inittempo   =_mm_read_UBYTE(modfp);
    mh->mastermult  =_mm_read_UBYTE(modfp);
    mh->ultraclick  =_mm_read_UBYTE(modfp);
    mh->pantable    =_mm_read_UBYTE(modfp);
    _mm_read_UBYTES(mh->unused2,8,modfp);
    mh->special     =_mm_read_I_UWORD(modfp);
    _mm_read_UBYTES(mh->channels,32,modfp);

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    /* set module variables */

    of.modtype     = strdup(S3M_Version);
    of.modtype[14] = ((mh->tracker >> 8) &0xf) + 0x30;
    of.modtype[16] = ((mh->tracker >> 4)&0xf) + 0x30;
    of.modtype[17] = ((mh->tracker)&0xf) + 0x30;
    of.songname    = DupStr(mh->songname,28);
    of.numpat      = mh->patnum;
    of.reppos      = 0;
    of.numins      = of.numsmp = mh->insnum;
    of.initspeed   = mh->initspeed;
    of.inittempo   = mh->inittempo;
    of.initvolume  = mh->mastervol<<1;

    /* read the order data */
    if(!AllocPositions(mh->ordnum)) return 0;
    for(t=0; t<mh->ordnum; t++)
        of.positions[t] = _mm_read_UBYTE(modfp);

    of.numpos = 0;
    for(t=0; t<mh->ordnum; t++)
    {   of.positions[of.numpos] = of.positions[t];
        poslookup[t]            = of.numpos;   /* bug fix for FREAKY S3Ms */
        if(of.positions[t]<254) of.numpos++;        
    }

    if((paraptr=(UWORD *)_mm_malloc((of.numins+of.numpat)*sizeof(UWORD)))==NULL) return 0;

    /* read the instrument+pattern parapointers */
    _mm_read_I_UWORDS(paraptr,of.numins+of.numpat,modfp);


    if(mh->pantable==252)
    {   /* read the panning table (ST 3.2 addition.  See below for further */
        /* portions of channel panning [past reampper]). */
        _mm_read_UBYTES(pan,32,modfp);
    }


    /* now is a good time to check if the header was too short :) */

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }


    /* ============================================== */
    /* Load those darned Samples!  (no insts in ST3) */

    if(!AllocSamples()) return 0;

    q = of.samples;

    for(t=0; t<of.numins; t++)
    {   S3MSAMPLE s;

        /* seek to instrument position */

        _mm_fseek(modfp,((long)paraptr[t])<<4,SEEK_SET);

        /* and load sample info */

        s.type      =_mm_read_UBYTE(modfp);
        _mm_read_string(s.filename,12,modfp);
        s.memsegh   =_mm_read_UBYTE(modfp);
        s.memsegl   =_mm_read_I_UWORD(modfp);
        s.length    =_mm_read_I_ULONG(modfp);
        s.loopbeg   =_mm_read_I_ULONG(modfp);
        s.loopend   =_mm_read_I_ULONG(modfp);
        s.volume    =_mm_read_UBYTE(modfp);
        s.dsk       =_mm_read_UBYTE(modfp);
        s.pack      =_mm_read_UBYTE(modfp);
        s.flags     =_mm_read_UBYTE(modfp);
        s.c2spd     =_mm_read_I_ULONG(modfp);
        _mm_read_UBYTES(s.unused,12,modfp);
        _mm_read_string(s.sampname,28,modfp);
        _mm_read_string(s.scrs,4,modfp);

        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_SAMPLEINFO;
            return 0;
        }

        q->samplename = DupStr(s.sampname,28);
        q->speed      = s.c2spd;
        q->length     = s.length;
        q->loopstart  = s.loopbeg;
        q->loopend    = s.loopend;
        q->volume     = s.volume;
        q->seekpos    = (((long)s.memsegh)<<16|s.memsegl)<<4;

        if(s.flags&1) q->flags |= SF_LOOP;
        if(s.flags&4) q->flags |= SF_16BITS;
        if(mh->fileformat==1) q->flags |= SF_SIGNED;

        /* DON'T load sample if it doesn't have the SCRS tag */
        if(memcmp(s.scrs,"SCRS",4)!=0) q->length = 0;

        q++;
    }

    /* ==================================== */
    /* Determine the number of channels actually used.  (what ever happened */
    /* to the concept of a single "numchn" variable, eh?! */

    of.numchn = 0;
    memset(remap,-1,32*sizeof(UBYTE));

    for(t=0; t<of.numpat; t++)
    {   /* seek to pattern position ( + 2 skip pattern length ) */
        _mm_fseek(modfp,(long)((paraptr[of.numins+t])<<4)+2,SEEK_SET);
        if(S3M_GetNumChannels()) return 0;
    }
    
    /* build the remap array  */
    for(t=0; t<32; t++)
    {   if(remap[t]==0)
        {   remap[t] = of.numchn;
            of.numchn++;
        }
    }

    /* ============================================================ */
    /* set panning positions AFTER building remap chart! */

    for(t=0; t<32; t++)
    {   if((mh->channels[t]<16) && (remap[t]!=-1))
        {   if(mh->channels[t]<8)
                of.panning[remap[t]] = 0x20;     /* 0x30 = std s3m val */
            else
                of.panning[remap[t]] = 0xd0;     /* 0xc0 = std s3m val */
        }
    }

    if(mh->pantable==252)
    {   /* set panning positions according to panning table (new for st3.2) */
        for(t=0; t<32; t++)
        {   if((pan[t]&0x20) && (mh->channels[t]<16) && (remap[t]!=-1))
                of.panning[remap[t]] = (pan[t]&0xf)<<4;
        }
    }


    /* ============================== */
    /* Load the pattern info now! */
    
    of.numtrk = of.numpat*of.numchn;
    if(!AllocTracks()) return 0;
    if(!AllocPatterns()) return 0;

    for(t=0; t<of.numpat; t++)
    {   /* seek to pattern position ( + 2 skip pattern length ) */
        _mm_fseek(modfp,(((long)paraptr[of.numins+t])<<4)+2,SEEK_SET);
        if(!S3M_ReadPattern()) return 0;
        for(u=0; u<of.numchn; u++)
            if(!(of.tracks[track++]=S3M_ConvertTrack(&s3mbuf[u*64]))) return 0;
    }

    return 1;
}

         
CHAR *S3M_LoadTitle(void)
{
   CHAR s[28];

   _mm_fseek(modfp,0,SEEK_SET);
   if(!fread(s,28,1,modfp)) return NULL;
   
   return(DupStr(s,28));
}


MLOADER load_s3m =
{   NULL,
    "S3M",
    "S3M loader v0.3",
    S3M_Init,
    S3M_Test,
    S3M_Load,
    S3M_Cleanup,

    S3M_LoadTitle
};

