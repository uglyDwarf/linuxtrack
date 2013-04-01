.PHONY : clean test all

all : Controller.exe

Controller.exe : main.o linuxtrack.o resources.o dinput.o
	wineg++ -m32 -o Controller -Wall -Wextra  $(LDFLAGS)\
          -L/usr/lib/i386-linux-gnu/ -L/usr/lib/i386-linux-gnu/wine \
           main.o linuxtrack.o resources.o dinput.o -lshell32 -ldxguid \
          -l dinput -l dinput8 -l uuid
          
main.o : main.cpp
	wineg++ -m32 -c -o main.o -Wall -Wextra -isystem/usr/include/wine/windows \
	   -I../../ $(CXXFLAGS) main.cpp

linuxtrack.o : ../../linuxtrack.c ../../linuxtrack.h
	winegcc -c -m32 -Wall -Wextra -o linuxtrack.o -I../../ $(CFLAGS) ../../linuxtrack.c

dinput.o : dinput.cpp kbi_interface.h
	wineg++ -m32 -c -Wall -Wextra -o dinput.o -isystem/usr/include/wine/windows \
	  -I../../ $(CXXFLAGS) dinput.cpp
	
resources.o : resources.rc
	wrc -o resources.o resources.rc

clean :
	@rm -rf *.exe* *.o

distclean :
	@rm -rf *.exe* *.o


