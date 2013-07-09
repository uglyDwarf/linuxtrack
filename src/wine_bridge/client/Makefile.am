nodist_pkgdata_DATA = 
if WINE_PLUGIN
  nodist_pkgdata_DATA += NPClient.dll.so
if WINE64
  nodist_pkgdata_DATA += NPClient64.dll.so
endif #WINE64
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

NPClient.dll.so : NPClient.spec NPClient_dll.o linuxtrack.o rest.o
	winegcc -m32 $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra -g -o $@ -shared $^

NPClient64.dll.so : NPClient.spec NPClient64_dll.o linuxtrack64.o rest64.o
	winegcc  $(WINE64_LIBS) $(LDFLAGbS)  -Wall -Wextra -g -o $@ -shared $^

NPClient_dll.o : NPClient_main.c NPClient_dll.h
	winegcc -c -m32 -g -Wall -Wextra -o $@ -I@srcdir@/../.. $(CFLAGS) $<

NPClient64_dll.o : NPClient_main.c NPClient_dll.h
	winegcc -c -g -Wall -Wextra -o $@ -I@srcdir@/../.. $(CFLAGS) $<

linuxtrack.o : ../../linuxtrack.c @top_builddir@/config.h
	winegcc -m32 -g -c -Wall -Wextra -o $@ -DHAVE_CONFIG_H -I../.. -I../../.. \
	  -I@srcdir@/../.. $(CFLAGS) $<

linuxtrack64.o : ../../linuxtrack.c @top_builddir@/config.h
	winegcc -g -c -Wall -Wextra -o $@ -DHAVE_CONFIG_H -I../.. -I../../.. \
	  -I@srcdir@/../.. $(CFLAGS) $<

rest.o : rest.c rest.h @top_builddir@/config.h
	winegcc -m32 -g -Wall -Wextra -c -o rest.o -I @srcdir@/../../.. -I../../.. \
	  $(CFLAGS) -DHAVE_CONFIG_H $<

rest64.o : rest.c rest.h @top_builddir@/config.h
	winegcc -g -Wall -Wextra -c -o $@ -I @srcdir@/../../.. -I../../.. \
	  $(CFLAGS) -DHAVE_CONFIG_H $<

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = NPClient_dll.h NPClient_main.c rest.c \
NPClient.h NPClient.spec rest.h

