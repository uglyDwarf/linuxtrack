if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
  SYSFLAGS = -DIBM=0 -DAPL=1 -DLIN=0
else
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
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
liblt_la_LIBADD = -ldl
liblt_la_LDFLAGS = -export-symbols liblt.sym '-Wl,-rpath,@prefix@/lib'

liblinuxtrack_la_SOURCES = cal.c cal.h list.c list.h dyn_load.c dyn_load.h\
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h tracking.c tracking.h \
  ltlib_int.c ltlib_int.h spline.c spline.h axis.c axis.h 
liblinuxtrack_la_LIBADD = -lm -lpthread -ldl
liblinuxtrack_la_LDFLAGS = -export-symbols-regex '^ltr_int' '-Wl,-rpath,@prefix@/lib'
#libtir4_la_SOURCES = tir4_driver.c tir4_driver.h new_cal.c new_cal.h
#libtir4_la_LIBADD = -lusb  -llinuxtrack

libtir_la_SOURCES = tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
libtir_la_LIBADD = -lz -llinuxtrack
libtir_la_LDFLAGS = -export-symbols drivers.sym '-Wl,-rpath,@prefix@/lib'

libltusb1_la_SOURCES = libusb_ifc.c usb_ifc.h
libltusb1_la_LIBADD = -lusb-1.0 -llinuxtrack
libltopenusb_la_SOURCES = openusb_ifc.c usb_ifc.h
libltopenusb_la_LIBADD = -lopenusb -llinuxtrack

libwii_la_SOURCES = wiimote_driver.c wiimote_driver.h runloop.c runloop.h
libwii_la_LIBADD = -lcwiid -llinuxtrack
libwii_la_LDFLAGS = -export-symbols drivers.sym

libwc_la_SOURCES = webcam_driver.c webcam_driver.h runloop.c runloop.h
libwc_la_LIBADD = -llinuxtrack -lv4l2
libwc_la_LDFLAGS = -export-symbols webcam_driver.sym

dist_pkgdata_DATA = .linuxtrack

if XPLANE_PLUGIN
  lib_LTLIBRARIES += xlinuxtrack.la
  xlinuxtrack_la_SOURCES = xlinuxtrack.c xlinuxtrack_pref.c xlinuxtrack_pref.h \
			    utils.c utils.h dyn_load.c dyn_load.h ltlib.c ltlib.h
  xlinuxtrack_la_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS)
  xlinuxtrack_la_LDFLAGS = -module -shared -fPIC -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib -export-symbols xlinuxtrack.sym  '-Wl,-rpath,@prefix@/lib'
  xlinuxtrack_la_LIBADD = -lm -ldl
endif

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
