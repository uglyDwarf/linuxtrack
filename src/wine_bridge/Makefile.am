if WINE_PLUGIN

nodist_pkgdata_DATA = linuxtrack-wine.exe


if DARWIN
  MYLDFLAGS = -Wl,-no_arch_warnings
  linuxtrack-wine.exe : bridge_mac
else
  MYLDFLAGS = -Wl,--no-warn-search-mismatch

if WINE64
  linuxtrack-wine.exe : bridge64
else
  linuxtrack-wine.exe : bridge
endif #WINE64

endif #DARWIN

.PHONY : bridge_files bridge64_files

bridge_mac : bridge_files
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib Tester.exe 
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib Controller.exe 
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib NPClient.dll 
	makensis ltr_wine.nsi

bridge : bridge_files
	makensis ltr_wine.nsi

bridge64 : bridge64_files
	makensis ltr_wine64.nsi


bridge_files :
	$(MAKE) -C tester CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	$(MAKE) -C controller CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	$(MAKE) -C client CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	cp tester/Tester.exe.so Tester.exe
	cp tester/Tester.exe Tester.sh
	cp controller/Controller.exe.so Controller.exe
	cp controller/Controller.exe Controller.sh
	cp client/NPClient.dll.so NPClient.dll

bridge64_files :
	$(MAKE) -C tester Tester64.exe Tester.exe CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	$(MAKE) -C controller CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	$(MAKE) -C client NPClient64.dll.so NPClient.dll.so CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	cp tester/Tester.exe.so Tester.exe
	cp tester/Tester.exe Tester.sh
	cp tester/Tester64.exe.so Tester64.exe
	cp tester/Tester64.exe Tester64.sh
	cp client/NPClient.dll.so NPClient.dll
	cp client/NPClient64.dll.so NPClient64.dll
	cp controller/Controller.exe.so Controller.exe
	cp controller/Controller.exe Controller.sh

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll* *.sh

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	$(MAKE) -C client clean
	$(MAKE) -C controller clean
	$(MAKE) -C tester clean
	rm -f *.exe* *.dll* *.sh

endif

EXTRA_DIST = dumper/data.txt dumper/Makefile dumper/main.c client/NPClient_main.c client/Makefile \
client/NPClient.spec client/NPClient_dll.h client/rest.h client/rest.c \
client/NPClient.h ltr_wine.nsi README tester/npifc.c tester/rest.c tester/rest.h \
tester/Makefile tester/npview.rc tester/resource.h tester/main.cpp \
tester/npifc.h controller/resources.rc controller/Makefile controller/dinput.cpp \
controller/kbi_interface.h controller/linuxtrack1.ico controller/linuxtrack.ico controller/resource.h \
controller/main.cpp linuxtrack1.ico linuxtrack.ico


