/*

Name:
S3M_IT.C

Description:
Commonfunctions between S3Ms and ITs  (smaller .EXE! :)

Portability:
All systems - all compilers (hopefully)

*/

#include "mikmod.h"

UBYTE  *poslookup=NULL;/* S3M/IT fix - removing blank patterns needs a */
                       /* lookup table to fix position-jump commands */
SBYTE   remap[64];     /* for removing empty channels */
SBYTE   isused[64];    /* for removing empty channels */


void S3MIT_ProcessCmd(UBYTE cmd,UBYTE inf,BOOL oldeffect)
{
    UBYTE hi,lo;

    lo=inf&0xf;
    hi=inf>>4;

    /* process S3M / IT specific command structure */

   if(cmd!=255)
   {   switch(cmd)
       {   case 1:                 /* Axx set speed to xx */
              UniWrite(UNI_S3MEFFECTA);
              UniWrite(inf);
           break;

           case 2:                 /* Bxx position jump */
              UniPTEffect(0xb,poslookup[inf]);
           break;

           case 3:                 /* Cxx patternbreak to row xx */
               if(oldeffect & 1)
                   UniPTEffect(0xd,(((inf&0xf0)>>4)*10)+(inf&0xf));
               else
                   UniPTEffect(0xd,inf);
           break;

           case 4:                 /* Dxy volumeslide */
               UniWrite(UNI_S3MEFFECTD);
               UniWrite(inf);
           break;

           case 5:                 /* Exy toneslide down */
               UniWrite(UNI_S3MEFFECTE);
               UniWrite(inf);
           break;

           case 6:                 /* Fxy toneslide up */
               UniWrite(UNI_S3MEFFECTF);
               UniWrite(inf);
           break;
                 
           case 7:                 /* Gxx Tone portamento, speed xx */
               UniWrite(UNI_ITEFFECTG);
               UniWrite(inf);
           break;

           case 8:                 /* Hxy vibrato */
               if(oldeffect & 1)
                  UniPTEffect(0x4,inf);
               else
               {   UniWrite(UNI_ITEFFECTH);
                   UniWrite(inf);
               }
           break;

           case 9:                 /* Ixy tremor, ontime x, offtime y */
               if(oldeffect & 1)
                   UniWrite(UNI_S3MEFFECTI);
               else                     
                   UniWrite(UNI_ITEFFECTI);
               UniWrite(inf);
           break;

           case 0xa:               /* Jxy arpeggio */
               UniPTEffect(0x0,inf);
           break;

           case 0xb:               /* Kxy Dual command H00 & Dxy */
               if(oldeffect & 1)
                   UniPTEffect(0x4,0);    
               else
               {   UniWrite(UNI_ITEFFECTH);
                   UniWrite(0);
               }
               UniWrite(UNI_S3MEFFECTD);
               UniWrite(inf);
           break;

           case 0xc:               /* Lxy Dual command G00 & Dxy */
               if(oldeffect & 1)
                  UniPTEffect(0x3,0);
               else
               {   UniWrite(UNI_ITEFFECTG);
                   UniWrite(0);
               }
               UniWrite(UNI_S3MEFFECTD);
               UniWrite(inf);
           break;

           case 0xd:               /* Mxx Set Channel Volume */
               UniWrite(UNI_ITEFFECTM);
               UniWrite(inf);
           break;       

           case 0xe:               /* Nxy Slide Channel Volume */
               UniWrite(UNI_ITEFFECTN);
               UniWrite(inf);
           break;

           case 0xf:               /* Oxx set sampleoffset xx00h */
               UniPTEffect(0x9,inf);
           break;

           case 0x10:              /* Pxy Slide Panning Commands */
               UniWrite(UNI_ITEFFECTP);
               UniWrite(inf);
           break;

           case 0x11:              /* Qxy Retrig (+volumeslide) */
               UniWrite(UNI_S3MEFFECTQ);
               if((inf!=0) && (lo==0) && !(oldeffect & 1))
                   UniWrite(1);
               else
                   UniWrite(inf); 
           break;

           case 0x12:              /* Rxy tremolo speed x, depth y */
               UniWrite(UNI_S3MEFFECTR);
               UniWrite(inf);
           break;

           case 0x13:              /* Sxx special commands */
               UniWrite(UNI_ITEFFECTS0);
               UniWrite(inf);
           break;

           case 0x14:      /* Txx tempo */
               if(inf>0x20)
               {   UniWrite(UNI_S3MEFFECTT);
                   UniWrite(inf);
               }
           break;

           case 0x15:      /* Uxy Fine Vibrato speed x, depth y */
               if(oldeffect & 1)
                   UniWrite(UNI_S3MEFFECTU);
               else
                   UniWrite(UNI_ITEFFECTU);
               UniWrite(inf);
           break;

           case 0x16:      /* Vxx Set Global Volume */
               UniWrite(UNI_XMEFFECTG);
               UniWrite(inf);
           break;

           case 0x17:      /* Wxy Global Volume Slide */
               UniWrite(UNI_ITEFFECTW);
               UniWrite(inf);  
           break;

           case 0x18:      /* Xxx amiga command 8xx */
               if(oldeffect & 1)
                   UniPTEffect(0x8,((inf<<1) == 256) ? 255 : (inf<<1));
               else
                   UniPTEffect(0x8,inf);                                          
           break;

           case 0x19:      /* Yxy Panbrello  speed x, depth y */
               UniWrite(UNI_ITEFFECTY);
               UniWrite(inf);
           break;
       }
   }
}
