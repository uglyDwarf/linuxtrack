bin_PROGRAMS = tir4_driver_test pose_test wiimote_driver_test xlinuxtrack_xpl 

tir4_driver_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  tir4_driver_test.c \
  math_utils.c math_utils.h pose.c pose.h wiimote_driver.c  wiimote_driver.h
wiimote_driver_test_SOURCES =  cal.c cal.h wiimote_driver.c  wiimote_driver.h  wiimote_driver_test.c \
  math_utils.c math_utils.h pose.c pose.h tir4_driver.c  tir4_driver.h

pose_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  pose_test.c \
  math_utils.c math_utils.h pose.c pose.h wiimote_driver.c  wiimote_driver.h \
  pref.c pref.h pref_global.c pref_global.h utils.c utils.h list.c list.h \
  pref_bison.y pref_flex.l
xlinuxtrack_xpl_SOURCES = cal.c cal.h tir4_driver.c  tir4_driver.h  pose_test.c math_utils.c math_utils.h pose.c pose.h wiimote_driver.c  wiimote_driver.h xlinuxtrack.c ltlib.c ltlib.h
dist_data_DATA = bulk_config_data.bin prefs.txt

wiimote_driver_test_CFLAGS = -m32 -fPIC
tir4_driver_test_CFLAGS = -m32 -fPIC
pose_test_CFLAGS = -m32 -fPIC
xlinuxtrack_xpl_CFLAGS = -I../${XPLANE_SDK}/CHeaders/XPLM -I../${XPLANE_SDK}/CHeaders/Widgets -I../${XPLANE_SDK}/CHeaders/Wrappers -DIBM=0 -DAPL=0 -DLIN=1 -fPIC -m32
wiimote_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
tir4_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
pose_test_LDFLAGS = -L/lib32 -L/usr/lib32 -lcwiid
xlinuxtrack_xpl_LDFLAGS = -shared -fPIC -L/lib32 -L/usr/lib32 -lcwiid

AM_YFLAGS = -d
AM_LFLAGS = -i
BUILT_SOURCES = pref_flex.c pref_bison.c pref_bison.h
