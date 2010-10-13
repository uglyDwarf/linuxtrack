if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  TIGER_CFLAG = -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
  TIGER_LDFLAG = -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -headerpad_max_install_names
  LEOPARD_CFLAG = -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
  LEOPARD_LDFLAG = -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -headerpad_max_install_names
  DARWINFLAGS = -undefined dynamic_lookup
  SYSFLAGS = -DIBM=0 -DAPL=1 -DLIN=0
  QMAKE_FLAGS = -spec macx-g++
else
  AM_CFLAGS = -m32
  LINUXFLAGS = '-Wl,-rpath,@libdir@'
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
  QMAKE_FLAGS = -spec linux-g++-32
endif


lib_LTLIBRARIES = liblinuxtrack.la liblt.la

if CWIID
  lib_LTLIBRARIES += libwii.la 
endif

if V4L2
  lib_LTLIBRARIES += libwc.la 
endif

#if LIBUSB
#  lib_LTLIBRARIES += libtir4.la 
#endif

if LIBZ
lib_LTLIBRARIES += libtir.la
if LIBUSB10
  lib_LTLIBRARIES += libltusb1.la 
endif

if OPENUSB
  lib_LTLIBRARIES += libltopenusb.la 
endif
endif

#bin_PROGRAMS = lt_server lt_client
#lt_server_SOURCES = lt_server.c netcomm.c netcomm.h
#lt_server_LDADD = -lm -lpthread -ldl -llinuxtrack 
#lt_client_SOURCES = lt_client.c netcomm.c netcomm.h
#lt_client_LDADD = -lm -lpthread -ldl -llinuxtrack 

include_HEADERS = ltlib.h

#liblt_la_SOURCES = ltlib.c ltlib.h utils.c utils.h list.c list_h dyn_load.c dyn_load.h
liblt_la_SOURCES = ltlib.c ltlib.h utils.c utils.h dyn_load.c dyn_load.h
liblt_la_CFLAGS = ${TIGER_CFLAG}
liblt_la_LIBADD = -ldl
liblt_la_LDFLAGS = -export-symbols liblt.sym $(LINUXFLAGS) ${TIGER_LDFLAG}

liblinuxtrack_la_SOURCES = cal.c cal.h list.c list.h dyn_load.c dyn_load.h\
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h tracking.c tracking.h \
  ltlib_int.c ltlib_int.h spline.c spline.h axis.c axis.h wii_driver_prefs.c \
  wii_driver_prefs.h tir_driver_prefs.c tir_driver_prefs.h wc_driver_prefs.c \
  wc_driver_prefs.h
liblinuxtrack_la_LIBADD = -lm -lpthread -ldl
liblinuxtrack_la_LDFLAGS = -export-symbols-regex '^ltr_int' $(LINUXFLAGS) ${TIGER_LDFLAG}
#libtir4_la_SOURCES = tir4_driver.c tir4_driver.h new_cal.c new_cal.h
#libtir4_la_LIBADD = -lusb  -llinuxtrack

libtir_la_SOURCES = tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
libtir_la_LIBADD = -lz -llinuxtrack
libtir_la_LDFLAGS = -export-symbols tir.sym $(LINUXFLAGS) ${TIGER_LDFLAG}

libltusb1_la_SOURCES = libusb_ifc.c usb_ifc.h
libltusb1_la_CFLAGS = ${TIGER_CFLAG}
libltusb1_la_LIBADD = -lusb-1.0 -llinuxtrack
libltusb1_la_LDFLAGS = ${TIGER_LDFLAG} $(LINUXFLAGS) 
libltopenusb_la_SOURCES = openusb_ifc.c usb_ifc.h
libltopenusb_la_CFLAGS = ${TIGER_CFLAG}
libltopenusb_la_LIBADD = -lopenusb -llinuxtrack
libltopenusb_la_LDFLAGS = ${TIGER_LDFLAG} $(LINUXFLAGS) 

libwii_la_SOURCES = wiimote_driver.c wiimote_driver.h runloop.c runloop.h
libwii_la_CFLAGS = ${TIGER_CFLAG}
libwii_la_LIBADD = -lcwiid -llinuxtrack
libwii_la_LDFLAGS = -export-symbols drivers.sym ${TIGER_LDFLAG} $(LINUXFLAGS) 

libwc_la_SOURCES = webcam_driver.c webcam_driver.h runloop.c runloop.h
libwc_la_CFLAGS = ${TIGER_CFLAG}
libwc_la_LIBADD = -llinuxtrack -lv4l2
libwc_la_LDFLAGS = -export-symbols webcam_driver.sym ${TIGER_LDFLAG} $(LINUXFLAGS) 

dist_pkgdata_DATA = .linuxtrack

if XPLANE_PLUGIN
  lib_LTLIBRARIES += xlinuxtrack.la
  xlinuxtrack_la_SOURCES = xlinuxtrack.c xlinuxtrack_pref.c xlinuxtrack_pref.h \
			    utils.c utils.h dyn_load.c dyn_load.h ltlib.c ltlib.h
  xlinuxtrack_la_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers $(SYSFLAGS) -fPIC $(DARWINCFLAGS) ${TIGER_CFLAG}
  xlinuxtrack_la_LDFLAGS = -module -shared -fPIC -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib -export-symbols xlinuxtrack.sym $(LINUXFLAGS) ${TIGER_LDFLAG}
  xlinuxtrack_la_LIBADD = -lm -ldl
endif

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h

#dirty hack to integrate qt_gui

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	-cd qt_gui;make clean

distclean-local: distclean-local-check
.PHONY: distclean-local-check
distclean-local-check:
	-cd qt_gui;make distclean

noinst_SCRIPTS = qt_gui/ltr_gui

qt_gui/ltr_gui : ${lib_LTLIBRARIES}
	pushd qt_gui; qmake ${QMAKE_FLAGS}; make; popd
	echo "QT_GUI COMPILED SUCCESSFULLY!"

install-exec-hook:
	pushd qt_gui; make install; popd

