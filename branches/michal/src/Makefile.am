if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
  SYSFLAGS = -DIBM=0 -DAPL=1 -DLIN=0
else
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
endif

bin_PROGRAMS = ltlib_test lt_server lt_client

lib_LTLIBRARIES = liblinuxtrack.la liblt.la

if CWIID
  lib_LTLIBRARIES += libwii.la 
endif

if V4L2
  lib_LTLIBRARIES += libwc.la 
endif

if LIBUSB
  lib_LTLIBRARIES += libtir4.la 
endif

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

liblt_la_SOURCES = ltlib.c ltlib.h
liblt_la_LIBADD = -ldl

liblinuxtrack_la_SOURCES = cal.c cal.h list.c list.h ltlib_int.c ltlib_int.h \
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h tracking.c tracking.h \
  netcomm.c netcomm.h

libtir4_la_SOURCES = tir4_driver.c tir4_driver.h
libtir4_la_LIBADD = -lusb  -llinuxtrack
libtir1_la_SOURCES = libusb_ifc.c tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c
libtir1_la_LIBADD = -lz -lusb-1.0 -llinuxtrack
libtir2_la_SOURCES = openusb_ifc.c tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c
libtir2_la_LIBADD = -lz -lopenusb -llinuxtrack
libwii_la_SOURCES = wiimote_driver.c wiimote_driver.h
libwii_la_LIBADD = -lcwiid -llinuxtrack
libwc_la_SOURCES = webcam_driver.c webcam_driver.h
libwc_la_LIBADD = -llinuxtrack

dist_data_DATA = .linuxtrack

if XPLANE_PLUGIN
  lib_LTLIBRARIES += xlinuxtrack.la
  xlinuxtrack_la_SOURCES = xlinuxtrack.c
  xlinuxtrack_la_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS)
  xlinuxtrack_la_LDFLAGS = -module -shared -fPIC -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib
  xlinuxtrack_la_LIBADD = -llt
endif

#pose_test_SOURCES = pose_test.c
ltlib_test_SOURCES = ltlib_test.c 

#pose_test_LDADD = -lm -lpthread -ldl -llt
ltlib_test_LDADD = -lm -lpthread -ldl -llt

#pose_test_CFLAGS = -m32 -fPIC
ltlib_test_CFLAGS = -fPIC -m32
#pose_test_LDFLAGS = -L/lib32 -L/usr/lib32
ltlib_test_LDFLAGS = -fPIC -L/lib32 -L/usr/lib32

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
