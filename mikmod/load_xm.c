/*

 Name: LOAD_XM.C

 Description:
 Fasttracker (XM) module loader

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


typedef struct XMHEADER
{   CHAR  id[17];                   /* ID text: 'Extended module: ' */
    CHAR  songname[21];             /* Module name, padded with zeroes and 0x1a at the end */
    CHAR  trackername[20];          /* Tracker name */
    UWORD version;                  /* (word) Version number, hi-byte major and low-byte minor */
    ULONG headersize;               /* Header size */
    UWORD songlength;               /* (word) Song length (in patten order table) */
    UWORD restart;                  /* (word) Restart position */
    UWORD numchn;                   /* (word) Number of channels (2,4,6,8,10,...,32) */
    UWORD numpat;                   /* (word) Number of patterns (max 256) */
    UWORD numins;                   /* (word) Number of instruments (max 128) */
    UWORD flags;                    /* (word) Flags: bit 0: 0 = Amiga frequency table (see below) 1 = Linear frequency table */
    UWORD tempo;                    /* (word) Default tempo */
    UWORD bpm;                      /* (word) Default BPM */
    UBYTE orders[256];              /* (byte) Pattern order table  */
} XMHEADER;


typedef struct XMINSTHEADER
{   ULONG size;                     /* (dword) Instrument size */
    CHAR  name[22];                 /* (char) Instrument name */
    UBYTE type;                     /* (byte) Instrument type (always 0) */
    UWORD numsmp;                   /* (word) Number of samples in instrument */
    ULONG ssize;                    /* */
} XMINSTHEADER;


typedef struct XMPATCHHEADER
{   UBYTE what[96];         /* (byte) Sample number for all notes */
    UWORD volenv[24];       /* (byte) Points for volume envelope */
    UWORD panenv[24];       /* (byte) Points for panning envelope */
    UBYTE volpts;           /* (byte) Number of volume points */
    UBYTE panpts;           /* (byte) Number of panning points */
    UBYTE volsus;           /* (byte) Volume sustain point */
    UBYTE volbeg;           /* (byte) Volume loop start point */
    UBYTE volend;           /* (byte) Volume loop end point */
    UBYTE pansus;           /* (byte) Panning sustain point */
    UBYTE panbeg;           /* (byte) Panning loop start point */
    UBYTE panend;           /* (byte) Panning loop end point */
    UBYTE volflg;           /* (byte) Volume type: bit 0: On; 1: Sustain; 2: Loop */
    UBYTE panflg;           /* (byte) Panning type: bit 0: On; 1: Sustain; 2: Loop */
    UBYTE vibflg;           /* (byte) Vibrato type */
    UBYTE vibsweep;         /* (byte) Vibrato sweep */
    UBYTE vibdepth;         /* (byte) Vibrato depth */
    UBYTE vibrate;          /* (byte) Vibrato rate */
    UWORD volfade;          /* (word) Volume fadeout */
    UWORD reserved[11];     /* (word) Reserved */
} XMPATCHHEADER;


typedef struct XMWAVHEADER
{   ULONG length;           /* (dword) Sample length */
    ULONG loopstart;        /* (dword) Sample loop start */
    ULONG looplength;       /* (dword) Sample loop length */
    UBYTE volume;           /* (byte) Volume  */
    SBYTE finetune;         /* (byte) Finetune (signed byte -128..+127) */
    UBYTE type;             /* (byte) Type: Bit 0-1: 0 = No loop, 1 = Forward loop, */
                            /*                       2 = Ping-pong loop; */
                            /*                    4: 16-bit sampledata */
    UBYTE panning;          /* (byte) Panning (0-255) */
    SBYTE relnote;          /* (byte) Relative note number (signed byte) */
    UBYTE reserved;         /* (byte) Reserved */
    CHAR  samplename[22];   /* (char) Sample name */

    UBYTE vibtype;          /* (byte) Vibrato type */
    UBYTE vibsweep;         /* (byte) Vibrato sweep */
    UBYTE vibdepth;         /* (byte) Vibrato depth */
    UBYTE vibrate;          /* (byte) Vibrato rate */
} XMWAVHEADER;


