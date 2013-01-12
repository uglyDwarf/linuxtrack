.PHONY : clean test all

all : Tester.exe

Tester.exe : main.o npview.o rest.o npifc.o 
	wineg++ -v -m32 -g -o Tester -L. -L/usr/lib/i386-linux-gnu/ \
          -L/usr/lib/i386-linux-gnu/wine main.o rest.o npview.o npifc.o

main.o : main.cpp Makefile
	wineg++ -m32 -g -c -o main.o main.cpp

npview.o : npview.rc
	wrc -o npview.o npview.rc

rest.o : rest.c rest.h Makefile
	winegcc -m32 -g -c -o rest.o rest.c 

npifc.o : npifc.c npifc.h Makefile
	winegcc -m32 -g -c -o npifc.o npifc.c

clean :
	@rm -rf *.exe* *.o


