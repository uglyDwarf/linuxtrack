
.PHONY : clean

#all : NPClient.dll.so NPClient64.dll.so

NPClient.dll.so : NPClient.spec NPClient_dll.o linuxtrack.o rest.o
	winegcc -m32 -L/usr/lib/i386-linux-gnu/ -L/usr/lib/i386-linux-gnu/wine \
	  $(LDFLAGS) -Wall -Wextra \
          -g -o NPClient.dll.so -shared NPClient.spec NPClient_dll.o linuxtrack.o rest.o

NPClient64.dll.so : NPClient.spec NPClient64_dll.o linuxtrack64.o rest64.o
	winegcc -L/usr/lib/x86_64-linux-gnu/ -L/usr/lib/x86_64-linux-gnu/wine \
	  $(LDFLAGS)  -Wall -Wextra \
          -g -o NPClient64.dll.so -shared NPClient.spec NPClient64_dll.o linuxtrack64.o rest64.o

NPClient_dll.o : NPClient_dll.h NPClient_main.c
	winegcc -c -m32 -g -Wall -Wextra -o NPClient_dll.o -I../../ $(CFLAGS) NPClient_main.c

NPClient64_dll.o : NPClient_dll.h NPClient_main.c
	winegcc -c -g -Wall -Wextra -o NPClient64_dll.o -I../../ $(CFLAGS) NPClient_main.c

linuxtrack.o : ../../linuxtrack.c
	winegcc -m32 -g -c -Wall -Wextra -o linuxtrack.o $(CFLAGS) ../../linuxtrack.c

linuxtrack64.o : ../../linuxtrack.c
	winegcc -g -c -Wall -Wextra -o linuxtrack64.o $(CFLAGS) ../../linuxtrack.c

rest.o : rest.c rest.h
	winegcc -m32 -g -Wall -Wextra -c -o rest.o -I ../../.. $(CFLAGS) -DHAVE_CONFIG_H rest.c

rest64.o : rest.c rest.h
	winegcc -g -Wall -Wextra -c -o rest64.o -I ../../.. $(CFLAGS) -DHAVE_CONFIG_H rest.c

clean :
	rm -rf *.o *.dll.so

distclean :
	rm -rf *.o *.dll.so

