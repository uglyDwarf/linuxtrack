noinst_SCRIPTS = 
if WINE_PLUGIN
  noinst_SCRIPTS += FreeTrackClient.dll.so
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

CC = winegcc

CXX = wineg++

SUFFIXES = .o .cpp .c .rc

.cpp.o :
	$(CXX) -c $(CXXFLAGS_PRE) $(CXXFLAGS) $(CPPFLAGS) -m32 -o $@ $<

.c.o :
	$(CC) -c $(CFLAGS_PRE) $(CFLAGS) $(CPPFLAGS) -m32 -o $@ $<

CXXFLAGS_PRE = -g -DHAVE_CONFIG_H -I../../.. -I. -I@srcdir@/../.. -I@top_builddir@
CFLAGS_PRE = -g -I../.. -I../../.. -DHAVE_CONFIG_H -I@srcdir@/../.. -I@top_builddir@
RCFLAGS = -I @srcdir@
#VPATH = ../..:@srcdir@/../..:@top_builddir@:@srcdir@
vpath %.h @srcdir@/../..
vpath %.h @top_builddir@
vpath %.c @srcdir@
vpath %.c @srcdir@/../..

FreeTrackClient.dll.so : FreeTrackClient.spec FreeTrackClient_main.o linuxtrack.o
	winegcc $(WINE_LIBS) $(LDFLAGS) -m32 -Wall -Wextra -g -o $@ -shared $^ -ldl

FreeTrackClient_main.o : FreeTrackClient_main.c FreeTrackClient_dll.h

linuxtrack.o : linuxtrack.c linuxtrack.h


clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = FreeTrackClient_main.c FreeTrackClient_dll.h \
FreeTrackClient.spec

