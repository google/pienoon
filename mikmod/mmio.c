/*
 --> The MMIO Portable Input/Output functions
  -> Divine Entertainment GameDev Libraries

 File: MMIO.C

 Description:
 Miscellaneous I/O routines.. used to solve some portability issues
 (like big/little endian machines and word alignment in structures )
 Also includes mikmod's ingenious error handling variable.

 Portability:
 All systems - all compilers


 -----------------------------------
 The way this module works - By Jake Stine [Air Richter]

 - _mm_fopen and _mm_copyfile will call the errorhandler [see mmerror.c] in
   addition to setting _mm_errno on exit.

 - _mm_iobase is for internal use.  It is used by ML_LoadFP() to ensure that it
   works properly with wad files.

 - _mm_read_I_UWORD and _mm_read_M_UWORD have distinct differences:
   the first is for reading data written by a little endian (intel) machine,
   and the second is for reading big endian (Mac, RISC, Alpha) machine data.

 - _mm_write functions work the same as the _mm_read functions.

 - _mm_read_string is for reading binary strings.  It is basically the same
   as an fread of bytes.
*/                                                                                     

#include <stdio.h>
#include "mmio.h"
#include <string.h>

#define COPY_BUFSIZE  1024

static long _mm_iobase  = 0,
            temp_iobase = 0;

UBYTE  _mm_cpybuf[COPY_BUFSIZE];


void StringWrite(CHAR  *s, FILE *fp)
/* Specialized file output procedure.  Writes a UWORD length and then a */
/* string of the specified length (no NULL terminator) afterward. */
{
    int slen;

    if(s==NULL)
    {   _mm_write_I_UWORD(0,fp);
    } else
    {   _mm_write_I_UWORD(slen = strlen(s),fp);
        _mm_write_UBYTES(s,slen,fp);
    }
}

CHAR *StringRead(FILE *fp)
/* Reads strings written out by StringWrite above:  a UWORD length followed */
/* by length characters.  A NULL is added to the string after loading. */
{
    CHAR  *s;
    UWORD len;

    len = _mm_read_I_UWORD(fp);
    if(len==0)
    {   s = _mm_calloc(16, sizeof(CHAR));
    } else
    {   if((s = (CHAR *)_mm_malloc(len+1)) == NULL) return NULL;
        _mm_read_UBYTES(s,len,fp);
        s[len] = 0;
    }

    return s;
}


FILE *_mm_fopen(CHAR *fname, CHAR *attrib)
{
    FILE *fp;

    if((fp=fopen(fname,attrib)) == NULL)
    {   _mm_errno = _mm_errno = MMERR_OPENING_FILE;
        if(_mm_errorhandler!=NULL) _mm_errorhandler();
    }
    return fp;
}


int _mm_fseek(FILE *stream, long offset, int whence)
{
   return fseek(stream,(whence==SEEK_SET) ? offset+_mm_iobase : offset, whence);
}


long _mm_ftell(FILE *stream)
{
   return ftell(stream)-_mm_iobase;
}


BOOL _mm_FileExists(CHAR *fname)
{
   FILE *fp;
   
   if((fp=fopen(fname,"r")) == NULL) return 0;
   fclose(fp);

   return 1;
}


long _mm_flength(FILE *stream)
{
   long tmp,tmp2;

   tmp = ftell(stream);
   fseek(stream,0,SEEK_END);
   tmp2 = ftell(stream);
   fseek(stream,tmp,SEEK_SET);
   return tmp2-tmp;
}


long _mm_iobase_get(void)
{
   return _mm_iobase;
}


void _mm_iobase_set(long iobase)
{
   temp_iobase = _mm_iobase;    /* store old value in case of revert */
   _mm_iobase  = iobase;   
}


/* Sets the current file-position as the new _mm_iobase */
void _mm_iobase_setcur(FILE *fp)
{
   temp_iobase = _mm_iobase;    /* store old value in case of revert */
   _mm_iobase  = ftell(fp);
}


/* Reverts to the last known _mm_iobase value. */
void _mm_iobase_revert(void)
{
   _mm_iobase  = temp_iobase;
}


