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
  AM_CFLAGS = 
  LINUXFLAGS = '-Wl,-rpath,$(pkglibdir)' '-DLIB_PATH="$(pkglibdir)/"'
  SYSFLAGS = -DIBM=0 -DAPL=0 -DLIN=1
  QMAKE_FLAGS = -spec linux-g++
  
endif

lib_LTLIBRARIES = liblinuxtrack.la
pkglib_LTLIBRARIES = libltr.la
bin_PROGRAMS = ltr_server

ltr_server_SOURCES = ltr_server.c ipc_utils.c ipc_utils.h utils.c utils.h
ltr_server_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS} $(LINUXFLAGS)
ltr_server_LDADD = -lpthread libltr.la
ltr_server_LDFLAGS = $(LINUXFLAGS) ${TIGER_LDFLAG}

if CWIID
  pkglib_LTLIBRARIES += libwii.la 
endif

if V4L2
  pkglib_LTLIBRARIES += libwc.la 
endif

#if LIBUSB
#  pkglib_LTLIBRARIES += libtir4.la 
#endif

if LIBZ
pkglib_LTLIBRARIES += libtir.la
if LIBUSB10
  pkglib_LTLIBRARIES += libltusb1.la 
endif

if OPENUSB
  pkglib_LTLIBRARIES += libltopenusb.la 
endif
endif

#bin_PROGRAMS = lt_server lt_client
#lt_server_SOURCES = lt_server.c netcomm.c netcomm.h
#lt_server_LDADD = -lm -lpthread -ldl libltr.la 
#lt_client_SOURCES = lt_client.c netcomm.c netcomm.h
#lt_client_LDADD = -lm -lpthread -ldl libltr.la 

include_HEADERS = linuxtrack.h

#liblinuxtrack_la_SOURCES = ltlib.c linuxtrack.h utils.c utils.h list.c list_h dyn_load.c dyn_load.h
#liblinuxtrack_la_SOURCES = ltlib.c linuxtrack.h utils.c utils.h dyn_load.c dyn_load.h
liblinuxtrack_la_SOURCES = ltlib.c linuxtrack.h utils.c utils.h ipc_utils.c ipc_utils.h
liblinuxtrack_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS} $(LINUXFLAGS)
liblinuxtrack_la_LDFLAGS = -export-symbols "${srcdir}/liblt.sym" $(LINUXFLAGS) ${TIGER_LDFLAG}

libltr_la_SOURCES = cal.c cal.h list.c list.h dyn_load.c dyn_load.h\
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h \
  utils.c utils.h image_process.c image_process.h tracking.c tracking.h \
  ltlib_int.c ltlib_int.h spline.c spline.h axis.c axis.h wii_driver_prefs.c \
  wii_driver_prefs.h tir_driver_prefs.c tir_driver_prefs.h wc_driver_prefs.c \
  wc_driver_prefs.h
libltr_la_LIBADD = -lm -lpthread -ldl
libltr_la_LDFLAGS = -export-symbols-regex '^ltr_int' $(LINUXFLAGS) ${TIGER_LDFLAG}
libltr_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS} $(LINUXFLAGS)
#libtir4_la_SOURCES = tir4_driver.c tir4_driver.h new_cal.c new_cal.h
#libtir4_la_LIBADD = -lusb  libltr.la

libtir_la_SOURCES = tir.h tir_hw.c tir_hw.h \
    tir_img.c tir_img.h usb_ifc.h tir_driver.h tir_driver.c runloop.c runloop.h
libtir_la_LIBADD = -lz libltr.la
libtir_la_LDFLAGS = -export-symbols "${srcdir}/tir.sym" $(LINUXFLAGS) ${TIGER_LDFLAG}

libltusb1_la_SOURCES = libusb_ifc.c usb_ifc.h
libltusb1_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS}
libltusb1_la_LIBADD = -lusb-1.0 libltr.la
libltusb1_la_LDFLAGS = ${TIGER_LDFLAG} $(LINUXFLAGS)
libltopenusb_la_SOURCES = openusb_ifc.c usb_ifc.h
libltopenusb_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS}
libltopenusb_la_LIBADD = -lopenusb libltr.la
libltopenusb_la_LDFLAGS = ${TIGER_LDFLAG} $(LINUXFLAGS)

libwii_la_SOURCES = wiimote_driver.c wiimote_driver.h runloop.c runloop.h
libwii_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS}
libwii_la_LIBADD = -lcwiid libltr.la
libwii_la_LDFLAGS = -export-symbols "${srcdir}/drivers.sym" ${TIGER_LDFLAG} $(LINUXFLAGS)

libwc_la_SOURCES = webcam_driver.c webcam_driver.h runloop.c runloop.h
libwc_la_CFLAGS = ${TIGER_CFLAG} ${AM_CFLAGS}
libwc_la_LIBADD = libltr.la -lv4l2
libwc_la_LDFLAGS = -export-symbols "${srcdir}/webcam_driver.sym" ${TIGER_LDFLAG} $(LINUXFLAGS)

