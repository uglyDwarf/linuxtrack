
.PHONY : clean

all : NPClient.dll.so

NPClient.dll.so : NPClient.spec NPClient_dll.o linuxtrack.o rest.o
	winegcc -v -m32 -g -o NPClient.dll.so -shared NPClient.spec NPClient_dll.o linuxtrack.o rest.o

NPClient_dll.o : NPClient_dll.h NPClient_main.c
	winegcc -c -m32 -g -o NPClient_dll.o -I../../ NPClient_main.c

linuxtrack.o : ../../linuxtrack.c
	winegcc -m32 -g -c -o linuxtrack.o ../../linuxtrack.c

rest.o : rest.c rest.h
	winegcc -m32 -g -c -o rest.o rest.c

clean :
	rm -rf *.o NPClient.dll.so