/* Procedure: _mm_copyfile */
/* Copies a given number of bytes from the source file to the destination */
/* file.  Returns 1 on error, and calls the _mm_errnohandler, if registered. */
BOOL _mm_copyfile(FILE *fpi, FILE *fpo, ULONG len)
{
    ULONG todo;
   
    while(len)
    {   todo = (len > COPY_BUFSIZE) ? COPY_BUFSIZE : len;
        if(!fread(_mm_cpybuf, todo, 1, fpi))
        {   _mm_errno = _mm_errno = MMERR_END_OF_FILE;
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            return 1;
        }
        if(!fwrite(_mm_cpybuf, todo, 1, fpo))
        {   _mm_errno = _mm_errno = MMERR_DISK_FULL;
            if(_mm_errorhandler!=NULL) _mm_errorhandler();
            return 1;
        }
        len -= todo;
    }

    return 0;
}


void _mm_write_string(CHAR *data, FILE *fp)
{
    if(data!=NULL)
       _mm_write_UBYTES(data, strlen(data), fp);
}


void _mm_fputs(FILE *fp, CHAR *data)
{
   if(data != NULL)
      _mm_write_UBYTES(data, strlen(data), fp);

#ifndef __UNIX__
   _mm_write_UBYTE(13,fp);
#endif
   _mm_write_UBYTE(10,fp);
}



#if 0
/* These have accompanying #define's in mmio.h */

void _mm_write_SBYTE(SBYTE data, FILE *fp)
{
   fputc(data,fp);
}

void _mm_write_UBYTE(UBYTE data,FILE *fp)
{
   fputc(data,fp);
}
#endif


#ifdef MM_BIG_ENDIAN

/* --> Big Endian Write Functions */

void _mm_write_M_UWORD(UWORD data,FILE *fp)
{
   _mm_write_UBYTE(data&0xff,fp);
   _mm_write_UBYTE(data>>8,fp);
}


void _mm_write_I_UWORD(UWORD data,FILE *fp)
{
   _mm_write_UBYTE(data>>8,fp);
   _mm_write_UBYTE(data&0xff,fp);
}


void _mm_write_M_ULONG(ULONG data,FILE *fp)
{
   _mm_write_M_UWORD(data&0xffff,fp);
   _mm_write_M_UWORD(data>>16,fp);
}


void _mm_write_I_ULONG(ULONG data,FILE *fp)
{
   _mm_write_I_UWORD(data>>16,fp);
   _mm_write_I_UWORD(data&0xffff,fp);
}

#else

/* --> Little Endian Write Functions */

void _mm_write_M_UWORD(UWORD data,FILE *fp)
{
   _mm_write_UBYTE(data>>8,fp);
   _mm_write_UBYTE(data&0xff,fp);
}


void _mm_write_I_UWORD(UWORD data,FILE *fp)
{
   _mm_write_UBYTE(data&0xff,fp);
   _mm_write_UBYTE(data>>8,fp);
}


void _mm_write_M_ULONG(ULONG data,FILE *fp)
{
   _mm_write_M_UWORD(data>>16,fp);
   _mm_write_M_UWORD(data&0xffff,fp);
}


void _mm_write_I_ULONG(ULONG data,FILE *fp)
{
   _mm_write_I_UWORD(data&0xffff,fp);
   _mm_write_I_UWORD(data>>16,fp);
}

#endif


void _mm_write_M_SWORD(SWORD data,FILE *fp)
{
   _mm_write_M_UWORD((UWORD)data,fp);
}


void _mm_write_I_SWORD(SWORD data,FILE *fp)
{
   _mm_write_I_UWORD((UWORD)data,fp);
}


void _mm_write_M_SLONG(SLONG data,FILE *fp)
{
   _mm_write_M_ULONG((ULONG)data,fp);
}


void _mm_write_I_SLONG(SLONG data,FILE *fp)
{
   _mm_write_I_ULONG((ULONG)data,fp);
}


#define DEFINE_MULTIPLE_WRITE_FUNCTION(type_name, type)       \
void                                                          \
_mm_write_##type_name##S (type *buffer, int number, FILE *fp) \
{                                                             \
   while(number>0)                                            \
   {  _mm_write_##type_name(*(buffer++),fp);                  \
      number--;                                               \
   }                                                          \
}

/*DEFINE_MULTIPLE_WRITE_FUNCTION (SBYTE, SBYTE) */
/*DEFINE_MULTIPLE_WRITE_FUNCTION (UBYTE, UBYTE) */

