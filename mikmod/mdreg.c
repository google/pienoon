/*

 Name:  MDREG.C

 Description:
 A single routine for registering all drivers in MikMod for the current
 platform.

 Portability:
 DOS, WIN95, OS2, SunOS, Solaris,
 Linux, HPUX, AIX, SGI, Alpha

 Anything not listed above is assumed to not be supported by this procedure!

 All Others: n

 - all compilers!

*/

#include "mikmod.h"

void MikMod_RegisterAllDrivers(void)
{

    MikMod_RegisterDriver(drv_sdl);
    MikMod_RegisterDriver(drv_nos);

}


