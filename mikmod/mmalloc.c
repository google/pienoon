/*
 --> The MMIO Portable Memory Management functions
  -> Divine Entertainment GameDev Libraries

 Copyright © 1997 by Jake Stine and Divine Entertainment

*/

#include <string.h>
#include "mmio.h"


/* Same as malloc, but sets error variable _mm_error when it failed */
void *_mm_malloc(size_t size)
{
    void *d;

    if((d=malloc(size))==NULL)
    {   _mm_errno = MMERR_OUT_OF_MEMORY;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
    }

    return d;
}


/* Same as calloc, but sets error variable _mm_error when it failed */
void *_mm_calloc(size_t nitems, size_t size)
{
    void *d;
   
    if((d=calloc(nitems,size))==NULL)
    {   _mm_errno = MMERR_OUT_OF_MEMORY;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
    }

    return d;
}


#ifndef __WATCOMC__
#ifndef __GNUC__
#ifndef __BEOS__
#ifndef _SGI_SOURCE
/* Same as Watcom's strdup function - allocates memory for a string */
/* and makes a copy of it.  Ruturns NULL if failed. */
CHAR *strdup(CHAR *src)
{
    CHAR *buf;

    if((buf = (CHAR *)_mm_malloc(strlen(src)+1)) == NULL)
    {   _mm_errno = MMERR_OUT_OF_MEMORY;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
        return NULL;
    }

    strcpy(buf,src);
    return buf;
}
#endif
#endif
#endif
#endif
