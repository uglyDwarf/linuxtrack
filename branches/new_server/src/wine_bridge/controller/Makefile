.PHONY : clean test all

all : Controller.exe

Controller.exe : main.cpp linuxtrack.o
	wineg++ -m32 -o Controller -I../../ linuxtrack.o main.cpp

linuxtrack.o : ../../linuxtrack.c ../../linuxtrack.h
	winegcc -c -m32 -o linuxtrack.o -I../../ ../../linuxtrack.c

clean :
	@rm -rf *.exe* *.o