DEFINE_MULTIPLE_WRITE_FUNCTION (M_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION (M_UWORD, UWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION (I_SWORD, SWORD)
DEFINE_MULTIPLE_WRITE_FUNCTION (I_UWORD, UWORD)

DEFINE_MULTIPLE_WRITE_FUNCTION (M_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION (M_ULONG, ULONG)
DEFINE_MULTIPLE_WRITE_FUNCTION (I_SLONG, SLONG)
DEFINE_MULTIPLE_WRITE_FUNCTION (I_ULONG, ULONG)


/**********
SBYTE _mm_read_SBYTE(FILE *fp)
{
   return(fgetc(fp));
}

UBYTE _mm_read_UBYTE(FILE *fp)
{
   return(fgetc(fp));
}
**********/


#ifdef MM_BIG_ENDIAN

UWORD _mm_read_I_UWORD(FILE *fp)
{
   UWORD result=((UWORD)_mm_read_UBYTE(fp))<<8;
   result|=_mm_read_UBYTE(fp);
   return result;
}

UWORD _mm_read_M_UWORD(FILE *fp)
{
   UWORD result=_mm_read_UBYTE(fp);
   result|=((UWORD)_mm_read_UBYTE(fp))<<8;
   return result;
}

ULONG _mm_read_I_ULONG(FILE *fp)
{
   ULONG result=((ULONG)_mm_read_M_UWORD(fp))<<16;
   result|=_mm_read_M_UWORD(fp);
   return result;
}

ULONG _mm_read_M_ULONG(FILE *fp)
{
   ULONG result=_mm_read_I_UWORD(fp);
   result|=((ULONG)_mm_read_I_UWORD(fp))<<16;
   return result;
}

#else

UWORD _mm_read_M_UWORD(FILE *fp)
{
   UWORD result=((UWORD)_mm_read_UBYTE(fp))<<8;
   result|=_mm_read_UBYTE(fp);
   return result;
}

UWORD _mm_read_I_UWORD(FILE *fp)
{
   UWORD result=_mm_read_UBYTE(fp);
   result|=((UWORD)_mm_read_UBYTE(fp))<<8;
   return result;
}

ULONG _mm_read_M_ULONG(FILE *fp)
{
   ULONG result=((ULONG)_mm_read_M_UWORD(fp))<<16;
   result|=_mm_read_M_UWORD(fp);
   return result;
}

ULONG _mm_read_I_ULONG(FILE *fp)
{
   ULONG result=_mm_read_I_UWORD(fp);
   result|=((ULONG)_mm_read_I_UWORD(fp))<<16;
   return result;
}

#endif

SWORD _mm_read_M_SWORD(FILE *fp)
{
   return((SWORD)_mm_read_M_UWORD(fp));
}

SWORD _mm_read_I_SWORD(FILE *fp)
{
   return((SWORD)_mm_read_I_UWORD(fp));
}

SLONG _mm_read_M_SLONG(FILE *fp)
{
   return((SLONG)_mm_read_M_ULONG(fp));
}

SLONG _mm_read_I_SLONG(FILE *fp)
{
   return((SLONG)_mm_read_I_ULONG(fp));
}


int _mm_read_string(CHAR *buffer, int number, FILE *fp)
{
    fread(buffer,1,number,fp);
    return !feof(fp);
}



#define DEFINE_MULTIPLE_READ_FUNCTION(type_name, type)       \
int                                                          \
_mm_read_##type_name##S (type *buffer, int number, FILE *fp) \
{                                                            \
   while(number>0)                                           \
   {  *(buffer++)=_mm_read_##type_name(fp);                  \
      number--;                                              \
   }                                                         \
   return !feof(fp);                                         \
}

/*DEFINE_MULTIPLE_READ_FUNCTION (SBYTE, SBYTE) */
/*DEFINE_MULTIPLE_READ_FUNCTION (UBYTE, UBYTE) */

DEFINE_MULTIPLE_READ_FUNCTION (M_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION (M_UWORD, UWORD)
DEFINE_MULTIPLE_READ_FUNCTION (I_SWORD, SWORD)
DEFINE_MULTIPLE_READ_FUNCTION (I_UWORD, UWORD)

DEFINE_MULTIPLE_READ_FUNCTION (M_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION (M_ULONG, ULONG)
DEFINE_MULTIPLE_READ_FUNCTION (I_SLONG, SLONG)
DEFINE_MULTIPLE_READ_FUNCTION (I_ULONG, ULONG)

