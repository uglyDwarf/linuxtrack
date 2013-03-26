.PHONY : clean test all

#all : Tester.exe Tester64.exe

Tester.exe : main.o npview.o rest.o npifc.o 
	wineg++ -m32 -g -o Tester -L. -L/usr/lib/i386-linux-gnu/ \
	  -Wl,--no-warn-search-mismatch -Wall -Wextra \
          -L/usr/lib/i386-linux-gnu/wine main.o rest.o npview.o npifc.o

Tester64.exe : main64.o npview.o rest64.o npifc64.o 
	wineg++ -g -o Tester64 -L. -L/usr/lib/x86_64-linux-gnu/ \
	  -Wl,--no-warn-search-mismatch -Wall -Wextra \
          -L/usr/lib/x86_64-linux-gnu/wine main64.o rest64.o npview.o npifc64.o

main.o : main.cpp Makefile
	wineg++ -m32 -g -Wall -Wextra -c -o main.o main.cpp

main64.o : main.cpp Makefile
	wineg++ -g -Wall -Wextra -c -o main64.o main.cpp

npview.o : npview.rc
	wrc -o npview.o npview.rc

rest.o : rest.c rest.h Makefile
	winegcc -m32 -g -Wall -Wextra -c -o rest.o rest.c 

rest64.o : rest.c rest.h Makefile
	winegcc -g -Wall -Wextra -c -o rest64.o rest.c 

npifc.o : npifc.c npifc.h Makefile
	winegcc -m32 -g -Wall -Wextra -c -o npifc.o npifc.c

npifc64.o : npifc.c npifc.h Makefile
	winegcc -g -Wall -Wextra -c -o npifc64.o -DFOR_WIN64=1 npifc.c

clean :
	@rm -rf *.exe* *.o

distclean :
	@rm -rf *.exe* *.o


