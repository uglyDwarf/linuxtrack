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
	winegcc -m32 $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra \
          -g -o NPClient.dll.so -shared NPClient.spec NPClient_dll.o linuxtrack.o rest.o

NPClient64.dll.so : NPClient.spec NPClient64_dll.o linuxtrack64.o rest64.o
	winegcc  $(WINE64_LIBS) $(LDFLAGS)  -Wall -Wextra \
          -g -o NPClient64.dll.so -shared NPClient.spec NPClient64_dll.o linuxtrack64.o rest64.o

NPClient_dll.o : NPClient_dll.h NPClient_main.c
	winegcc -c -m32 -g -Wall -Wextra -o NPClient_dll.o -I../../ $(CFLAGS) NPClient_main.c

NPClient64_dll.o : NPClient_dll.h NPClient_main.c
	winegcc -c -g -Wall -Wextra -o NPClient64_dll.o -I../../ $(CFLAGS) NPClient_main.c

linuxtrack.o : ../../linuxtrack.c ../../../config.h
	winegcc -m32 -g -c -Wall -Wextra -o linuxtrack.o -DHAVE_CONFIG_H -I../../.. $(CFLAGS) ../../linuxtrack.c

linuxtrack64.o : ../../linuxtrack.c ../../../config.h
	winegcc -g -c -Wall -Wextra -o linuxtrack64.o -DHAVE_CONFIG_H -I../../.. $(CFLAGS) ../../linuxtrack.c

rest.o : rest.c rest.h ../../../config.h
	winegcc -m32 -g -Wall -Wextra -c -o rest.o -I ../../.. $(CFLAGS) -DHAVE_CONFIG_H rest.c

rest64.o : rest.c rest.h ../../../config.h
	winegcc -g -Wall -Wextra -c -o rest64.o -I ../../.. $(CFLAGS) -DHAVE_CONFIG_H rest.c

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh

EXTRA_DIST = NPClient_dll.h NPClient_main.c rest.c \
NPClient.h NPClient.spec rest.h

