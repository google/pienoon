/*
  --> The Protracker Enums
   -> For MikMod 3.0
*/

#ifndef _PTFORM_H_
#define _PTFORM_H_

#ifdef __cplusplus
extern "C" {
#endif


extern UWORD  mytab[12],logtab[104];
extern UBYTE  VibratoTable[32],avibtab[128];
extern SBYTE  PanbrelloTable[256];
extern ULONG  lintab[768];


/**************************************************************************
****** Unitrack stuff: ****************************************************
**************************************************************************/

/* The UniTrack stuff is generally for internal use only, but would be */
/* required in making a tracker or a module player tha scrolls pattern */
/* data. */

/* prototypes: */

extern void    UniSetRow(UBYTE *t);
extern UBYTE   UniGetByte(void);
extern UBYTE  *UniFindRow(UBYTE *t,UWORD row);
extern void    UniReset(void);
extern void    UniWrite(UBYTE data);
extern void    UniNewline(void);
extern UBYTE  *UniDup(void);
extern void    UniSkipOpcode(UBYTE op);
extern BOOL    UniInit(void);
extern void    UniCleanup(void);
extern UWORD   TrkLen(UBYTE *t);
extern BOOL    MyCmp(UBYTE *a, UBYTE *b, UWORD l);
extern void    UniInstrument(UBYTE ins);
extern void    UniNote(UBYTE note);
extern void    UniPTEffect(UBYTE eff, UBYTE dat);
extern void    UniVolEffect(UWORD eff, UBYTE dat);


/* Enumaerated UniMod Commands */

enum
{   UNI_NOTE = 1,
    UNI_INSTRUMENT,
    UNI_PTEFFECT0,
    UNI_PTEFFECT1,
    UNI_PTEFFECT2,
    UNI_PTEFFECT3,
    UNI_PTEFFECT4,
    UNI_PTEFFECT5,
    UNI_PTEFFECT6,
    UNI_PTEFFECT7,
    UNI_PTEFFECT8,
    UNI_PTEFFECT9,
    UNI_PTEFFECTA,
    UNI_PTEFFECTB,
    UNI_PTEFFECTC,
    UNI_PTEFFECTD,
    UNI_PTEFFECTE,
    UNI_PTEFFECTF,
    UNI_S3MEFFECTA,
    UNI_S3MEFFECTD,
    UNI_S3MEFFECTE,
    UNI_S3MEFFECTF,
    UNI_S3MEFFECTI,
    UNI_S3MEFFECTQ,
    UNI_S3MEFFECTR,
    UNI_S3MEFFECTT,
    UNI_S3MEFFECTU, 
    UNI_KEYOFF,
    UNI_KEYFADE,
    UNI_VOLEFFECTS,
    UNI_XMEFFECT4,
    UNI_XMEFFECTA,
    UNI_XMEFFECTE1,
    UNI_XMEFFECTE2,
    UNI_XMEFFECTEA,
    UNI_XMEFFECTEB,
    UNI_XMEFFECTG,
    UNI_XMEFFECTH,
    UNI_XMEFFECTL,
    UNI_XMEFFECTP,
    UNI_XMEFFECTX1,
    UNI_XMEFFECTX2,
    UNI_ITEFFECTG,     /* Porta to Note .. no kick=0; */
    UNI_ITEFFECTH,     /* IT specific Vibrato */
    UNI_ITEFFECTI,     /* IT tremor (xy not incremeneted) */
    UNI_ITEFFECTM,     /* Set Channel Volume */
    UNI_ITEFFECTN,     /* Slide / Fineslide Channel Volume */
    UNI_ITEFFECTP,     /* Slide / Fineslide Channel Panning */
    UNI_ITEFFECTU,     /* IT fine vibrato */
    UNI_ITEFFECTW,     /* Slide / Fineslide Global volume */
    UNI_ITEFFECTY,     /* The Satanic Panbrello */
    UNI_ITEFFECTS0,
    UNI_LAST
};


/* IT / S3M Extended SS effects: */

enum
{   SS_GLISSANDO = 1,
    SS_FINETUNE,
    SS_VIBWAVE,
    SS_TREMWAVE,
    SS_PANWAVE,
    SS_FRAMEDELAY,
    SS_S7EFFECTS,
    SS_PANNING,
    SS_SURROUND,
    SS_HIOFFSET,
    SS_PATLOOP,
    SS_NOTECUT,
    SS_NOTEDELAY,
    SS_PATDELAY
};


/* IT Volume column effects */

enum
{   VOL_VOLUME = 1,
    VOL_PANNING,
    VOL_VOLSLIDE,
    VOL_PITCHSLIDEDN,
    VOL_PITCHSLIDEUP,
    VOL_PORTAMENTO,
    VOL_VIBRATO
};


/**************************************************************************
****** Instrument stuff: **************************************************
**************************************************************************/


/* Instrument format flags */
#define IF_OWNPAN       1
#define IF_PITCHPAN     2

/* Envelope flags: */

#define EF_ON           1
#define EF_SUSTAIN      2
#define EF_LOOP         4
#define EF_VOLENV       8

/* New Note Action Flags */

#define NNA_CUT         0
#define NNA_CONTINUE    1
#define NNA_OFF         2
#define NNA_FADE        3

#define DCT_OFF         0
#define DCT_NOTE        1                         
#define DCT_SAMPLE      2                         
#define DCT_INST        3           

#define DCA_CUT         0
#define DCA_OFF         1
#define DCA_FADE        2

#define KEY_KICK        0
#define KEY_OFF         1
#define KEY_FADE        2
#define KEY_KILL        3

#define AV_IT           1   /* IT vs. XM vibrato info */


typedef struct ENVPT
{   SWORD pos;
    SWORD val;
} ENVPT;


typedef struct INSTRUMENT
{
    UBYTE flags;

    UBYTE samplenumber[120];
    UBYTE samplenote[120];

    UBYTE nnatype;
    UBYTE dca;              /* duplicate check action */
    UBYTE dct;              /* duplicate check type */
    UBYTE globvol;
    UWORD panning;          /* instrument-based panning var */
    
    UBYTE pitpansep;        /* pitch pan separation (0 to 255) */
    UBYTE pitpancenter;     /* pitch pan center (0 to 119) */
    UBYTE rvolvar;          /* random volume varations (0 - 100%) */
    UBYTE rpanvar;          /* random panning varations (0 - 100%) */

    UWORD volfade;

    UBYTE volflg;           /* bit 0: on 1: sustain 2: loop */
    UBYTE volpts;
    UBYTE volsusbeg;
    UBYTE volsusend;
    UBYTE volbeg;
    UBYTE volend;
    ENVPT volenv[32];

    UBYTE panflg;           /* bit 0: on 1: sustain 2: loop */
    UBYTE panpts;
    UBYTE pansusbeg;
    UBYTE pansusend;
    UBYTE panbeg;
    UBYTE panend;
    ENVPT panenv[32];

    UBYTE pitflg;           /* bit 0: on 1: sustain 2: loop */
    UBYTE pitpts;
    UBYTE pitsusbeg;
    UBYTE pitsusend;
    UBYTE pitbeg;
    UBYTE pitend;
    ENVPT pitenv[32];

/*    UBYTE vibtype; */
/*    UBYTE vibsweep; */
/*    UBYTE vibdepth; */
/*    UBYTE vibrate; */

    CHAR  *insname;

} INSTRUMENT;



/**************************************************************************
****** Player stuff: ******************************************************
**************************************************************************/

typedef struct ENVPR
{   UBYTE flg;          /* envelope flag */
    UBYTE pts;          /* number of envelope points */
    UBYTE susbeg;       /* envelope sustain index begin */
    UBYTE susend;       /* envelope sustain index end */
    UBYTE beg;          /* envelope loop begin */
    UBYTE end;          /* envelope loop end */
    SWORD p;            /* current envelope counter */
    UWORD a;            /* envelope index a */
    UWORD b;            /* envelope index b */
    ENVPT *env;         /* envelope points */
} ENVPR;



/*  Struct MP_VOICE - Used by NNA only player (audio control.  AUDTMP is */
/*                  used for full effects control). */
typedef struct MP_VOICE
{   INSTRUMENT  *i;
    SAMPLE      *s;
    UBYTE  sample;       /* which instrument number */

    SWORD  volume;       /* output volume (vol + sampcol + instvol) */
    UWORD  panning;      /* panning position */
    SBYTE  chanvol;      /* channel's "global" volume */
    UWORD  fadevol;      /* fading volume rate */
    UWORD  period;       /* period to play the sample at */

    UBYTE  volflg;       /* volume envelope settings */
    UBYTE  panflg;       /* panning envelope settings */
    UBYTE  pitflg;       /* pitch envelope settings */

    UBYTE  keyoff;        /* if true = fade out and stuff */
    UBYTE  kick;         /* if true = sample has to be restarted */
    UBYTE  note;         /* the audible note (as heard, direct rep of period) */
    UBYTE  nna;          /* New note action type + master/slave flags */
    SWORD  handle;       /* which sample-handle */
    SLONG  start;        /* The start byte index in the sample */


    /* ---------------------------------- */
    /* Below here is info NOT in MP_CONTROL!! */
    /* ---------------------------------- */

    ENVPR  venv;
    ENVPR  penv;
    ENVPR  cenv;

    UWORD  avibpos;      /* autovibrato pos */
    UWORD  aswppos;      /* autovibrato sweep pos */

    ULONG  totalvol;     /* total volume of channel (before global mixings) */

    BOOL   mflag;
    SWORD  masterchn;
    struct MP_CONTROL *master;/* index of "master" effects channel */
} MP_VOICE;


typedef struct MP_CONTROL
{   INSTRUMENT  *i;
    SAMPLE      *s;
    UBYTE  sample;       /* which sample number */
    UBYTE  note;         /* the audible note (as heard, direct rep of period) */
    SWORD  outvolume;    /* output volume (vol + sampcol + instvol) */
    SBYTE  chanvol;      /* channel's "global" volume */
    UWORD  fadevol;      /* fading volume rate */
    UWORD  panning;      /* panning position */
    UBYTE  kick;         /* if true = sample has to be restarted */
    UBYTE  muted;        /* if set, channel not played */
    UWORD  period;       /* period to play the sample at */
    UBYTE  nna;          /* New note action type + master/slave flags */

    UBYTE  volflg;       /* volume envelope settings */
    UBYTE  panflg;       /* panning envelope settings */
    UBYTE  pitflg;       /* pitch envelope settings */

    UBYTE  keyoff;        /* if true = fade out and stuff */
    SWORD  handle;       /* which sample-handle */
    UBYTE  notedelay;    /* (used for note delay) */
    SLONG  start;        /* The starting byte index in the sample */

    struct MP_VOICE *slave;/* Audio Slave of current effects control channel */
    UBYTE slavechn;      /* Audio Slave of current effects control channel */
    UBYTE anote;         /* the note that indexes the audible (note seen in tracker) */
    SWORD ownper;
    SWORD ownvol;
    UBYTE dca;           /* duplicate check action */
    UBYTE dct;           /* duplicate check type */
    UBYTE *row;          /* row currently playing on this channel */
    SBYTE retrig;        /* retrig value (0 means don't retrig) */
    ULONG speed;         /* what finetune to use */
    SWORD volume;        /* amiga volume (0 t/m 64) to play the sample at */

    SBYTE tmpvolume;     /* tmp volume */
    UWORD tmpperiod;     /* tmp period */
    UWORD wantedperiod;  /* period to slide to (with effect 3 or 5) */
    UBYTE pansspd;       /* panslide speed */
    UWORD slidespeed;    /* */
    UWORD portspeed;     /* noteslide speed (toneportamento) */

    UBYTE s3mtremor;     /* s3m tremor (effect I) counter */
    UBYTE s3mtronof;     /* s3m tremor ontime/offtime */
    UBYTE s3mvolslide;   /* last used volslide */
    UBYTE s3mrtgspeed;   /* last used retrig speed */
    UBYTE s3mrtgslide;   /* last used retrig slide */

    UBYTE glissando;     /* glissando (0 means off) */
    UBYTE wavecontrol;   /* */

    SBYTE vibpos;        /* current vibrato position */
    UBYTE vibspd;        /* "" speed */
    UBYTE vibdepth;      /* "" depth */

    SBYTE trmpos;        /* current tremolo position */
    UBYTE trmspd;        /* "" speed */
    UBYTE trmdepth;      /* "" depth */

    UBYTE fslideupspd;
    UBYTE fslidednspd;
    UBYTE fportupspd;    /* fx E1 (extra fine portamento up) data */
    UBYTE fportdnspd;    /* fx E2 (extra fine portamento dn) data */
    UBYTE ffportupspd;   /* fx X1 (extra fine portamento up) data */
    UBYTE ffportdnspd;   /* fx X2 (extra fine portamento dn) data */

    ULONG hioffset;      /* last used high order of sample offset */
    UWORD soffset;       /* last used low order of sample-offset (effect 9) */

    UBYTE sseffect;      /* last used Sxx effect */
    UBYTE ssdata;        /* last used Sxx data info */
    UBYTE chanvolslide;  /* last used channel volume slide */

    UBYTE panbwave;      /* current panbrello waveform */
    UBYTE panbpos;       /* current panbrello position */
    SBYTE panbspd;       /* "" speed */
    UBYTE panbdepth;     /* "" depth */

    UWORD newsamp;       /* set to 1 upon a sample / inst change */
    UBYTE voleffect;     /* Volume Column Effect Memory as used by Impulse Tracker */
    UBYTE voldata;       /* Volume Column Data Memory */
} MP_CONTROL;


/******************************************************
******** MikMod UniMod type: **************************
*******************************************************/

/* UniMod flags */
#define UF_XMPERIODS    1       /* XM periods / finetuning */
#define UF_LINEAR       2       /* LINEAR periods (UF_XMPERIODS must be set as well) */
#define UF_INST         4       /* Instruments are used */
#define UF_NNA          8       /* New Note Actions used (set numvoices rather than numchn) */


typedef struct UNIMOD
{
    /* This section of elements are all file-storage related. */
    /* all of this information can be found in the UNIMOD disk format. */
    /* For further details about there variables, see the MikMod Docs. */

    UWORD       flags;          /* See UniMod Flags above */
    UBYTE       numchn;         /* number of module channels */
    UBYTE       numvoices;      /* max # voices used for full NNA playback */
    UWORD       numpos;         /* number of positions in this song */
    UWORD       numpat;         /* number of patterns in this song */
    UWORD       numtrk;         /* number of tracks */
    UWORD       numins;         /* number of instruments */
    UWORD       numsmp;         /* number of samples */
    UWORD       reppos;         /* restart position */
    UBYTE       initspeed;      /* initial song speed */
    UBYTE       inittempo;      /* initial song tempo */
    UBYTE       initvolume;     /* initial global volume (0 - 128) */
    UWORD       panning[64];    /* 64 panning positions */
    UBYTE       chanvol[64];    /* 64 channel positions */
    CHAR       *songname;       /* name of the song */
    CHAR       *composer;       /* name of the composer */
    CHAR       *comment;        /* module comments */
    UBYTE     **tracks;         /* array of numtrk pointers to tracks */
    UWORD      *patterns;       /* array of Patterns [pointers to tracks for each channel]. */
    UWORD      *pattrows;       /* array of number of rows for each pattern */
    UWORD      *positions;      /* all positions */
    INSTRUMENT *instruments;    /* all instruments */
    SAMPLE     *samples;        /* all samples */

    /* following are the player-instance variables.  They are in no way file */
    /* storage related - they are for internal replay use only. */

    /* All following variables can be modified at any time. */

    CHAR       *modtype;        /* string type of module loaded */
    UBYTE       bpm;            /* current beats-per-minute speed */
    UWORD       sngspd;         /* current song speed */
    SWORD       volume;         /* song volume (0-128) (or user volume) */
    BOOL        extspd;         /* extended speed flag (default enabled) */
    BOOL        panflag;        /* panning flag (default enabled) */
    BOOL        loop;           /* loop module ? (default disabled) */
    BOOL        forbid;         /* if true, no player update! */

    /* The following variables are considered useful for reading, and should */
    /* should not be directly modified by the end user. */

    MP_CONTROL *control;        /* Effects Channel information (pf->numchn alloc'ed) */
    MP_VOICE   *voice;          /* Audio Voice information (md_numchn alloc'ed) */
    UWORD       numrow;         /* number of rows on current pattern */
    UWORD       vbtick;         /* tick counter (counts from 0 to sngspd) */
    UWORD       patpos;         /* current row number */
    SWORD       sngpos;         /* current song position.  This should not */
                                /* be modified directly.  Use MikMod_NextPosition, */
                                /* MikMod_PrevPosition, and MikMod_SetPosition. */

    /* The following variables should not be modified, and have information */
    /* that is pretty useless outside the internal player, so just ignore :) */

    UBYTE       globalslide;    /* global volume slide rate */
    UWORD       pat_reppos;     /* patternloop position */
    UWORD       pat_repcnt;     /* times to loop */
    UWORD       patbrk;         /* position where to start a new pattern */
    UBYTE       patdly;         /* patterndelay counter (command memory) */
    UBYTE       patdly2;        /* patterndelay counter (real one) */
    SWORD       posjmp;         /* flag to indicate a position jump is needed... */
                                /*  changed since 1.00: now also indicates the */
                                /*  direction the position has to jump to: */
                                /*  0: Don't do anything */
                                /*  1: Jump back 1 position */
                                /*  2: Restart on current position */
                                /*  3: Jump forward 1 position */

} UNIMOD;


/***************************************************
****** Loader stuff: *******************************
****************************************************/

/* loader structure: */

typedef struct MLOADER
{   struct MLOADER *next;
    CHAR    *type;
    CHAR    *version;
    BOOL    (*Init)(void);
    BOOL    (*Test)(void);
    BOOL    (*Load)(void);
    void    (*Cleanup)(void);
    CHAR    *(*LoadTitle)(void);
} MLOADER;

/* public loader variables: */

extern FILE   *modfp;
extern UWORD  finetune[16];
extern UNIMOD of;           /* static unimod loading space */
extern UWORD  npertab[60];  /* used by the original MOD loaders */

/* main loader prototypes: */

void    ML_InfoLoader(void);
void    ML_RegisterLoader(MLOADER *ldr);
UNIMOD *MikMod_LoadSongFP(FILE *fp, int maxchan);
UNIMOD *MikMod_LoadSong(CHAR *filename, int maxchan);
void    MikMod_FreeSong(UNIMOD *mf);


/* other loader prototypes: (used by the loader modules) */

BOOL    InitTracks(void);
void    AddTrack(UBYTE *tr);
BOOL    ReadComment(UWORD len);
BOOL    AllocPositions(int total);
BOOL    AllocPatterns(void);
BOOL    AllocTracks(void);
BOOL    AllocInstruments(void);
BOOL    AllocSamples(void);
CHAR    *DupStr(CHAR *s, UWORD len);


/* Declare external loaders: */

extern MLOADER  load_uni;        /* Internal UniMod Loader (Current version of UniMod only) */
extern MLOADER  load_mod;        /* Standard 31-instrument Module loader (Protracker, StarTracker, FastTracker, etc) */
extern MLOADER  load_m15;        /* 15-instrument (SoundTracker and Ultimate SoundTracker) */
extern MLOADER  load_mtm;        /* Multi-Tracker Module (by Renaissance) */
extern MLOADER  load_s3m;        /* ScreamTracker 3 (by Future Crew) */
extern MLOADER  load_stm;        /* ScreamTracker 2 (by Future Crew) */
extern MLOADER  load_ult;        /* UltraTracker  */
extern MLOADER  load_xm;         /* FastTracker 2 (by Trition) */
extern MLOADER  load_it;         /* Impulse Tracker (by Jeffrey Lim) */
extern MLOADER  load_669;        /* 669 and Extended-669 (by Tran / Renaissance) */
extern MLOADER  load_dsm;        /* DSIK internal module format */
extern MLOADER  load_med;        /* MMD0 and MMD1 Amiga MED modules (by OctaMED) */
extern MLOADER  load_far;        /* Farandole Composer Module */

/* used to convert c4spd to linear XM periods (IT loader). */
extern UWORD getlinearperiod(UBYTE note, ULONG fine);
extern ULONG getfrequency(UBYTE flags, ULONG period);


#define MP_HandleTick     Player_HandleTick
#define ML_LoadFN(x,y)    MikMod_LoadSong(x,y)
#define ML_LoadFP(x,y)    MikMod_LoadSongFP(x,y)
#define MP_PlayStart(x)   Player_Start(x)
#define MP_PlayStop       Player_Stop


/* MikMod Player Prototypes: */
/* =========================================================== */
/* This batch of prototypes affects the currently ACTIVE module */
/* set with MikMod_PlayStart) */

extern void   Player_Start(UNIMOD *mf);
extern BOOL   Player_Active(void);
extern void   Player_Stop(void);
extern void   Player_TogglePause(void);
extern void   Player_NextPosition(void);
extern void   Player_PrevPosition(void);
extern void   Player_SetPosition(UWORD pos);
extern void   Player_Mute(SLONG arg1, ...);
extern void   Player_UnMute(SLONG arg1, ...);
extern void   Player_ToggleMute(SLONG arg1, ...);
extern BOOL   Player_Muted(int chan);
extern void   Player_HandleTick(void);
extern void   Player_SetVolume(int volume);
extern UNIMOD *Player_GetUnimod(void);

extern BOOL   Player_Init(UNIMOD *mf);   /* internal use only [by loader] */
extern void   Player_Exit(UNIMOD *mf);   /* internal use only [by loader] */

/* This batch of prototypes adheres closely to the old MikMod 2.10 */
/* naming, and affects ANY specified module (need not be active, */
/* only loaded and initialized) */

extern BOOL   MP_Playing(UNIMOD *mf);
extern void   MP_TogglePause(UNIMOD *mf);
extern void   MP_NextPosition(UNIMOD *mf);
extern void   MP_PrevPosition(UNIMOD *mf);
extern void   MP_SetPosition(UNIMOD *mf, UWORD pos);
extern void   MP_Mute(UNIMOD *mf, SLONG arg1, ...);
extern void   MP_UnMute(UNIMOD *mf, SLONG arg1, ...);
extern void   MP_ToggleMute(UNIMOD *mf, SLONG arg1, ...);
extern BOOL   MP_Muted(UNIMOD *mf, int chan);

#ifdef __cplusplus
}
#endif

#endif

