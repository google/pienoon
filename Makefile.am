# Makefile.am for the SDL sample mixer library and players

lib_LTLIBRARIES = libSDLmixer.la

SUBDIRS = @MUSIC_SUBDIRS@
DIST_SUBIDRS = mikmod timidity

libSDLmixerincludedir = $(includedir)/SDL
libSDLmixerinclude_HEADERS =	\
	SDL_mixer.h

libSDLmixer_la_SOURCES =	\
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

libSDLmixer_la_LDFLAGS = 		\
        -release $(LT_RELEASE)	\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
libSDLmixer_la_LIBADD = $(MIKMOD_LIB) $(TIMIDITY_LIB)

bin_PROGRAMS = playwave playmus

playwave_LDADD = libSDLmixer.la
playmus_LDADD = libSDLmixer.la
