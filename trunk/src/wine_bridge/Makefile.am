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


bridge_files : tester/Tester.exe.so tester/Tester.exe \
               controller/Controller.exe.so controller/Controller.exe \
               client/NPClient.dll.so
#	$(MAKE) -C tester CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
#	$(MAKE) -C controller CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
#	$(MAKE) -C client CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	cp tester/Tester.exe.so Tester.exe
	cp controller/Controller.exe.so Controller.exe
	cp client/NPClient.dll.so NPClient.dll

bridge64_files : tester/Tester.exe.so tester/Tester.exe tester/Tester64.exe.so tester/Tester64.exe \
               controller/Controller.exe.so controller/Controller.exe \
               client/NPClient.dll.so client/NPClient64.dll.so
#	$(MAKE) -C tester Tester64.exe Tester.exe CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
#	$(MAKE) -C controller CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
#	$(MAKE) -C client NPClient64.dll.so NPClient.dll.so CFLAGS="${CFLAGS}" CXXFLAGS="${CXXFLAGS}" LDFLAGS="${MYLDFLAGS}"
	cp tester/Tester.exe.so Tester.exe
	cp tester/Tester64.exe.so Tester64.exe
	cp client/NPClient.dll.so NPClient.dll
	cp client/NPClient64.dll.so NPClient64.dll
	cp controller/Controller.exe.so Controller.exe

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe* *.dll*

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe* *.dll*

endif

dist_pkgdata_DATA = README.wine
EXTRA_DIST = ltr_wine.nsi ltr_wine64.nsi linuxtrack1.ico linuxtrack.ico


