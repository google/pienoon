/*
    TDEFS.H   : Type definitions for the more commonly used type statements.
       A 'shortened' version of MTYPES.H that has only what I use most.

    This module is my hidden secret of "portability" to many compilers
    and platforms.  I really love C sometimes.. ;)
*/


#ifndef TDEFS_H
#define TDEFS_H


/*
        MikMod atomic types:
        ====================
*/

typedef char            CHAR;

#ifdef __OS2__

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
typedef signed long     SLONG;          /* has to be 4 bytes signed */
/* ULONG and BOOL are already defined in OS2.H */

#elif defined(__alpha)

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
/* long is 8 bytes on dec alpha - RCA */
typedef signed int      SLONG;          /* has to be 4 bytes signed */
typedef unsigned int    ULONG;          /* has to be 4 bytes unsigned */
typedef int             BOOL;           /* doesn't matter.. 0=FALSE, <>0 true */

#else

typedef signed char     SBYTE;          /* has to be 1 byte signed */
typedef unsigned char   UBYTE;          /* has to be 1 byte unsigned */
typedef signed short    SWORD;          /* has to be 2 bytes signed */
typedef unsigned short  UWORD;          /* has to be 2 bytes unsigned */
typedef signed long     SLONG;          /* has to be 4 bytes signed */
typedef unsigned long   ULONG;          /* has to be 4 bytes unsigned */
typedef int             BOOL;           /* doesn't matter.. 0=FALSE, <>0 true */

#endif


#ifdef __OS2__
#define INCL_DOS
#define INCL_MCIOS2
#define INCL_MMIOOS2
#include <os2.h>
#include <os2me.h>
#include <mmio.h>
#endif


#ifdef __WATCOMC__

typedef __int64          SDOUBLE;
typedef unsigned __int64 UDOUBLE;

#define inportb(x)     inp(x)
#define outportb(x,y)  outp(x,y)
#define inport(x)      inpw(x)
#define outport(x,y)   outpw(x,y)
#define disable()      _disable()
#define enable()       _enable()
#endif

#ifdef __BORLANDC__
#define inp(x)      inportb(x)
#define outp(x,y)   outportb(x,y)
#define inpw(x)     inport(x)
#define outpw(x,y)  outport(x,y)  
#define _disable()  disable() 
#define _enable()   enable()
#endif

#ifdef __DJGPP__
#include <dpmi.h>
#include <go32.h>
#include <pc.h>
#define inp       inportw
#define outport   outportw
#define inport    inportw
#define interrupt 
#endif

#if defined(__OS2__)
        #define delay(x)  DosSleep(x);
#elif defined(__WIN32__)
        #define delay(x)  Sleep(x);
#elif defined(__GNUC__)
        #define delay(x) ;
#endif

#endif

