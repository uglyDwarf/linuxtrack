bin_PROGRAMS = tir4_driver_test pose_test
tir4_driver_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  tir4_driver_test.c \
  math_utils.c math_utils.h pose.c pose.h
pose_test_SOURCES =  cal.c cal.h tir4_driver.c  tir4_driver.h  pose_test.c \
  math_utils.c math_utils.h pose.c pose.h
dist_data_DATA = bulk_config_data.bin

if XPLANE_PLUGIN
bin_PROGRAMS += xlinuxtrack
xlinuxtrack_SOURCES = xlinuxtrack.c cal.c cal.h tir4_driver.c  tir4_driver.h \
  math_utils.c math_utils.h pose.c pose.h
xlinuxtrack_LDFLAGS = -shared -fPIC
xlinuxtrack_CPPFLAGS = -I ../$(XPLANE_SDK)/CHeaders/XPLM -DIBM=0 -DAPL=0 -DLIN=1 
endif
