if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
  SYSFLAGS = -DIBM=0 -DAPL=1 -DLIN=0
else
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
endif

bin_PROGRAMS = pose_test ltlib_test

liblinuxtrack_a_SOURCES = cal.c cal.h list.c list.h ltlib.c ltlib.h \
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h

noinst_LIBRARIES = liblinuxtrack.a
dist_data_DATA = .linuxtrack

if LIBUSB
  bin_PROGRAMS += tir4_driver_test
  liblinuxtrack_a_SOURCES += tir4_driver.c tir4_driver.h
  tir4_driver_test_SOURCES =  tir4_driver_test.c
  tir4_driver_test_LDADD =  liblinuxtrack.a
  tir4_driver_test_CFLAGS = -m32 -fPIC
  tir4_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32
endif

if LIBUSB10
  liblinuxtrack_a_SOURCES += libusb_ifc.c tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c
else
  if OPENUSB
    liblinuxtrack_a_SOURCES += openusb_ifc.c tir.h tir_hw.c tir_hw.h \
      tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c
  endif
endif

if CWIID
  bin_PROGRAMS += wiimote_driver_test
  liblinuxtrack_a_SOURCES += wiimote_driver.c wiimote_driver.h
  wiimote_driver_test_SOURCES = wiimote_driver_test.c 
  wiimote_driver_test_LDADD = liblinuxtrack.a 
  wiimote_driver_test_CFLAGS = -m32 -fPIC
  wiimote_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
  CWIIDFLAGS = -lcwiid
endif

if V4L2
  bin_PROGRAMS += webcam_driver_test
  liblinuxtrack_a_SOURCES += webcam_driver.c webcam_driver.h
  webcam_driver_test_SOURCES =  webcam_driver_test.c
  webcam_driver_test_LDADD =  liblinuxtrack.a
  webcam_driver_test_CFLAGS = -m32 -fPIC
  webcam_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 $(CWIIDFLAGS)
endif

if XPLANE_PLUGIN
  bin_PROGRAMS += xlinuxtrack.xpl
  xlinuxtrack_xpl_SOURCES = xlinuxtrack.c
  xlinuxtrack_xpl_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS)
  xlinuxtrack_xpl_LDFLAGS = -shared -fPIC -L/lib32 -L/usr/lib32 $(CWIIDFLAGS) $(DARWINFLAGS) -dynamiclib
  xlinuxtrack_xpl_LDADD = liblinuxtrack.a
endif

pose_test_SOURCES = pose_test.c
ltlib_test_SOURCES = ltlib_test.c 

pose_test_LDADD = liblinuxtrack.a
ltlib_test_LDADD = liblinuxtrack.a 

pose_test_CFLAGS = -m32 -fPIC
ltlib_test_CFLAGS = -fPIC -m32
pose_test_LDFLAGS = -L/lib32 -L/usr/lib32 $(CWIIDFLAGS)
ltlib_test_LDFLAGS = -fPIC -L/lib32 -L/usr/lib32 $(CWIIDFLAGS)

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
