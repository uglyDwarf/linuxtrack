EXTRA_DIST = Info.plist PkgInfo English.lproj/InfoPlist.strings \
English.lproj/MainMenu.nib/keyedobjects.nib English.lproj/MainMenu.nib/designable.nib

AM_CFLAGS = -arch i386 -arch ppc
TIGER_CFLAG = -isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
TIGER_LDFLAG = -Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4
LEOPARD_CFLAG = -isysroot /Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
LEOPARD_LDFLAG = -Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5
DARWINFLAGS = -undefined dynamic_lookup 

if DARWIN
  lib_LTLIBRARIES = libmacwc.la libmacwii.la
  bin_PROGRAMS = qt_cam sg_cam WiiServer
endif

qt_cam_SOURCES = qt_camera.m args.c args.h buffer.c buffer.h mac_camera.h com_proc.c \
  com_proc.h ipc_utils.c ipc_utils.h processing.c processing.h macwebcam.c
qt_cam_LDADD = -llinuxtrack -lobjc -lpthread
qt_cam_LDFLAGS = ${AM_CFLAGS} ${LEOPARD_LDFLAG} -framework Cocoa -framework QTKit -framework QuartzCore
qt_cam_CFLAGS = ${AM_CFLAGS} ${LEOPARD_CFLAG}
qt_cam_OBJCFLAGS = ${AM_CFLAGS} ${LEOPARD_CFLAG}

sg_cam_SOURCES = sg_camera.c args.c args.h buffer.c buffer.h mac_camera.h com_proc.c \
  com_proc.h ipc_utils.c ipc_utils.h processing.c processing.h macwebcam.c
sg_cam_LDADD = -llinuxtrack -lobjc -lpthread
sg_cam_LDFLAGS = ${AM_CFLAGS} ${TIGER_LDFLAG} -framework QuickTime -framework CoreServices
sg_cam_CFLAGS = ${AM_CFLAGS} ${TIGER_CFLAG}
sg_cam_OBJCFLAGS = ${AM_CFLAGS} ${TIGER_CFLAG}

libmacwc_la_SOURCES = macwebcam_driver.c com_proc.c com_proc.h ../runloop.c ../runloop.h \
  ipc_utils.c ipc_utils.h
libmacwc_la_CFLAGS = ${TIGER_CFLAG}
libmacwc_la_LIBADD = -llinuxtrack
libmacwc_la_LDFLAGS = -export-symbols ../drivers.sym ${TIGER_LDFLAG}

libmacwii_la_SOURCES = macwii_driver.c wii_com.c wii_com.h com_proc.c com_proc.h \
  ../runloop.c ../runloop.h ../wii_driver_prefs.c ../wii_driver_prefs.h
libmacwii_la_CFLAGS = ${TIGER_CFLAG}
libmacwii_la_LIBADD = -llinuxtrack
libmacwii_la_LDFLAGS = -export-symbols ../drivers.sym ${TIGER_LDFLAG}

WiiServer_SOURCES = com_proc.c com_proc.h wii_com.c wii_com.h wiimote.m wiimote.h \
  wiimote_server.m wiimote_server.h wiisrvr_main.m
WiiServer_LDADD = -llinuxtrack -lobjc -lpthread
WiiServer_LDFLAGS = ${AM_CFLAGS} ${TIGER_LDFLAG} -framework Cocoa -framework CoreData \
  -framework AppKit -framework Foundation -framework IOBluetooth
WiiServer_CFLAGS = ${AM_CFLAGS} ${TIGER_CFLAG}
WiiServer_OBJCFLAGS = ${AM_CFLAGS} ${TIGER_CFLAG}