dist_pkgdata_DATA = .linuxtrack 51-TIR.rules

if XPLANE_PLUGIN
  pkglib_LTLIBRARIES += xlinuxtrack.la xlinuxtrack9.la
  xlinuxtrack_la_SOURCES = xlinuxtrack.c xlinuxtrack_pref.c xlinuxtrack_pref.h \
			    utils.c utils.h ltlib.c linuxtrack.h ipc_utils.c ipc_utils.h
  xlinuxtrack_la_CFLAGS = -I${XPLANE_SDK}/XPLM -I${XPLANE_SDK}/Widgets $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS) ${TIGER_CFLAG} ${AM_CFLAGS} $(LINUXFLAGS)
  xlinuxtrack_la_LDFLAGS = -module -shared -fPIC -m32 -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib -export-symbols "${srcdir}/xlinuxtrack.sym" $(LINUXFLAGS) ${TIGER_LDFLAG}
  xlinuxtrack_la_LIBADD = -lm -ldl
  xlinuxtrack9_la_SOURCES = xlinuxtrack9.c \
			    utils.c utils.h ltlib.c linuxtrack.h ipc_utils.c ipc_utils.h
  xlinuxtrack9_la_CFLAGS = -I${XPLANE_SDK}/XPLM -I${XPLANE_SDK}/Widgets $(SYSFLAGS) -fPIC -m32 $(DARWINCFLAGS) ${TIGER_CFLAG} ${AM_CFLAGS} $(LINUXFLAGS)
  xlinuxtrack9_la_LDFLAGS = -module -shared -fPIC -m32 -L/lib32 -L/usr/lib32 $(DARWINFLAGS) -dynamiclib -export-symbols "${srcdir}/xlinuxtrack.sym" $(LINUXFLAGS) ${TIGER_LDFLAG}
  xlinuxtrack9_la_LIBADD = -lm -ldl
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


qt_gui/ltr_gui : ${pkglib_LTLIBRARIES}
	pushd qt_gui; qmake ${QMAKE_FLAGS} "LIBDIR=$(pkglibdir)"; make; popd

install-exec-hook:
	pushd qt_gui; make INSTALL_ROOT=${DESTDIR} install; popd

uninstall-hook:
	pushd qt_gui; make INSTALL_ROOT=${DESTDIR} uninstall; popd


EXTRA_DIST = qt_gui/ltr_axis.cpp qt_gui/glwidget.h qt_gui/model_wizzard.ui \
  qt_gui/objreader.cpp qt_gui/window.h qt_gui/webcam_prefs.cpp \
  qt_gui/xm8_detail.png qt_gui/ltr_profiles.cpp qt_gui/scp_form.h \
  qt_gui/prefs_link.h qt_gui/webcam_info.cpp qt_gui/scp_form.cpp \
  qt_gui/ltr_axis.h qt_gui/main.cpp qt_gui/wiimote_prefs.h qt_gui/ltr_tracking.cpp \
  qt_gui/scurve.cpp qt_gui/ltr_state.cpp qt_gui/scurve.h qt_gui/scurve.ui \
  qt_gui/ltr_show.h qt_gui/ltr_gui.cpp qt_gui/ltr_profiles.h qt_gui/scp_form.ui \
  qt_gui/XM8_cockpit.obj qt_gui/macwebcam_prefs.cpp qt_gui/tir_prefs.cpp \
  qt_gui/macwebcam_info.h qt_gui/objreader.h qt_gui/log_view.h qt_gui/scview.cpp \
  qt_gui/ltr_dev_help.cpp qt_gui/xm8d_body.obj qt_gui/ltr_gui.h qt_gui/ltr_gui_prefs.h \
  qt_gui/wiimote_prefs.cpp qt_gui/cap.png qt_gui/ltr_model.h qt_gui/ltr_gui.ui \
  qt_gui/ltr_tracking.h qt_gui/clip.png qt_gui/log_view.cpp qt_gui/ltr_rc.qrc \
  qt_gui/ltr.ui qt_gui/dev_help.ui qt_gui/tir_prefs.h qt_gui/window.cpp \
  qt_gui/webcam_prefs.h qt_gui/macwebcam_info.cpp qt_gui/webcam_info.h \
  qt_gui/ltr_model.cpp qt_gui/ltr_gui_prefs.cpp qt_gui/ltr_show.cpp \
  qt_gui/logview.ui qt_gui/ltr_state.h qt_gui/ltr_dev_help.h qt_gui/scview.h \
  qt_gui/macwebcam_prefs.h qt_gui/glwidget.cpp qt_gui/model_creation.ui \
  tir4_driver.c tir4_driver.h 51-TIR.rules drivers.sym liblt.sym tir.sym webcam_driver.sym \
  xlinuxtrack.sym

man_MANS = ltr_gui.1 ltr_server.1
