/*

 Name:  MLREG.C

 Description:
 A single routine for registering all loaders in MikMod for the current
 platform.

 Portability:
 All systems - all compilers

*/

#include "mikmod.h"

void MikMod_RegisterAllLoaders(void)
{
   MikMod_RegisterLoader(load_it);
   MikMod_RegisterLoader(load_xm);
   MikMod_RegisterLoader(load_s3m);
   MikMod_RegisterLoader(load_mod);
}
