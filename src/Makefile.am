bin_PROGRAMS = tir4_driver_test pose_test wiimote_driver_test xlinuxtrack.xpl ltlib_test

noinst_LIBRARIES = liblinuxtrack.a

liblinuxtrack_a_SOURCES = cal.c cal.h list.c list.h ltlib.c ltlib.h \
  math_utils.c math_utils.h pose.c pose.h pref.c pref.h pref_bison.y \
  pref_flex.l pref_global.c pref_global.h pref_int.h tir4_driver.c \
  tir4_driver.h utils.c utils.h wiimote_driver.c wiimote_driver.h




tir4_driver_test_SOURCES =  tir4_driver_test.c
wiimote_driver_test_SOURCES = wiimote_driver_test.c 
pose_test_SOURCES = pose_test.c
xlinuxtrack_xpl_SOURCES = xlinuxtrack.c
ltlib_test_SOURCES = ltlib_test.c 

tir4_driver_test_LDADD =  liblinuxtrack.a
wiimote_driver_test_LDADD = liblinuxtrack.a 
pose_test_LDADD = liblinuxtrack.a
xlinuxtrack_xpl_LDADD = liblinuxtrack.a
ltlib_test_LDADD = liblinuxtrack.a 


dist_data_DATA = bulk_config_data.bin .linuxtrack

wiimote_driver_test_CFLAGS = -m32 -fPIC
tir4_driver_test_CFLAGS = -m32 -fPIC
pose_test_CFLAGS = -m32 -fPIC
xlinuxtrack_xpl_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers -DIBM=0 -DAPL=0 -DLIN=1 -fPIC -m32
ltlib_test_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers -DIBM=0 -DAPL=0 -DLIN=1 -fPIC -m32
wiimote_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
tir4_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
pose_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
ltlib_test_LDFLAGS = -fPIC -L/lib32 -L/usr/lib32 -lcwiid
xlinuxtrack_xpl_LDFLAGS = -shared -fPIC -L/lib32 -L/usr/lib32 -lcwiid

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
