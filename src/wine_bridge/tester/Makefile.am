nodist_pkgdata_DATA = 
if WINE_PLUGIN
  nodist_pkgdata_DATA += Tester.exe
if WINE64
  nodist_pkgdata_DATA += Tester64.exe
endif #WINE64
endif #WINE_PLUGIN

if DARWIN
  LDFLAGS += -Wl,-no_arch_warnings
else
  LDFLAGS += -Wl,--no-warn-search-mismatch
endif

Tester.exe : main.o npview.o rest.o npifc.o 
	wineg++ -m32 -g -o Tester -L. $(WINE_LIBS) $(LDFLAGS) -Wall -Wextra $^

Tester64.exe : main64.o npview.o rest64.o npifc64.o 
	wineg++ -g -o Tester64 -L. $(WINE64_LIBS) $(LDFLAGS) -Wall -Wextra $^

main.o : @srcdir@/main.cpp Makefile
	wineg++ -m32 -g -Wall -Wextra -c -o $@ $(CXXFLAGS) $<

main64.o : @srcdir@/main.cpp Makefile
	wineg++ -g -Wall -Wextra -c -o $@ $(CXXFLAGS) $<

npview.o : npview.rc
	wrc -o $@ -I @srcdir@ $<

rest.o : rest.c rest.h Makefile
	winegcc -m32 -g -Wall -Wextra -c -o $@ -I../../.. $(CFLAGS) -DHAVE_CONFIG_H $< 

rest64.o : rest.c rest.h Makefile
	winegcc -g -Wall -Wextra -c -o $@ -I../../.. $(CFLAGS) -DHAVE_CONFIG_H $< 

npifc.o : npifc.c npifc.h Makefile
	winegcc -m32 -g -Wall -Wextra -c -o $@ $(CFLAGS) $<

npifc64.o : npifc.c npifc.h Makefile
	winegcc -g -Wall -Wextra -c -o $@ $(CFLAGS) -DFOR_WIN64=1 $<

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll* *.sh *.o

EXTRA_DIST = main.cpp npifc.c npifc.h resource.h rest.c rest.h

