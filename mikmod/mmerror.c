/*
 --> The MM_Error Portable Error Handling Functions
  -> Divine Entertainment GameDev Libraries

 File: MMERROR.C

 Description:
 Error handling functions for use with the DivEnt MMIO and MikMod
 libraries.  Register an error handler with _mm_RegisterErrorHandler()
 and you're all set.

 NOTES:

  - the global variables _mm_error, _mm_errno, and _mm_critical are
    set before the error handler in called.  See below for the values
    of these variables.

--------------------------


*/


#include "mmio.h"

CHAR *_mm_errmsg[] =
{
    "",

/* Generic MikMod Errors [referenced by _mm_error] */
/* ----------------------------------------------- */

    "Cannot open requested file",
    "Out of memory",
    "Unexpected end of file",
    "Cannot write to file - Disk full",

/* Specific Misceallenous Errors: */

    "Sample load failed - Out of memory",
    "Sample load failed - Out of sample handles",
    "Could not allocate page-contiguous dma-buffer",
    "Unknown wave file or sample type",
    "Unknown streaming audio type",

/* Specific Module Loader [MLOADER.C] errors: */

    "Failure loading module pattern",
    "Failure loading module track",
    "Failure loading module header",
    "Failure loading sampleinfo",
    "Unknown module format",

/* Specific Driver [MDRIVER.C / drivers] errors: */

    "None of the supported sound-devices were detected",
    "Device number out of range",
    "Software mixer failure - Out of memory",

#ifdef SUN
#elif defined(SOLARIS)
#elif defined(__alpha)
    "Cannot find suitable audio port!"
#elif defined(OSS)
    #ifdef ULTRA
    #endif
#elif defined(__hpux)
    "Unable to open /dev/audio",
    "Unable to set non-blocking mode for /dev/audio",
    "Unable to select 16bit-linear sample format",
    "Could not select requested sample-rate",
    "Could not select requested number of channels",
    "Unable to select audio output",
    "Unable to get audio description",
    "Unable to get gain values",
    "Unable to set gain values",
    "Could not set transmission buffer size"
#elif defined(AIX)
    "Could not open AIX audio device",
    "Configuration (init) of AIX audio device failed",
    "Configuration (control) of AIX audio device failed",
    "Configuration (start) of AIX audio device failed",
    "Unable to set non-blocking mode for /dev/audio",
#elif defined(SGI)
#elif defined(__OS2__)
#elif defined(__WIN32__)
#else
    "The requested soundcard was not found",
    "Could not open a High-DMA channel"
#endif
};


void (*_mm_errorhandler)(void) = NULL;
int   _mm_errno = 0;
BOOL  _mm_critical = 0;


void _mm_RegisterErrorHandler(void (*proc)(void))
{
    _mm_errorhandler = proc;
}

