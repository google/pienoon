/*

Name:  VIRTCH.C

Description:
 Sample mixing routines, using a 32 bits mixing buffer.

 Optional features include:
   (a) 4-step reverb (for 16 bit output only)
   (b) Interpolation of sample data during mixing
   (c) Dolby Surround Sound
   (d) Optimized assembly mixers for the Intel platform
   (e) Optional high-speed or high-quality modes

C Mixer Portability:
 All Systems -- All compilers.

Assembly Mixer Portability:

 MSDOS:  BC(?)   Watcom(y)   DJGPP(y)
 Win95:  ?
 Os2:    ?
 Linux:  y

 (y) - yes
 (n) - no (not possible or not useful)
 (?) - may be possible, but not tested

*/

#include <stddef.h>
#include <string.h>
#include "mikmod.h"


/*
// ** For PC-assembly mixing
// =========================
// Uncomment both lines below for assembly mixing under WATCOM or GCC for
// Linux.  Note that there is no 16 bit mixers for assembly yet (C only), so
// defining __ASSEMBLY__ when not defining __FASTMIXER__ will lead to compiler
// errors.
*/

#define __FASTMIXER__
/* #define __ASSEMBLY__ */
#define __FAST_REVERB__

/*
// Various other VIRTCH.C Compiler Options
// =======================================

// BITSHIFT : Controls the maximum volume of the sound output.  All data
//      is shifted right by BITSHIFT after being mixed.  Higher values
//      result in quieter sound and less chance of distortion.  If you
//      are using the assembly mixer, you must change the bitshift const-
//      ant in RESAMPLE.ASM or RESAMPLE.S as well!
*/

#define BITSHIFT 9

/*
// REVERBERATION : Controls the duration of the reverb.  Larger values
//      represent a shorter reverb loop.  Smaller values extend the reverb
//      but can result in more of an echo-ish sound.
*/

#define REVERBERATION  110000l

/*
// BOUNDS_CHECKING : Forces VIRTCH to perform bounds checking.  Commenting
//      the line below will result in a slightly faster mixing process but
//      could cause nasty clicks and pops on some modules.  Disable this
//      option on games or demos only, where speed is very important all
//      songs / sndfx played can be specifically tested for pops.
*/

#define BOUNDS_CHECKING


#ifndef __cdecl
#ifdef __GNUC__
#define __cdecl
#endif
#endif

#ifdef __WATCOMC__
#define   inline
#endif

#define FRACBITS 11
#define FRACMASK ((1l<<FRACBITS)-1l)

#define TICKLSIZE 8192
#define TICKWSIZE (TICKLSIZE*2)
#define TICKBSIZE (TICKWSIZE*2)

#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif


typedef struct
{   UBYTE  kick;                  /* =1 -> sample has to be restarted */
    UBYTE  active;                /* =1 -> sample is playing */
    UWORD  flags;                 /* 16/8 bits looping/one-shot */
    SWORD  handle;                /* identifies the sample */
    ULONG  start;                 /* start index */
    ULONG  size;                  /* samplesize */
    ULONG  reppos;                /* loop start */
    ULONG  repend;                /* loop end */
    ULONG  frq;                   /* current frequency */
    int    vol;                   /* current volume */
    int    pan;                   /* current panning position */
    SLONG  current;               /* current index in the sample */
    SLONG  increment;             /* fixed-point increment value */
} VINFO;

#ifdef __FASTMIXER__
static SBYTE **Samples;
SLONG         *lvoltab, *rvoltab; /* Volume Table values for use by 8 bit mixers */
#else
static SWORD **Samples;
static SLONG  lvolsel, rvolsel;   /* Volume Selectors for 16 bit mixers. */
#endif

/* Volume table for 8 bit sample mixing */
#ifdef __FASTMIXER__
static SLONG  **voltab;
#endif

static VINFO   *vinf = NULL, *vnf;
static long    TICKLEFT, samplesthatfit, vc_memory = 0;
static int     vc_softchn;
static SLONG   idxsize, idxlpos, idxlend;
static SLONG   *VC_TICKBUF = NULL;
static UWORD   vc_mode;


/*
// Reverb control variables
// ========================
*/

static int     RVc1, RVc2, RVc3, RVc4;
#ifndef __FAST_REVERB__
static int     RVc5, RVc6, RVc7, RVc8;
#endif
static ULONG   RVRindex;


/* For Mono or Left Channel */

static SLONG  *RVbuf1 = NULL, *RVbuf2 = NULL, *RVbuf3 = NULL, *RVbuf4 = NULL;
#ifndef __FAST_REVERB__
static SLONG  *RVbuf5 = NULL, *RVbuf6 = NULL, *RVbuf7 = NULL, *RVbuf8 = NULL;
#endif

/*
// For Stereo only (Right Channel)
//   Values start at 9 to leave room for expanding this to 8-step
//   reverb in the future.
*/

static SLONG  *RVbuf9 = NULL, *RVbuf10 = NULL, *RVbuf11 = NULL, *RVbuf12 = NULL;
#ifndef __FAST_REVERB__
static SLONG  *RVbuf13 = NULL, *RVbuf14 = NULL, *RVbuf15 = NULL, *RVbuf16 = NULL;
#endif


