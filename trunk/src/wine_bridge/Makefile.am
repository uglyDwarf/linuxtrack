if WINE_PLUGIN

nodist_pkgdata_DATA = linuxtrack-wine.exe

.PHONY : bridge bridge64

if WINE64
  linuxtrack-wine.exe : bridge64
else
  linuxtrack-wine.exe : bridge
endif

bridge :
	$(MAKE) -C tester 
	$(MAKE) -C controller
	$(MAKE) -C client
	cp tester/Tester.exe* .
	cp controller/Controller.exe* .
	cp client/NPClient.dll.so NPClient.dll
	makensis ltr_wine.nsi

bridge64 :
	$(MAKE) -C tester Tester64.exe Tester.exe
	$(MAKE) -C controller
	$(MAKE) -C client NPClient64.dll.so NPClient.dll.so
	cp tester/Tester.exe* .
	cp tester/Tester64.exe* .
	cp client/NPClient.dll.so NPClient.dll
	cp client/NPClient64.dll.so NPClient64.dll
	cp controller/Controller.exe* .
	makensis ltr_wine64.nsi

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll*

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll*

endif

EXTRA_DIST = dumper/data.txt dumper/Makefile dumper/main.c client/NPClient_main.c client/Makefile \
client/NPClient.spec client/NPClient_dll.h client/rest.h client/rest.c \
client/NPClient.h ltr_wine.nsi README tester/npifc.c tester/rest.c tester/rest.h \
tester/Makefile tester/npview.rc tester/resource.h tester/main.cpp \
tester/npifc.h controller/resources.rc controller/Makefile controller/dinput.cpp \
controller/kbi_interface.h controller/linuxtrack1.ico controller/linuxtrack.ico controller/resource.h \
controller/main.cpp linuxtrack1.ico linuxtrack.ico


