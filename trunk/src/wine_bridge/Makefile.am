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

.PHONY : bridge bridge64 bridge_mac

bridge_mac : ltr_wine.nsi tester/Tester.exe.so controller/Controller.exe.so client/NPClient.dll.so
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib tester/Tester.exe.so
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib controller/Controller.exe.so 
	install_name_tool -change $(WINE_PREFIX)/lib/libwine.1.dylib libwine.1.dylib client/NPClient.dll.so
	makensis $<

bridge : ltr_wine.nsi tester/Tester.exe.so controller/Controller.exe.so client/NPClient.dll.so
	makensis $<

bridge64 : ltr_wine64.nsi tester/Tester.exe.so tester/Tester64.exe.so \
               controller/Controller.exe.so client/NPClient.dll.so client/NPClient64.dll.so
	makensis $<

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -f *.exe

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	rm -f *.exe

endif

dist_pkgdata_DATA = README.wine