/*
// Define external Assembly Language Prototypes
// ============================================
*/

#ifdef __ASSEMBLY__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __cdecl
#ifdef __GNUC__
#define __cdecl
#endif
#endif

void __cdecl AsmStereoNormal(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);
void __cdecl AsmStereoInterp(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);
void __cdecl AsmSurroundNormal(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);
void __cdecl AsmSurroundInterp(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);
void __cdecl AsmMonoNormal(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);
void __cdecl AsmMonoInterp(SBYTE *srce,SLONG *dest,SLONG index,SLONG increment,SLONG todo);

#ifdef __cplusplus
};
#endif

#else

#ifdef __FASTMIXER__

/*
// ==============================================================
//  8 bit sample mixers!
*/

static SLONG MixStereoNormal(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    UBYTE  sample1, sample2, sample3, sample4;
    int    remain;

    remain = todo & 3;
    
    for(todo>>=2; todo; todo--)
    {
        sample1 = srce[index >> FRACBITS];
        index  += increment;
        sample2 = srce[index >> FRACBITS];
        index  += increment;
        sample3 = srce[index >> FRACBITS];
        index  += increment;
        sample4 = srce[index >> FRACBITS];
        index  += increment;
        
        *dest++ += lvoltab[sample1];
        *dest++ += rvoltab[sample1];
        *dest++ += lvoltab[sample2];
        *dest++ += rvoltab[sample2];
        *dest++ += lvoltab[sample3];
        *dest++ += rvoltab[sample3];
        *dest++ += lvoltab[sample4];
        *dest++ += rvoltab[sample4];
    }

    for(; remain--; )
    {
        sample1    = srce[index >> FRACBITS];
        index     += increment;
        *dest++   += lvoltab[sample1];
        *dest++   += rvoltab[sample1];
    }
    
    return index;
}


