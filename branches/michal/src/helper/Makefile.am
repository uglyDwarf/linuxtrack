l32libdir = $(prefix)/lib32

if ! DARWIN
  LINUXFLAGS = '-Wl,-rpath,$(pkglibdir)' '-DLIB_PATH="$(pkglibdir)/"'
if X64

  l32lib_LTLIBRARIES = liblinuxtrack.la
endif
endif

liblinuxtrack_la_SOURCES = ../ltlib.c ../linuxtrack.h ../utils.c ../utils.h \
  ../ipc_utils.c ../ipc_utils.h
liblinuxtrack_la_CFLAGS = $(LINUXFLAGS) -m32 -I..
liblinuxtrack_la_LDFLAGS = -export-symbols "${srcdir}/../liblt.sym" -m32 $(LINUXFLAGS)
