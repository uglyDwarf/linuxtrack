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
           $^ -lshell32 -ldxguid -l dinput -l dinput8 -l uuid

main.o : main.cpp @top_builddir@/config.h
	wineg++ -m32 -c -o $@ -Wall -Wextra -isystem/usr/include/wine/windows \
	   -I../../.. -I. -I@srcdir@/../.. -I@top_builddir@ $(CXXFLAGS) -DHAVE_CONFIG_H $<

linuxtrack.o : ../../linuxtrack.c ../../linuxtrack.h @top_builddir@/config.h 
	winegcc -c -m32 -Wall -Wextra -o $@ -I../.. -I../../.. -DHAVE_CONFIG_H \
	-I@srcdir@/../.. -I@top_builddir@ $(CFLAGS) $<

dinput.o : dinput.cpp kbi_interface.h
	wineg++ -m32 -c -Wall -Wextra -o $@ -isystem/usr/include/wine/windows \
	  -I@srcdir@/../.. $(CXXFLAGS) $<

resources.o : resources.rc
	wrc -o resources.o $<

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = linuxtrack1.ico linuxtrack.ico dinput.cpp kbi_interface.h \
main.cpp resource.h resources.rc

