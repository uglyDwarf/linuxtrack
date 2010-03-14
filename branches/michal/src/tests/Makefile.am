if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
endif

noinst_PROGRAMS = ltlib_test webcam_driver_test

#pose_test_SOURCES = pose_test.c
ltlib_test_SOURCES = ltlib_test.c 
webcam_driver_test_SOURCES = webcam_driver_test.c ../webcam_driver.c

#pose_test_LDADD = -lm -lpthread -ldl -llt
ltlib_test_LDADD = -lm -lpthread -ldl -llt
webcam_driver_test_LDADD = -lm -lpthread -ldl -llinuxtrack

#pose_test_CFLAGS = -m32 -I..
ltlib_test_CFLAGS = -m32 -I..
webcam_driver_test_CFLAGS = -m32 -I..

#pose_test_LDFLAGS = -L/lib32 -L/usr/lib32 -L..
ltlib_test_LDFLAGS = -L/lib32 -L/usr/lib32 -L..
webcam_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32  -L..
