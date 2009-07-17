bin_PROGRAMS = tir4_driver_test pose_test wiimote_driver_test
tir4_driver_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  tir4_driver_test.c \
  math_utils.c math_utils.h pose.c pose.h wiimote_driver.c  wiimote_driver.h
wiimote_driver_test_SOURCES =  cal.c cal.h wiimote_driver.c  wiimote_driver.h  wiimote_driver_test.c \
  math_utils.c math_utils.h pose.c pose.h tir4_driver.c  tir4_driver.h

pose_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  pose_test.c \
  math_utils.c math_utils.h pose.c pose.h wiimote_driver.c  wiimote_driver.h
dist_data_DATA = bulk_config_data.bin

wiimote_driver_test_LDFLAGS = -lcwiid
tir4_driver_test_LDFLAGS = -lcwiid
pose_test_LDFLAGS = -lcwiid

if XPLANE_PLUGIN
bin_PROGRAMS += xlinuxtrack
xlinuxtrack_SOURCES = xlinuxtrack.c cal.c cal.h tir4_driver.c  tir4_driver.h \
  math_utils.c math_utils.h pose.c pose.h
xlinuxtrack_LDFLAGS = -shared -fPIC
xlinuxtrack_CPPFLAGS = -I ../$(XPLANE_SDK)/CHeaders/XPLM -DIBM=0 -DAPL=0 -DLIN=1 
endif
