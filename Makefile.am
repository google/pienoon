# Makefile.am for the SDL sample mixer library and players

lib_LTLIBRARIES = libmixer.la

SUBDIRS = @MUSIC_SUBDIRS@
DIST_SUBIDRS = mikmod timidity

libmixerincludedir = $(includedir)
libmixerinclude_HEADERS =	\
	mixer.h

libmixer_la_SOURCES =	\
	mixer.c			\
	music.c			\
	music_cmd.c		\
	music_cmd.h		\
	wave.h			\
	wavestream.c		\
	wavestream.h

if USE_MIKMOD
MIKMOD_LIB = mikmod/libmikmod.la
else
MIKMOD_LIB =
endif
if USE_TIMIDITY
TIMIDITY_LIB = timidity/libtimidity.la
else
TIMIDITY_LIB =
endif

libmixer_la_LDFLAGS = 		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libmixer_la_LIBADD = $(MIKMOD_LIB) $(TIMIDITY_LIB)

bin_PROGRAMS = playwave playmus

playwave_LDADD = libmixer.la
playmus_LDADD = libmixer.la
