if WINE_PLUGIN
  
  nodist_pkgdata_DATA = linuxtrack-wine.exe
  
#.PHONY: build_parts
#build_parts:

Tester.exe.so :
	$(MAKE) -C tester
	cp tester/Tester.exe* .

Controller.exe.so :
	$(MAKE) -C controller
	cp controller/Controller.exe* .

NPClient.dll :
	$(MAKE) -C client
	cp client/NPClient.dll.so NPClient.dll

data.txt :
	$(MAKE) -C dumper
	cp dumper/data.txt .

linuxtrack-wine.exe : Tester.exe.so Controller.exe.so NPClient.dll data.txt
	makensis ltr_wine.nsi

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C dumper clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll* data.txt

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C dumper clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll* data.txt

endif

EXTRA_DIST = dumper/data.txt dumper/Makefile dumper/main.c client/NPClient_main.c client/Makefile \
client/NPClient.spec client/NPClient_dll.h client/rest.h client/rest.c \
client/NPClient.h ltr_wine.nsi README tester/npifc.c tester/rest.c tester/rest.h \
tester/Makefile tester/npview.rc tester/resource.h tester/main.cpp \
tester/npifc.h controller/resources.rc controller/Makefile controller/dinput.cpp \
controller/kbi_interface.h controller/linuxtrack1.ico controller/linuxtrack.ico controller/resource.h \
controller/main.cpp linuxtrack1.ico linuxtrack.ico


