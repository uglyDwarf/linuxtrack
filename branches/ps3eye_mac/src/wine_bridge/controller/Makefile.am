noinst_SCRIPTS = 
if WINE_PLUGIN
  noinst_SCRIPTS += Controller.exe
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

CC = winegcc

CXX = wineg++

.cpp.o :
	$(CXX) -c $(CXXFLAGS_PRE) $(CXXFLAGS) $(CPPFLAGS) -m32 -o $@ $<

.c.o :
	$(CC) -c $(CFLAGS_PRE) $(CFLAGS) $(CPPFLAGS) -m32 -o $@ $<

.rc.o :
	wrc -o $@ $<

CXXFLAGS_PRE = -isystem/usr/include/wine/windows -DHAVE_CONFIG_H \
	   -I../../.. -I. -I@srcdir@/../.. -I@top_builddir@

CFLAGS_PRE = -I../.. -I../../.. -DHAVE_CONFIG_H \
	-I@srcdir@/../.. -I@top_builddir@

#VPATH = ../..:@srcdir@/../..:@top_builddir@:@srcdir@
vpath %.h @srcdir@/../..
vpath %.h @top_builddir@
vpath %.c @srcdir@
vpath %.c @srcdir@/../..

Controller.exe : main.o resources.o dinput.o linuxtrack.o
	wineg++ -o Controller   $(LDFLAGS) $(WINE_LIBS) -m32 \
           $^ -lshell32 -ldxguid -ldinput -ldinput8 -luuid -ldl

main.o : main.cpp @top_builddir@/config.h

linuxtrack.o : linuxtrack.c linuxtrack.h config.h 

dinput.o : dinput.cpp kbi_interface.h

resources.o : resources.rc


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

