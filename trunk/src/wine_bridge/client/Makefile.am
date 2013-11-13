noinst_SCRIPTS = 
if WINE_PLUGIN
  noinst_SCRIPTS += NPClient.dll.so check_data.exe.so
if WINE64
  noinst_SCRIPTS += NPClient64.dll.so
endif #WINE64
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

CC = winegcc

CXX = wineg++

SUFFIXES = .o .cpp .c .rc 64.o

.cpp.o :
	$(CXX) -m32 -c $(CXXFLAGS_PRE) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.c.o :
	$(CC) -m32 -c $(CFLAGS_PRE) $(CFLAGS) $(CPPFLAGS) -o $@ $<

.cpp64.o :
	$(CXX) -c $(CXXFLAGS_PRE) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.c64.o :
	$(CC) -c $(CFLAGS_PRE) $(CFLAGS) $(CPPFLAGS) -o $@ $<

.rc.o :
	wrc -o $@ $(RCFLAGS) $<

CXXFLAGS_PRE = -g -DHAVE_CONFIG_H -I../../.. -I. -I@srcdir@/../.. -I@top_builddir@
CFLAGS_PRE = -g -I../.. -I../../.. -DHAVE_CONFIG_H -I@srcdir@/../.. -I@top_builddir@
RCFLAGS = -I @srcdir@
VPATH = ../..:@srcdir@/../..:@top_builddir@:@srcdir@

check_data.exe.so : check_data.o rest.o
	winegcc -m32 $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra -g -o $@ $^

NPClient.dll.so : NPClient.spec NPClient_main.o linuxtrack.o rest.o
	winegcc -m32 $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra -g -o $@ -shared $^ -ldl

NPClient64.dll.so : NPClient.spec NPClient_main64.o linuxtrack64.o rest64.o
	winegcc  $(WINE64_LIBS) $(LDFLAGbS)  -Wall -Wextra -g -o $@ -shared $^ -ldl

check_data.o : check_data.c rest.h

NPClient_main.o : NPClient_main.c NPClient_dll.h

NPClient_main64.o : NPClient_main.c NPClient_dll.h

linuxtrack.o : linuxtrack.c config.h

linuxtrack64.o : linuxtrack.c config.h

rest.o : rest.c rest.h config.h

rest64.o : rest.c rest.h config.h

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = NPClient_dll.h NPClient_main.c rest.c \
NPClient.h NPClient.spec rest.h check_data.c