typedef struct XMPATHEADE
{   ULONG size;                     /* (dword) Pattern header length  */
    UBYTE packing;                  /* (byte) Packing type (always 0) */
    UWORD numrows;                  /* (word) Number of rows in pattern (1..256) */
    UWORD packsize;                 /* (word) Packed patterndata size */
} XMPATHEADER;

typedef struct MTMNOTE
{    UBYTE a,b,c;
} MTMNOTE;


typedef struct XMNOTE
{    UBYTE note,ins,vol,eff,dat;
}XMNOTE;

/**************************************************************************
**************************************************************************/

static XMNOTE *xmpat = NULL;
static XMHEADER *mh = NULL;

BOOL XM_Test(void)
{
    UBYTE id[17];
    
    if(!_mm_read_UBYTES(id,17,modfp)) return 0;
    if(!memcmp(id,"Extended Module: ",17)) return 1;
    return 0;
}


BOOL XM_Init(void)
{
    if(!(mh=(XMHEADER *)_mm_calloc(1,sizeof(XMHEADER)))) return 0;
    return 1;
}


void XM_Cleanup(void)
{
    if(mh!=NULL) free(mh);
    mh = NULL;
}


void XM_ReadNote(XMNOTE *n)
{
    UBYTE cmp;

    memset(n,0,sizeof(XMNOTE));

    cmp = _mm_read_UBYTE(modfp);

    if(cmp&0x80)
    {   if(cmp&1)  n->note = _mm_read_UBYTE(modfp);
        if(cmp&2)  n->ins  = _mm_read_UBYTE(modfp);
        if(cmp&4)  n->vol  = _mm_read_UBYTE(modfp);
        if(cmp&8)  n->eff  = _mm_read_UBYTE(modfp);
        if(cmp&16) n->dat  = _mm_read_UBYTE(modfp);
    }
    else
    {   n->note = cmp;
        n->ins  = _mm_read_UBYTE(modfp);
        n->vol  = _mm_read_UBYTE(modfp);
        n->eff  = _mm_read_UBYTE(modfp);
        n->dat  = _mm_read_UBYTE(modfp);
    }
}


