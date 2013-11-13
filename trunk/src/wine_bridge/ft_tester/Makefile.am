noinst_SCRIPTS = 
if WINE_PLUGIN
  noinst_SCRIPTS += ftc.exe.so
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

CC = winegcc

CXX = wineg++

SUFFIXES = .o .cpp .c .rc

.cpp.o :
	$(CXX) -m32 -c $(CXXFLAGS_PRE) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

.c.o :
	$(CC) -m32 -c $(CFLAGS_PRE) $(CFLAGS) $(CPPFLAGS) -o $@ $<

.rc.o :
	wrc -o $@ $(RCFLAGS) $<

CXXFLAGS_PRE = -g -DHAVE_CONFIG_H -I../../.. -I. -I@srcdir@/../.. -I@top_builddir@
CFLAGS_PRE = -g -I../.. -I../../.. -DHAVE_CONFIG_H -I@srcdir@/../.. -I@top_builddir@
RCFLAGS = -I @srcdir@
VPATH = ../..:@srcdir@/../..:@top_builddir@:@srcdir@

ftc.exe.so : main.o fttester.o
	wineg++ -m32 -g -o $@ -L. $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra $^

fttester.o : fttester.rc resource.h config.h

main.o : main.cpp

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = resource.h fttester.rc main.cpp