static SLONG MixStereoInterp(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    UBYTE  sample;

    for(; todo; todo--)
    {   sample = (UBYTE)((srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                 (srce[(index >> FRACBITS) + 1] * (index & FRACMASK)) >> FRACBITS);
        index  += increment;

        *dest++ += lvoltab[sample];
        *dest++ += rvoltab[sample];
    }   

    return index;
}


static SLONG MixSurroundNormal(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SLONG sample1, sample2, sample3, sample4;
    int   remain;

    remain = todo & 3;
    
    for(todo>>=2; todo; todo--)
    {
        sample1 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index  += increment;
        sample2 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index  += increment;
        sample3 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index  += increment;
        sample4 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index  += increment;
        
        *dest++ += sample1;
        *dest++ -= sample1;
        *dest++ += sample2;
        *dest++ -= sample2;
        *dest++ += sample3;
        *dest++ -= sample3;
        *dest++ += sample4;
        *dest++ -= sample4;
    }

    for(; remain--; )
    {   sample1   = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index    += increment;
        *dest++  += sample1;
        *dest++  -= sample1;
    }

    return index;
}


static SLONG MixSurroundInterp(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SLONG sample;

    for(; todo; todo--)
    {   sample = lvoltab[(UBYTE)((srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                         (srce[(index >> FRACBITS) + 1] * (index & FRACMASK)) >> FRACBITS)];
        index  += increment;
    
        *dest++ += sample;
        *dest++ -= sample;
    }
    
    return index;
}


static SLONG MixMonoNormal(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SLONG  sample1, sample2, sample3, sample4;
    int    remain;

    remain = todo & 3;

    for(todo>>=2; todo; todo--)
    {
        sample1 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index += increment;
        sample2 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index += increment;
        sample3 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index += increment;
        sample4 = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index += increment;

        *dest++ += sample1;
        *dest++ += sample2;
        *dest++ += sample3;
        *dest++ += sample4;
    }

    for(; remain--;)
    {   sample1    = lvoltab[(UBYTE)srce[index >> FRACBITS]];
        index     += increment;
        *dest++   += sample1;
    }

    return index;
}


static SLONG MixMonoInterp(SBYTE *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SLONG  sample;

    for(; todo; todo--)
    {   sample = lvoltab[(UBYTE)(((srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                         (srce[(index >> FRACBITS) + 1] * (index & FRACMASK))) >> FRACBITS)];
        index += increment;

        *dest++ += sample;
    }

    return index;
}


#else

/*
// ==============================================================
//  16 bit sample mixers!
*/

static SLONG MixStereoNormal(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, ULONG todo)
{
    SWORD  sample;

    for(; todo; todo--)
    {
        sample = srce[index >> FRACBITS];
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }
    
    return index;
}


static SLONG MixStereoInterp(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, ULONG todo)
{
    SWORD  sample;

    for(; todo; todo--)
    {
        sample = (srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                 (srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS;
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ += rvolsel * sample;
    }
    
    return index;
}


static SLONG MixSurroundNormal(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, ULONG todo)
{
    SWORD  sample;

    for (; todo; todo--)
    {
        sample = srce[index >> FRACBITS];
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ -= lvolsel * sample;
    }
    
    return index;
}


static SLONG MixSurroundInterp(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, ULONG todo)
{
    SWORD  sample;

    for (; todo; todo--)
    {
        sample = (srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                 (srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS;
        index += increment;

        *dest++ += lvolsel * sample;
        *dest++ -= lvolsel * sample;
    }
    
    return index;
}


static SLONG MixMonoNormal(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SWORD  sample;
    
    for(; todo; todo--)
    {
        sample = srce[index >> FRACBITS];
        index += increment;

        *dest++ += lvolsel * sample;
    }

    return index;
}


static SLONG MixMonoInterp(SWORD *srce, SLONG *dest, SLONG index, SLONG increment, SLONG todo)
{
    SWORD  sample;
    
    for(; todo; todo--)
    {
        sample = (srce[index >> FRACBITS] * ((FRACMASK+1l) - (index & FRACMASK))) +
                 (srce[(index >> FRACBITS)+1] * (index & FRACMASK)) >> FRACBITS;
        index += increment;

        *dest++ += lvolsel * sample;
    }

    return index;
}


#endif
#endif

static void (*MixReverb)(SLONG *srce, SLONG count);

static void MixReverb_Normal(SLONG *srce, SLONG count)
{
    unsigned int  speedup;
    int           ReverbPct;
    unsigned int  loc1, loc2, loc3, loc4;
#ifndef __FAST_REVERB__
    unsigned int  loc5, loc6, loc7, loc8;

    ReverbPct = 58 + (md_reverb*4);
#else
    ReverbPct = 89 + (md_reverb*2);
#endif

    loc1 = RVRindex % RVc1;
    loc2 = RVRindex % RVc2;
    loc3 = RVRindex % RVc3;
    loc4 = RVRindex % RVc4;
#ifndef __FAST_REVERB__
    loc5 = RVRindex % RVc5;
    loc6 = RVRindex % RVc6;
    loc7 = RVRindex % RVc7;
    loc8 = RVRindex % RVc8;
#endif

    for(; count; count--)
    {
        /* Compute the LEFT CHANNEL echo buffers! */

        speedup = *srce >> 3;

        RVbuf1[loc1] = speedup + ((ReverbPct * RVbuf1[loc1]) / 128);
        RVbuf2[loc2] = speedup + ((ReverbPct * RVbuf2[loc2]) / 128);
        RVbuf3[loc3] = speedup + ((ReverbPct * RVbuf3[loc3]) / 128);
        RVbuf4[loc4] = speedup + ((ReverbPct * RVbuf4[loc4]) / 128);
#ifndef __FAST_REVERB__
        RVbuf5[loc5] = speedup + ((ReverbPct * RVbuf5[loc5]) / 128);
        RVbuf6[loc6] = speedup + ((ReverbPct * RVbuf6[loc6]) / 128);
        RVbuf7[loc7] = speedup + ((ReverbPct * RVbuf7[loc7]) / 128);
        RVbuf8[loc8] = speedup + ((ReverbPct * RVbuf8[loc8]) / 128);
#endif

        /* Prepare to compute actual finalized data! */

        RVRindex++;
        loc1 = RVRindex % RVc1;
        loc2 = RVRindex % RVc2;
        loc3 = RVRindex % RVc3;
        loc4 = RVRindex % RVc4;
#ifndef __FAST_REVERB__
        loc5 = RVRindex % RVc5;
        loc6 = RVRindex % RVc6;
        loc7 = RVRindex % RVc7;
        loc8 = RVRindex % RVc8;
#endif
        /* Left Channel! */
        
#ifdef __FAST_REVERB__
        *srce++ += RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4];
#else
        *srce++ += RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4] +
                   RVbuf5[loc5] - RVbuf6[loc6] + RVbuf7[loc7] - RVbuf8[loc8];
#endif
    }
}


static void MixReverb_Stereo(SLONG *srce, SLONG count)
{
    unsigned int  speedup;
    int           ReverbPct;
    unsigned int  loc1, loc2, loc3, loc4;
#ifndef __FAST_REVERB__
    unsigned int  loc5, loc6, loc7, loc8;

    ReverbPct = 63 + (md_reverb*4);
#else
    ReverbPct = 92 + (md_reverb*2);
#endif

    loc1 = RVRindex % RVc1;
    loc2 = RVRindex % RVc2;
    loc3 = RVRindex % RVc3;
    loc4 = RVRindex % RVc4;
#ifndef __FAST_REVERB__
    loc5 = RVRindex % RVc5;
    loc6 = RVRindex % RVc6;
    loc7 = RVRindex % RVc7;
    loc8 = RVRindex % RVc8;
#endif

    for(; count; count--)
    {
        /* Compute the LEFT CHANNEL echo buffers! */

        speedup = *srce >> 3;

        RVbuf1[loc1] = speedup + ((ReverbPct * RVbuf1[loc1]) / 128);
        RVbuf2[loc2] = speedup + ((ReverbPct * RVbuf2[loc2]) / 128);
        RVbuf3[loc3] = speedup + ((ReverbPct * RVbuf3[loc3]) / 128);
        RVbuf4[loc4] = speedup + ((ReverbPct * RVbuf4[loc4]) / 128);
#ifndef __FAST_REVERB__
        RVbuf5[loc5] = speedup + ((ReverbPct * RVbuf5[loc5]) / 128);
        RVbuf6[loc6] = speedup + ((ReverbPct * RVbuf6[loc6]) / 128);
        RVbuf7[loc7] = speedup + ((ReverbPct * RVbuf7[loc7]) / 128);
        RVbuf8[loc8] = speedup + ((ReverbPct * RVbuf8[loc8]) / 128);
#endif

        /* Compute the RIGHT CHANNEL echo buffers! */
        
        speedup = srce[1] >> 3;

        RVbuf9[loc1]  = speedup + ((ReverbPct * RVbuf9[loc1]) / 128);
        RVbuf10[loc2] = speedup + ((ReverbPct * RVbuf11[loc2]) / 128);
        RVbuf11[loc3] = speedup + ((ReverbPct * RVbuf12[loc3]) / 128);
        RVbuf12[loc4] = speedup + ((ReverbPct * RVbuf12[loc4]) / 128);
#ifndef __FAST_REVERB__
        RVbuf13[loc5] = speedup + ((ReverbPct * RVbuf13[loc5]) / 128);
        RVbuf14[loc6] = speedup + ((ReverbPct * RVbuf14[loc6]) / 128);
        RVbuf15[loc7] = speedup + ((ReverbPct * RVbuf15[loc7]) / 128);
        RVbuf16[loc8] = speedup + ((ReverbPct * RVbuf16[loc8]) / 128);
#endif

        /* Prepare to compute actual finalized data! */

        RVRindex++;
        loc1 = RVRindex % RVc1;
        loc2 = RVRindex % RVc2;
        loc3 = RVRindex % RVc3;
        loc4 = RVRindex % RVc4;
#ifndef __FAST_REVERB__
        loc5 = RVRindex % RVc5;
        loc6 = RVRindex % RVc6;
        loc7 = RVRindex % RVc7;
        loc8 = RVRindex % RVc8;
#endif

#ifdef __FAST_REVERB__
        /* Left Channel then right channel! */
        *srce++ += RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4];
        *srce++ += RVbuf9[loc1] - RVbuf10[loc2] + RVbuf11[loc3] - RVbuf12[loc4];
#else
        /* Left Channel then right channel! */
        *srce++ += RVbuf1[loc1] - RVbuf2[loc2] + RVbuf3[loc3] - RVbuf4[loc4] +
                   RVbuf5[loc5] - RVbuf6[loc6] + RVbuf7[loc7] - RVbuf8[loc8];

        *srce++ += RVbuf9[loc1] - RVbuf10[loc2] + RVbuf11[loc3] - RVbuf12[loc4] +
                   RVbuf13[loc5] - RVbuf14[loc6] + RVbuf15[loc7] - RVbuf16[loc8];
#endif
    }
}


static void Mix32To16(SWORD *dste, SLONG *srce, SLONG count)
{
    SLONG         x1, x2, x3, x4;
    int           remain;

    remain = count & 3;
    
    for(count>>=2; count; count--)
    {   x1 = *srce++ >> BITSHIFT;
        x2 = *srce++ >> BITSHIFT;
        x3 = *srce++ >> BITSHIFT;
        x4 = *srce++ >> BITSHIFT;

#ifdef BOUNDS_CHECKING
        x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
        x2 = (x2 > 32767) ? 32767 : (x2 < -32768) ? -32768 : x2;
        x3 = (x3 > 32767) ? 32767 : (x3 < -32768) ? -32768 : x3;
        x4 = (x4 > 32767) ? 32767 : (x4 < -32768) ? -32768 : x4;
#endif

        *dste++ = x1;
        *dste++ = x2;
        *dste++ = x3;
        *dste++ = x4;
    }

    for(; remain; remain--)
    {   x1 = *srce++ >> BITSHIFT;
#ifdef BOUNDS_CHECKING
        x1 = (x1 > 32767) ? 32767 : (x1 < -32768) ? -32768 : x1;
#endif
        *dste++ = x1;
    }
}


static void Mix32To8(SBYTE *dste, SLONG *srce, SLONG count)
{
    int   x1, x2, x3, x4;
    int   remain;
    
    remain = count & 3;
    
    for(count>>=2; count; count--)
    {   x1 = *srce++ >> (BITSHIFT + 8);
        x2 = *srce++ >> (BITSHIFT + 8);
        x3 = *srce++ >> (BITSHIFT + 8);
        x4 = *srce++ >> (BITSHIFT + 8);

#ifdef BOUNDS_CHECKING
        x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
        x2 = (x2 > 127) ? 127 : (x2 < -128) ? -128 : x2;
        x3 = (x3 > 127) ? 127 : (x3 < -128) ? -128 : x3;
        x4 = (x4 > 127) ? 127 : (x4 < -128) ? -128 : x4;
#endif

        *dste++ = x1 + 128;
        *dste++ = x2 + 128;
        *dste++ = x3 + 128;
        *dste++ = x4 + 128;
    }

    for(; remain; remain--)
    {   x1 = *srce++ >> (BITSHIFT + 8);
#ifdef BOUNDS_CHECKING
        x1 = (x1 > 127) ? 127 : (x1 < -128) ? -128 : x1;
#endif
        *dste++ = x1 + 128;
    }
}


static ULONG samples2bytes(ULONG samples)
{
    if(vc_mode & DMODE_16BITS) samples <<= 1;
    if(vc_mode & DMODE_STEREO) samples <<= 1;
    return samples;
}


static ULONG bytes2samples(ULONG bytes)
{
    if(vc_mode & DMODE_16BITS) bytes >>= 1;
    if(vc_mode & DMODE_STEREO) bytes >>= 1;
    return bytes;
}


static void AddChannel(SLONG *ptr, SLONG todo)
{
    SLONG  end, done;
#ifdef __FASTMIXER__
    SBYTE  *s;
#else
    SWORD  *s;
#endif

    if((s=Samples[vnf->handle]) == NULL)
    {   vnf->current = 0;
        vnf->active  = 0;
        return;
    }

    while(todo > 0)
    {   /* update the 'current' index so the sample loops, or
        // stops playing if it reached the end of the sample
	*/

        if(vnf->flags & SF_REVERSE)
        {
            /* The sample is playing in reverse */

            if((vnf->flags & SF_LOOP) && (vnf->current < idxlpos))
            {
                /* the sample is looping, and it has
                // reached the loopstart index
		*/

                if(vnf->flags & SF_BIDI)
                {
                    /* sample is doing bidirectional loops, so 'bounce'
                    // the current index against the idxlpos
		    */

                    vnf->current    = idxlpos + (idxlpos - vnf->current);
                    vnf->flags     &= ~SF_REVERSE;
                    vnf->increment  = -vnf->increment;
                } else
                    /* normal backwards looping, so set the
                    // current position to loopend index
		    */

                   vnf->current = idxlend - (idxlpos-vnf->current);
            } else
            {
                /* the sample is not looping, so check if it reached index 0 */

                if(vnf->current < 0)
                {
                    /* playing index reached 0, so stop playing this sample */

                    vnf->current = 0;
                    vnf->active  = 0;
                    break;
                }
            }
        } else
        {
            /* The sample is playing forward */

            if((vnf->flags & SF_LOOP) && (vnf->current > idxlend))
            {
                /* the sample is looping, so check if
                // it reached the loopend index
		*/

                if(vnf->flags & SF_BIDI)
                {
                    /* sample is doing bidirectional loops, so 'bounce'
                    //  the current index against the idxlend
		    */

                    vnf->flags    |= SF_REVERSE;
                    vnf->increment = -vnf->increment;
                    vnf->current   = idxlend - (vnf->current-idxlend);
                } else
                    /* normal backwards looping, so set the
                    // current position to loopend index
		    */

                    vnf->current = idxlpos + (vnf->current-idxlend);
            } else
            {
                /* sample is not looping, so check
                // if it reached the last position
		*/

                if(vnf->current > idxsize)
                {
                    /* yes, so stop playing this sample */

                    vnf->current = 0;
                    vnf->active  = 0;
                    break;
                }
            }
        }

        end = (vnf->flags & SF_REVERSE) ? 
                (vnf->flags & SF_LOOP) ? idxlpos : 0 :
                (vnf->flags & SF_LOOP) ? idxlend : idxsize;

        done = MIN((end - vnf->current) / vnf->increment + 1, todo);

        if(!done)
        {   vnf->active = 0;
            break;
        }

        if(vnf->vol)
        {
#ifdef __ASSEMBLY__
            if(md_mode & DMODE_INTERP)
            {   if(vc_mode & DMODE_STEREO)
                    if((vnf->pan == PAN_SURROUND) && (vc_mode & DMODE_SURROUND))
                        AsmSurroundInterp(s,ptr,vnf->current,vnf->increment,done);
                    else
                        AsmStereoInterp(s,ptr,vnf->current,vnf->increment,done);
                else
                    AsmMonoInterp(s,ptr,vnf->current,vnf->increment,done);
            } else if(vc_mode & DMODE_STEREO)
                if((vnf->pan == PAN_SURROUND) && (vc_mode & DMODE_SURROUND))
                    AsmSurroundNormal(s,ptr,vnf->current,vnf->increment,done);
                else
                    AsmStereoNormal(s,ptr,vnf->current,vnf->increment,done);
            else
                AsmMonoNormal(s,ptr,vnf->current,vnf->increment,done);

            vnf->current += (vnf->increment*done);
#else
            if((md_mode & DMODE_INTERP))
            {   if(vc_mode & DMODE_STEREO)
                    if((vnf->pan == PAN_SURROUND) && (vc_mode & DMODE_SURROUND))
                        vnf->current = MixSurroundInterp(s,ptr,vnf->current,vnf->increment,done);
                    else
                        vnf->current = MixStereoInterp(s,ptr,vnf->current,vnf->increment,done);
                else
                    vnf->current = MixMonoInterp(s,ptr,vnf->current,vnf->increment,done);
            } else if(vc_mode & DMODE_STEREO)
                if((vnf->pan == PAN_SURROUND) && (vc_mode & DMODE_SURROUND))
                    vnf->current = MixSurroundNormal(s,ptr,vnf->current,vnf->increment,done);
                else
                    vnf->current = MixStereoNormal(s,ptr,vnf->current,vnf->increment,done);
            else
                vnf->current = MixMonoNormal(s,ptr,vnf->current,vnf->increment,done);
#endif
        }

        todo -= done;
        ptr  += (vc_mode & DMODE_STEREO) ? (done<<1) : done;
    }

}


void VC_WriteSamples(SBYTE *buf, ULONG todo)
{
    int    left, portion = 0, count;
    SBYTE  *buffer;
    int    t;
    int    pan, vol;

    while(todo)
    {   if(TICKLEFT==0)
        {   if(vc_mode & DMODE_SOFT_MUSIC) md_player();
            TICKLEFT = (md_mixfreq * 125l) / (md_bpm * 50L);
        }

        left = MIN(TICKLEFT, todo);
        
        buffer    = buf;
        TICKLEFT -= left;
        todo     -= left;

        buf += samples2bytes(left);

        while(left)
        {   portion = MIN(left, samplesthatfit);
            count   = (vc_mode & DMODE_STEREO) ? (portion<<1) : portion;
            
            memset(VC_TICKBUF, 0, count<<2);

            for(t=0; t<vc_softchn; t++)
            {   vnf = &vinf[t];

                if(vnf->kick)
                {   vnf->current = vnf->start << FRACBITS;
                    vnf->kick    = 0;
                    vnf->active  = 1;
                }
                
                if((vnf->frq == 0) || (vnf->size == 0))  vnf->active = 0;
                
                if(vnf->active)
                {   vnf->increment = (vnf->frq << FRACBITS) / md_mixfreq;
                    if(vnf->flags & SF_REVERSE) vnf->increment =- vnf->increment;
                    vol = vnf->vol;  pan = vnf->pan;

                    if(vc_mode & DMODE_STEREO)
                    {   if(pan != PAN_SURROUND)
                        {
#ifdef __FASTMIXER__
                            lvoltab = voltab[(vol * (255-pan)) / 1024];
                            rvoltab = voltab[(vol * pan) / 1024];
#else
                            lvolsel = (vol * (255-pan)) >> 8;
                            rvolsel = (vol * pan) >> 8;
#endif
                        } else
                        {
#ifdef __FASTMIXER__
                            lvoltab = voltab[(vol+1)>>3];
#else
                            lvolsel = vol/2;
#endif
                        }
                    } else
                    {
#ifdef __FASTMIXER__
                        lvoltab = voltab[vol>>2];
#else
                        lvolsel = vol;
#endif
                    }

                    idxsize = (vnf->size)   ? (vnf->size << FRACBITS)-1 : 0;
                    idxlend = (vnf->repend) ? (vnf->repend << FRACBITS)-1 : 0;
                    idxlpos = vnf->reppos << FRACBITS;
                    AddChannel(VC_TICKBUF, portion);
                }
            }

            if(md_reverb) MixReverb(VC_TICKBUF, portion);

            if(vc_mode & DMODE_16BITS)
                Mix32To16((SWORD *) buffer, VC_TICKBUF, count);
            else
                Mix32To8((SBYTE *) buffer, VC_TICKBUF, count);

            buffer += samples2bytes(portion);
            left   -= portion;
        }
    }
}


void VC_SilenceBytes(SBYTE *buf, ULONG todo)

/*  Fill the buffer with 'todo' bytes of silence (it depends on the mixing
//  mode how the buffer is filled)
*/

{
    /* clear the buffer to zero (16 bits signed) or 0x80 (8 bits unsigned) */

    if(vc_mode & DMODE_16BITS)
        memset(buf,0,todo);
    else
        memset(buf,0x80,todo);
}


ULONG VC_WriteBytes(SBYTE *buf, ULONG todo)

/*  Writes 'todo' mixed SBYTES (!!) to 'buf'. It returns the number of
//  SBYTES actually written to 'buf' (which is rounded to number of samples
//  that fit into 'todo' bytes).
*/

{
    if(vc_softchn == 0)
    {   VC_SilenceBytes(buf,todo);
        return todo;
    }

    todo = bytes2samples(todo);
    VC_WriteSamples(buf,todo);

    return samples2bytes(todo);
}


BOOL VC_Init(void)
{

#ifdef __FASTMIXER__
    int t;

    _mm_errno = MMERR_INITIALIZING_MIXER;
    if((voltab = (SLONG **)calloc(65,sizeof(SLONG *))) == NULL) return 1;
    for(t=0; t<65; t++)
       if((voltab[t] = (SLONG *)calloc(256,sizeof(SLONG))) == NULL) return 1;

    if((Samples = (SBYTE **)calloc(MAXSAMPLEHANDLES, sizeof(SBYTE *))) == NULL) return 1;
#else
    _mm_errno = MMERR_INITIALIZING_MIXER;
    if((Samples = (SWORD **)calloc(MAXSAMPLEHANDLES, sizeof(SWORD *))) == NULL) return 1;
#endif

    if(VC_TICKBUF==NULL) if((VC_TICKBUF=(SLONG *)malloc((TICKLSIZE+32) * sizeof(SLONG))) == NULL) return 1;

    MixReverb = (md_mode & DMODE_STEREO) ? MixReverb_Stereo : MixReverb_Normal;

    vc_mode = md_mode;

    _mm_errno = 0;
    return 0;
}


void VC_Exit(void)
{
#ifdef __FASTMIXER__
    int t;
    if(voltab!=NULL)
    {   for(t=0; t<65; t++) if(voltab[t]!=NULL) free(voltab[t]);
        free(voltab); voltab = NULL;
    }
#endif

    if(vinf!=NULL) free(vinf);
    if(Samples!=NULL) free(Samples);

    vinf            = NULL;
    Samples         = NULL;
}


BOOL VC_PlayStart(void)
{
    int    numchn;

    numchn = md_softchn;
#ifdef __FASTMIXER__
    if(numchn > 0)
    {
        int    c, t;
        SLONG  volmul;

        for(t=0; t<65; t++)
        {   volmul = (65536l*t) / 64;
            for(c=-128; c<128; c++)
                voltab[t][(UBYTE)c] = (SLONG)c*volmul;
        }
    }
#endif

    samplesthatfit = TICKLSIZE;
    if(vc_mode & DMODE_STEREO) samplesthatfit >>= 1;
    TICKLEFT = 0;

#ifdef __FAST_REVERB__
    RVc1 = (5000L * md_mixfreq) / (REVERBERATION * 2);
    RVc2 = (5946L * md_mixfreq) / (REVERBERATION * 2);
    RVc3 = (7071L * md_mixfreq) / (REVERBERATION * 2);
    RVc4 = (8409L * md_mixfreq) / (REVERBERATION * 2);
#else
    RVc1 = (5000L * md_mixfreq) / REVERBERATION;
    RVc2 = (5078L * md_mixfreq) / REVERBERATION;
    RVc3 = (5313L * md_mixfreq) / REVERBERATION;
    RVc4 = (5703L * md_mixfreq) / REVERBERATION;
    RVc5 = (6250L * md_mixfreq) / REVERBERATION;
    RVc6 = (6953L * md_mixfreq) / REVERBERATION;
    RVc7 = (7813L * md_mixfreq) / REVERBERATION;
    RVc8 = (8828L * md_mixfreq) / REVERBERATION;
#endif
   
    if((RVbuf1 = (SLONG *)_mm_calloc((RVc1+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf2 = (SLONG *)_mm_calloc((RVc2+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf3 = (SLONG *)_mm_calloc((RVc3+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf4 = (SLONG *)_mm_calloc((RVc4+1),sizeof(SLONG))) == NULL) return 1;
#ifndef __FAST_REVERB__
    if((RVbuf5 = (SLONG *)_mm_calloc((RVc5+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf6 = (SLONG *)_mm_calloc((RVc6+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf7 = (SLONG *)_mm_calloc((RVc7+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf8 = (SLONG *)_mm_calloc((RVc8+1),sizeof(SLONG))) == NULL) return 1;
#endif
    
    if((RVbuf9 = (SLONG *)_mm_calloc((RVc1+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf10 = (SLONG *)_mm_calloc((RVc2+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf11 = (SLONG *)_mm_calloc((RVc3+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf12 = (SLONG *)_mm_calloc((RVc4+1),sizeof(SLONG))) == NULL) return 1;
#ifndef __FAST_REVERB__
    if((RVbuf13 = (SLONG *)_mm_calloc((RVc5+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf14 = (SLONG *)_mm_calloc((RVc6+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf15 = (SLONG *)_mm_calloc((RVc7+1),sizeof(SLONG))) == NULL) return 1;
    if((RVbuf16 = (SLONG *)_mm_calloc((RVc8+1),sizeof(SLONG))) == NULL) return 1;
#endif

    RVRindex   = 0;

    return 0;
}


void VC_PlayStop(void)
{
    if(RVbuf1  != NULL) free(RVbuf1);
    if(RVbuf2  != NULL) free(RVbuf2);
    if(RVbuf3  != NULL) free(RVbuf3);
    if(RVbuf4  != NULL) free(RVbuf4);
    if(RVbuf9  != NULL) free(RVbuf9);
    if(RVbuf10 != NULL) free(RVbuf10);
    if(RVbuf11 != NULL) free(RVbuf11);
    if(RVbuf12 != NULL) free(RVbuf12);

    RVbuf1  = RVbuf2  = RVbuf3  = RVbuf4  = NULL;
    RVbuf9  = RVbuf10 = RVbuf11 = RVbuf12 = NULL;

#ifndef __FAST_REVERB__
    if(RVbuf5  != NULL) free(RVbuf5);
    if(RVbuf6  != NULL) free(RVbuf6);
    if(RVbuf7  != NULL) free(RVbuf7);
    if(RVbuf8  != NULL) free(RVbuf8);
    if(RVbuf13 != NULL) free(RVbuf13);
    if(RVbuf14 != NULL) free(RVbuf14);
    if(RVbuf15 != NULL) free(RVbuf15);
    if(RVbuf16 != NULL) free(RVbuf16);

    RVbuf13 = RVbuf14 = RVbuf15 = RVbuf16 = NULL;
    RVbuf5  = RVbuf6  = RVbuf7  = RVbuf8  = NULL;    
#endif

}


BOOL VC_SetNumVoices(void)
{
    int t;

    if((vc_softchn = md_softchn) == 0) return 0;

    if(vinf!=NULL) free(vinf);
    if((vinf = _mm_calloc(sizeof(VINFO),vc_softchn)) == NULL) return 1;
    
    for(t=0; t<vc_softchn; t++)
    {   vinf[t].frq = 10000;
        vinf[t].pan = (t&1) ? 0 : 255;
    }
    
    return 0;
}


void VC_VoiceSetVolume(UBYTE voice, UWORD vol)
{
    vinf[voice].vol = vol;
}


void VC_VoiceSetFrequency(UBYTE voice, ULONG frq)
{
    vinf[voice].frq = frq;
}


void VC_VoiceSetPanning(UBYTE voice, ULONG pan)
{
    vinf[voice].pan = pan;
}


void VC_VoicePlay(UBYTE voice, SWORD handle, ULONG start, ULONG size, ULONG reppos, ULONG repend, UWORD flags)
{
    vinf[voice].flags    = flags;
    vinf[voice].handle   = handle;
    vinf[voice].start    = start;
    vinf[voice].size     = size;
    vinf[voice].reppos   = reppos;
    vinf[voice].repend   = repend;
    vinf[voice].kick     = 1;
}


void VC_VoiceStop(UBYTE voice)
{
    vinf[voice].active = 0;
}  


BOOL VC_VoiceStopped(UBYTE voice)
{
    return(vinf[voice].active==0);
}


void VC_VoiceReleaseSustain(UBYTE voice)
{

}


SLONG VC_VoiceGetPosition(UBYTE voice)
{
    return(vinf[voice].current >> FRACBITS);
}


/**************************************************
***************************************************
***************************************************
**************************************************/


void VC_SampleUnload(SWORD handle)
{
    free(Samples[handle]);
    Samples[handle] = NULL;
}


SWORD VC_SampleLoad(SAMPLOAD *sload, int type)
{
    SAMPLE *s = sload->sample;
    int    handle;
    ULONG  t, length,loopstart,loopend;

    if(type==MD_HARDWARE) return -1;

    /* Find empty slot to put sample address in */

    for(handle=0; handle<MAXSAMPLEHANDLES; handle++)
        if(Samples[handle]==NULL) break;

    if(handle==MAXSAMPLEHANDLES)
    {   _mm_errno = MMERR_OUT_OF_HANDLES;
        return -1;
    }

    length    = s->length;
    loopstart = s->loopstart;
    loopend   = s->loopend;

    SL_SampleSigned(sload);

#ifdef __FASTMIXER__
    SL_Sample16to8(sload);
    if((Samples[handle]=(SBYTE *)malloc(length+20))==NULL)
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        return -1;
    }

    /* read sample into buffer. */
    SL_Load(Samples[handle],sload,length);
#else
    SL_Sample8to16(sload);
    if((Samples[handle]=(SWORD *)malloc((length+20)<<1))==NULL)
    {   _mm_errno = MMERR_SAMPLE_TOO_BIG;
        return -1;
    }

    /* read sample into buffer. */
    SL_Load(Samples[handle],sload,length);
#endif


    /* Unclick samples: */

    if(s->flags & SF_LOOP)
    {   if(s->flags & SF_BIDI)
            for(t=0; t<16; t++) Samples[handle][loopend+t] = Samples[handle][(loopend-t)-1];
        else
            for(t=0; t<16; t++) Samples[handle][loopend+t] = Samples[handle][t+loopstart];
    } else
        for(t=0; t<16; t++) Samples[handle][t+length] = 0;

    return handle;
}


ULONG VC_SampleSpace(int type)
{
    return vc_memory;
}


ULONG VC_SampleLength(int type, SAMPLE *s)
{
#ifdef __FASTMIXER__
    return s->length + 16;
#else
    return (s->length * ((s->flags&SF_16BITS) ? 2 : 1)) + 16;
#endif
}


/**************************************************
***************************************************
***************************************************
**************************************************/


ULONG VC_VoiceRealVolume(UBYTE voice)
{
    ULONG i,s,size;
    int k,j;
#ifdef __FASTMIXER__
    SBYTE *smp;
#else
    SWORD *smp;
#endif
    SLONG t;
                    
    t = vinf[voice].current>>FRACBITS;
    if(vinf[voice].active==0) return 0;

    s    = vinf[voice].handle;
    size = vinf[voice].size;

    i=64; t-=64; k=0; j=0;
    if(i>size) i = size;
    if(t<0) t = 0;
    if(t+i > size) t = size-i;

    i &= ~1;  /* make sure it's EVEN. */

    smp = &Samples[s][t];
    for(; i; i--, smp++)
    {   if(k<*smp) k = *smp;
        if(j>*smp) j = *smp;
    }

#ifdef __FASTMIXER__
    k = abs(k-j)<<8;
#else
    k = abs(k-j);
#endif

    return k;
}

