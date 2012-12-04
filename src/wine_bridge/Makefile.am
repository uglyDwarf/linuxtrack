if WINE_PLUGIN
  
  nodist_pkgdata_DATA = linuxtrack-wine.exe
  
.PHONY: build_parts
build_parts:
	make -C client
	make -C controller
	make -C dumper
	make -C tester

Tester.exe.so : tester/Tester.exe
	cp tester/Tester.exe* .

Controller.exe.so : controller/Controller.exe
	cp controller/Controller.exe* .

NPClient.dll : client/NPClient.dll.so
	cp client/NPClient.dll.so NPClient.dll

data.txt : dumper/data.txt
	cp dumper/data.txt .

linuxtrack-wine.exe : build_parts Tester.exe.so Controller.exe.so NPClient.dll data.txt
	makensis ltr_wine.nsi

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	make -C client clean
	make -C controller clean
	make -C dumper clean
	make -C tester clean
	rm -f *.exe* *.dll* data.txt

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	make -C client clean
	make -C controller clean
	make -C dumper clean
	make -C tester clean
	rm -f *.exe* *.dll* data.txt

endif

EXTRA_DIST = dumper/data.txt dumper/Makefile dumper/main.c client/NPClient_main.c client/Makefile \
client/NPClient.spec client/NPClient_dll.h client/rest.h client/rest.c \
client/NPClient.h ltr_wine.nsi README tester/npifc.c tester/rest.c tester/rest.h \
tester/Makefile tester/npview.rc tester/resource.h tester/main.cpp \
tester/npifc.h controller/resources.rc controller/Makefile controller/dinput.cpp \
controller/kbi_interface.h controller/linuxtrack1.ico controller/linuxtrack.ico controller/resource.h \
controller/main.cpp linuxtrack1.ico linuxtrack.ico


