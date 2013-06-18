nodist_pkgdata_DATA = 
if WINE_PLUGIN
  nodist_pkgdata_DATA += Controller.exe
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif


Controller.exe : main.o linuxtrack.o resources.o dinput.o
	wineg++ -m32 -o Controller -Wall -Wextra  $(LDFLAGS) $(WINE_LIBS) \
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

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh


