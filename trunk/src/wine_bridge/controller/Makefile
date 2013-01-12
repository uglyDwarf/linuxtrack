.PHONY : clean test all

all : Controller.exe

Controller.exe : main.cpp linuxtrack.o resources.o dinput.o
	wineg++ -v -m32 -o Controller -isystem/usr/include/wine/windows \
          -L/usr/lib/i386-linux-gnu/ -L/usr/lib/i386-linux-gnu/wine \
          -I../../ linuxtrack.o resources.o dinput.o -lshell32 -ldxguid \
          -l dinput -l dinput8 -l uuid main.cpp

linuxtrack.o : ../../linuxtrack.c ../../linuxtrack.h
	winegcc -c -m32 -o linuxtrack.o -I../../ ../../linuxtrack.c

dinput.o : dinput.cpp kbi_interface.h
	wineg++ -m32 -c -o dinput.o -isystem/usr/include/wine/windows -I../../ dinput.cpp
	
resources.o : resources.rc
	wrc -o resources.o resources.rc

clean :
	@rm -rf *.exe* *.o


