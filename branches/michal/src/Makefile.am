if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
  SYSFLAGS = -DIBM=0 -DAPL=1 -DLIN=0
else
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
endif

bin_PROGRAMS = lt_server lt_client

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
if LIBUSB10
  lib_LTLIBRARIES += libtir1.la 
endif

if OPENUSB
  lib_LTLIBRARIES += libtir2.la 
endif
endif

lt_server_SOURCES = lt_server.c
lt_server_LDADD = -lm -lpthread -ldl -llinuxtrack 
lt_client = lt_client.c
lt_client_LDADD = -lm -lpthread -ldl -llinuxtrack 

include_HEADERS = ltlib.h

liblt_la_SOURCES = ltlib.c ltlib.h utils.c utils.h list.c list_h dyn_load.c dyn_load.h
liblt_la_LIBADD = -ldl

liblinuxtrack_la_SOURCES = cal.c cal.h list.c list.h dyn_load.c dyn_load.h\
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h tracking.c tracking.h \
  netcomm.c netcomm.h ltlib_client.c ltlib_client.h ltlib_int.c ltlib_int.h

#libtir4_la_SOURCES = tir4_driver.c tir4_driver.h new_cal.c new_cal.h
#libtir4_la_LIBADD = -lusb  -llinuxtrack
libtir1_la_SOURCES = libusb_ifc.c tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
libtir1_la_LIBADD = -lz -lusb-1.0 -llinuxtrack
libtir2_la_SOURCES = openusb_ifc.c tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
libtir2_la_LIBADD = -lz -lopenusb -llinuxtrack
libwii_la_SOURCES = wiimote_driver.c wiimote_driver.h runloop.c runloop.h
libwii_la_LIBADD = -lcwiid -llinuxtrack
libwc_la_SOURCES = webcam_driver.c webcam_driver.h runloop.c runloop.h
libwc_la_LIBADD = -llinuxtrack

dist_data_DATA = .linuxtrack

if XPLANE_PLUGIN
  lib_LTLIBRARIES += xlinuxtrack.la
  xlinuxtrack_la_SOURCES = xlinuxtrack.c
  xlinuxtrack_la_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS)
  xlinuxtrack_la_LDFLAGS = -module -shared -fPIC -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib
  xlinuxtrack_la_LIBADD = -llt
endif

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