UBYTE *XM_Convert(XMNOTE *xmtrack,UWORD rows)
{
    int t;
    UBYTE note,ins,vol,eff,dat;

    UniReset();

    for(t=0; t<rows; t++)
    {   note = xmtrack->note;
        ins  = xmtrack->ins;
        vol  = xmtrack->vol;
        eff  = xmtrack->eff;
        dat  = xmtrack->dat;

        if(note!=0)
        {  if(note==97) 
           {   UniWrite(UNI_KEYFADE);
               UniWrite(0);
           } else
               UniNote(note-1);
        }

        if(ins!=0) UniInstrument(ins-1);

        switch(vol>>4)
        {
            case 0x6:                   /* volslide down */
                if(vol&0xf)
                {   UniWrite(UNI_XMEFFECTA);
                    UniWrite(vol&0xf);
                }
                break;

            case 0x7:                   /* volslide up */
                if(vol&0xf)
                {   UniWrite(UNI_XMEFFECTA);
                    UniWrite(vol<<4);
                }
                break;

            /* volume-row fine volume slide is compatible with protracker */
            /* EBx and EAx effects i.e. a zero nibble means DO NOT SLIDE, as */
            /* opposed to 'take the last sliding value'. */

            case 0x8:                       /* finevol down */
                UniPTEffect(0xe,0xb0 | (vol&0xf));
                break;

            case 0x9:                       /* finevol up */
                UniPTEffect(0xe,0xa0 | (vol&0xf));
                break;

            case 0xa:                       /* set vibrato speed */
                UniPTEffect(0x4,vol<<4);
                break;

            case 0xb:                       /* vibrato */
                UniPTEffect(0x4,vol&0xf);
                break;

            case 0xc:                       /* set panning */
                UniPTEffect(0x8,vol<<4);
                break;

            case 0xd:                       /* panning slide left */
                /* only slide when data nibble not zero: */

                if(vol&0xf)
                {   UniWrite(UNI_XMEFFECTP);
                    UniWrite(vol&0xf);
                }
                break;

            case 0xe:                       /* panning slide right */
                /* only slide when data nibble not zero: */

                if(vol&0xf)
                {   UniWrite(UNI_XMEFFECTP);
                    UniWrite(vol<<4);
                }
             break;

            case 0xf:                       /* tone porta */
                UniPTEffect(0x3,vol<<4);
            break;

            default:
                if(vol>=0x10 && vol<=0x50)
                    UniPTEffect(0xc,vol-0x10);
        }

        switch(eff)
        {
            case 0x4:                       /* Effect 4: Vibrato */
                UniWrite(UNI_XMEFFECT4);
                UniWrite(dat);
            break;

            case 0xa:
                UniWrite(UNI_XMEFFECTA);
                UniWrite(dat);
            break;

            case 0xe:
                switch(dat>>4)
                {  case 0x1:      /* XM fine porta up */
                      UniWrite(UNI_XMEFFECTE1);
                      UniWrite(dat&0xf);
                   break;

                   case 0x2:      /* XM fine porta down */
                      UniWrite(UNI_XMEFFECTE2);
                      UniWrite(dat&0xf);
                   break;

                   case 0xa:      /* XM fine volume up */
                      UniWrite(UNI_XMEFFECTEA);
                      UniWrite(dat&0xf);
                   break;

                   case 0xb:      /* XM fine volume down */
                      UniWrite(UNI_XMEFFECTEB);
                      UniWrite(dat&0xf);
                   break;

                   default:
                      UniPTEffect(0x0e,dat);
                }
            break;

            case 'G'-55:                    /* G - set global volume */
                if(dat>64) dat = 64;
                UniWrite(UNI_XMEFFECTG);
                UniWrite(dat);
                break;

            case 'H'-55:                    /* H - global volume slide */
                UniWrite(UNI_XMEFFECTH);
                UniWrite(dat);
                break;

            case 'K'-55:                    /* K - keyOff and KeyFade */
                UniWrite(UNI_KEYFADE);
                UniWrite(dat);
                break;

            case 'L'-55:                    /* L - set envelope position */
                UniWrite(UNI_XMEFFECTL);
                UniWrite(dat);
                break;

            case 'P'-55:                    /* P - panning slide */
                UniWrite(UNI_XMEFFECTP);
                UniWrite(dat);
                break;

            case 'R'-55:                    /* R - multi retrig note */
                UniWrite(UNI_S3MEFFECTQ);
                UniWrite(dat);
                break;

            case 'T'-55:                    /* T - Tremor !! (== S3M effect I) */
                UniWrite(UNI_S3MEFFECTI);
                UniWrite(dat);
                break;

            case 'X'-55:
                if((dat>>4) == 1)           /* X1 - Extra Fine Porta up */
                {  UniWrite(UNI_XMEFFECTX1);
                   UniWrite(dat & 0xf);
                } else if((dat>>4) == 2)    /* X2 - Extra Fine Porta down */
                {  UniWrite(UNI_XMEFFECTX2);
                   UniWrite(dat & 0xf);
                }
            break;

            default:
                if(eff <= 0xf)
                {   /* Convert pattern jump from Dec to Hex */
                    if(eff == 0xd)
                        dat = (((dat&0xf0)>>4)*10)+(dat&0xf);
                    UniPTEffect(eff,dat);
                }
            break;
        }

        UniNewline();
        xmtrack++;
    }
    return UniDup();
}



BOOL XM_Load(void)
{
    INSTRUMENT *d;
    SAMPLE     *q;
    XMWAVHEADER *wh,*s;
    int   t,u,v,p,numtrk;
    long  next;
    ULONG nextwav[256];
    BOOL  dummypat=0;
 
    /* try to read module header */

    _mm_read_string(mh->id,17,modfp);
    _mm_read_string(mh->songname,21,modfp);
    _mm_read_string(mh->trackername,20,modfp);
    mh->version     =_mm_read_I_UWORD(modfp);
    mh->headersize  =_mm_read_I_ULONG(modfp);
    mh->songlength  =_mm_read_I_UWORD(modfp);
    mh->restart     =_mm_read_I_UWORD(modfp);
    mh->numchn      =_mm_read_I_UWORD(modfp);
    mh->numpat      =_mm_read_I_UWORD(modfp);
    mh->numins      =_mm_read_I_UWORD(modfp);
    mh->flags       =_mm_read_I_UWORD(modfp);
    mh->tempo       =_mm_read_I_UWORD(modfp);
    mh->bpm         =_mm_read_I_UWORD(modfp);
    _mm_read_UBYTES(mh->orders,256,modfp);

    if(feof(modfp))
    {   _mm_errno = MMERR_LOADING_HEADER;
        return 0;
    }

    /* set module variables */
    of.initspeed = mh->tempo;         
    of.inittempo = mh->bpm;
    of.modtype   = DupStr(mh->trackername,20);
    of.numchn    = mh->numchn;
    of.numpat    = mh->numpat;
    of.numtrk    = (UWORD)of.numpat*of.numchn;   /* get number of channels */
    of.songname  = DupStr(mh->songname,20);      /* make a cstr of songname */
    of.numpos    = mh->songlength;               /* copy the songlength */
    of.reppos    = mh->restart;
    of.numins    = mh->numins;
    of.flags |= UF_XMPERIODS | UF_INST;
    if(mh->flags&1) of.flags |= UF_LINEAR;

    memset(of.chanvol,64,of.numchn);             /* store channel volumes */

    if(!AllocPositions(of.numpos+3)) return 0;
    for(t=0; t<of.numpos; t++)
        of.positions[t] = mh->orders[t];

/*
   WHY THIS CODE HERE?? I CAN'T REMEMBER!
   
   Well, I do know why, mikmak!  Seems that FT2 doesn't always count blank 
   patterns AT ALL if they are at the END of the song.  So, we have to check
   for any patter numbers in the order list greater than the number of pat-
   terns total.  If one or more is found, we set it equal to the pattern total
   and make a dummy pattern to accomidate for the discrepency!
*/

    for(t=0; t<of.numpos; t++)
    {   if(of.positions[t] > of.numpat)
        {  of.positions[t] = of.numpat;
           dummypat = 1;
        }
    }      

    if(dummypat) { of.numpat++; of.numtrk+=of.numchn; }

    if(!AllocTracks()) return 0;
    if(!AllocPatterns()) return 0;

    numtrk = 0;
    for(t=0; t<mh->numpat; t++)
    {   XMPATHEADER ph;

        ph.size     =_mm_read_I_ULONG(modfp);
        ph.packing  =_mm_read_UBYTE(modfp);
        ph.numrows  =_mm_read_I_UWORD(modfp);
        ph.packsize =_mm_read_I_UWORD(modfp);

        of.pattrows[t] = ph.numrows;

        /*  Gr8.. when packsize is 0, don't try to load a pattern.. it's empty. */
        /*  This bug was discovered thanks to Khyron's module.. */

        if(!(xmpat=(XMNOTE *)_mm_calloc(ph.numrows*of.numchn,sizeof(XMNOTE)))) return 0;

        if(ph.packsize>0)
        {   for(u=0; u<ph.numrows; u++)
            {   for(v=0; v<of.numchn; v++)
                    XM_ReadNote(&xmpat[(v*ph.numrows)+u]);
            }
        }

        if(feof(modfp))
        {   _mm_errno = MMERR_LOADING_PATTERN;
            return 0;
        }

        for(v=0; v<of.numchn; v++)                                           
           of.tracks[numtrk++] = XM_Convert(&xmpat[v*ph.numrows],ph.numrows);

        free(xmpat);
    }
                                                          
    if(dummypat)
    {  of.pattrows[t] = 64;
       if(!(xmpat=(XMNOTE *)_mm_calloc(64*of.numchn,sizeof(XMNOTE)))) return 0;
       for(v=0; v<of.numchn; v++)
           of.tracks[numtrk++] = XM_Convert(&xmpat[v*64],64);
       free(xmpat);
    }

    if(!AllocInstruments()) return 0;
    if((wh = (XMWAVHEADER *)_mm_calloc(256,sizeof(XMWAVHEADER))) == NULL) return 0;
    d = of.instruments;
    s = wh;


    for(t=0; t<of.numins; t++)
    {   XMINSTHEADER ih;
        int          headend;

        memset(d->samplenumber,255,120);

        /* read instrument header */

        headend     = _mm_ftell(modfp);
        ih.size     = _mm_read_I_ULONG(modfp);
        headend    += ih.size;
        _mm_read_string(ih.name, 22, modfp);
        ih.type     = _mm_read_UBYTE(modfp);
        ih.numsmp   = _mm_read_I_UWORD(modfp);
        d->insname  = DupStr(ih.name,22);

        if(ih.size > 29)
        {   ih.ssize    = _mm_read_I_ULONG(modfp);
            if(ih.numsmp > 0)
            {   XMPATCHHEADER pth;
    
                _mm_read_UBYTES (pth.what, 96, modfp);
                _mm_read_I_UWORDS (pth.volenv, 24, modfp);
                _mm_read_I_UWORDS (pth.panenv, 24, modfp);
                pth.volpts      =  _mm_read_UBYTE(modfp);
                pth.panpts      =  _mm_read_UBYTE(modfp);
                pth.volsus      =  _mm_read_UBYTE(modfp);
                pth.volbeg      =  _mm_read_UBYTE(modfp);
                pth.volend      =  _mm_read_UBYTE(modfp);
                pth.pansus      =  _mm_read_UBYTE(modfp);
                pth.panbeg      =  _mm_read_UBYTE(modfp);
                pth.panend      =  _mm_read_UBYTE(modfp);
                pth.volflg      =  _mm_read_UBYTE(modfp);
                pth.panflg      =  _mm_read_UBYTE(modfp);
                pth.vibflg      =  _mm_read_UBYTE(modfp);
                pth.vibsweep    =  _mm_read_UBYTE(modfp);
                pth.vibdepth    =  _mm_read_UBYTE(modfp);
                pth.vibrate     =  _mm_read_UBYTE(modfp);
                pth.volfade     =  _mm_read_I_UWORD(modfp);
    
                /* read the remainder of the header */
                for(u=headend-_mm_ftell(modfp); u; u--)  _mm_read_UBYTE(modfp);
    
                if(feof(modfp))
                {   _mm_errno = MMERR_LOADING_SAMPLEINFO;
                    return 0;
                }
    
                for(u=0; u<96; u++)         
                   d->samplenumber[u] = pth.what[u] + of.numsmp;
    
                d->volfade = pth.volfade;
    
                memcpy(d->volenv,pth.volenv,24);
                if(pth.volflg & 1)  d->volflg |= EF_ON;
                if(pth.volflg & 2)  d->volflg |= EF_SUSTAIN;
                if(pth.volflg & 4)  d->volflg |= EF_LOOP;
                d->volsusbeg = d->volsusend = pth.volsus;
                d->volbeg    = pth.volbeg;
                d->volend    = pth.volend;
                d->volpts    = pth.volpts;
    
                /* scale volume envelope: */
    
                for(p=0; p<12; p++)
                    d->volenv[p].val <<= 2;
    
                if((d->volflg & EF_ON) && (d->volpts < 2))
                    d->volflg &= ~EF_ON;
    
                memcpy(d->panenv,pth.panenv,24);
                d->panflg    = pth.panflg;
                d->pansusbeg = d->pansusend = pth.pansus;
                d->panbeg    = pth.panbeg;
                d->panend    = pth.panend;
                d->panpts    = pth.panpts;
    
                /* scale panning envelope: */
    
                for(p=0; p<12; p++)
                    d->panenv[p].val <<= 2;
                if((d->panflg & EF_ON) && (d->panpts < 2))
                    d->panflg &= ~EF_ON;
    
                next = 0;
    
                /*  Samples are stored outside the instrument struct now, so we have */
                /*  to load them all into a temp area, count the of.numsmp along the */
                /*  way and then do an AllocSamples() and move everything over  */
    
                for(u=0; u<ih.numsmp; u++,s++)
                {   s->length       =_mm_read_I_ULONG (modfp);
                    s->loopstart    =_mm_read_I_ULONG (modfp);
                    s->looplength   =_mm_read_I_ULONG (modfp);
                    s->volume       =_mm_read_UBYTE (modfp);
                    s->finetune     =_mm_read_SBYTE (modfp);
                    s->type         =_mm_read_UBYTE (modfp);
                    s->panning      =_mm_read_UBYTE (modfp);
                    s->relnote      =_mm_read_SBYTE (modfp);
                    s->vibtype      = pth.vibflg;
                    s->vibsweep     = pth.vibsweep;
                    s->vibdepth     = pth.vibdepth*4;
                    s->vibrate      = pth.vibrate;
    
                    s->reserved =_mm_read_UBYTE (modfp);
                    _mm_read_string(s->samplename, 22, modfp);
    
                    nextwav[of.numsmp+u] = next;
                    next += s->length;
    
                    if(feof(modfp))
                    {   _mm_errno = MMERR_LOADING_SAMPLEINFO;
                        return 0;
                    }
                }
    
                for(u=0; u<ih.numsmp; u++) nextwav[of.numsmp++] += _mm_ftell(modfp);
                _mm_fseek(modfp,next,SEEK_CUR);
            }
        }

        d++;
    }

    if(!AllocSamples()) return 0;
    q = of.samples;
    s = wh;

    for(u=0; u<of.numsmp; u++,q++,s++)
    {   q->samplename   = DupStr(s->samplename,22);
        q->length       = s->length;
        q->loopstart    = s->loopstart;
        q->loopend      = s->loopstart+s->looplength;
        q->volume       = s->volume;
        q->speed        = s->finetune+128;
        q->panning      = s->panning;
        q->seekpos      = nextwav[u];
        q->vibtype      = s->vibtype;
        q->vibsweep     = s->vibsweep;
        q->vibdepth     = s->vibdepth;
        q->vibrate      = s->vibrate;

        if(s->type & 0x10)
        {   q->length    >>= 1;
            q->loopstart >>= 1;
            q->loopend   >>= 1;
        }

        q->flags|=SF_OWNPAN;
        if(s->type&0x3) q->flags|=SF_LOOP;
        if(s->type&0x2) q->flags|=SF_BIDI;

        if(s->type&0x10) q->flags|=SF_16BITS;
        q->flags|=SF_DELTA;
        q->flags|=SF_SIGNED;
    }

    d = of.instruments;
    s = wh;
    for(u=0; u<of.numins; u++, d++)
    {  /*for(t=0; t<3; t++)
          if((s[d->samplenumber[t]].relnote / 12) > )
          {  s[d->samplenumber[t]].relnote -= 12;
             of.samples[d->samplenumber[t]].speed <<= 1;
          }
       */
       for(t=0; t<96; t++)
          d->samplenote[t] = (d->samplenumber[t]==of.numsmp) ? 255 : (t+s[d->samplenumber[t]].relnote);
    }

    free(wh);
    return 1;
}



CHAR *XM_LoadTitle(void)
{
    CHAR s[21];

    _mm_fseek(modfp,17,SEEK_SET);
    if(!fread(s,21,1,modfp)) return NULL;
  
    return(DupStr(s,21));
}



MLOADER load_xm =
{   NULL,
    "XM",
    "Portable XM loader v0.5",
    XM_Init,
    XM_Test,
    XM_Load,
    XM_Cleanup,
    XM_LoadTitle
};
