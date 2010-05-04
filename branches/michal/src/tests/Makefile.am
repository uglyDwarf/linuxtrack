if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
endif

noinst_PROGRAMS = ltlib_test webcam_driver_test pref_test tests

#pose_test_SOURCES = pose_test.c
ltlib_test_SOURCES = ltlib_test.c 
webcam_driver_test_SOURCES = webcam_driver_test.c ../webcam_driver.c
pref_test_SOURCES = pref_test.c 
tests_SOURCES = test.c ../utils.h ../utils.c ../list.c ../list.h ../pref.c ../pref.h \
                ../pref_bison.c ../pref_bison.h ../pref_flex.c ../pref_int.h

#pose_test_LDADD = -lm -lpthread -ldl -llt
ltlib_test_LDADD = -lm -lpthread -ldl -llt
webcam_driver_test_LDADD = -lm -lpthread -ldl -llinuxtrack
pref_test_LDADD = -lm -lpthread -ldl -llinuxtrack
test_LDALL = -lm

#pose_test_CFLAGS = -m32 -I..
ltlib_test_CFLAGS = -m32 -I..
webcam_driver_test_CFLAGS = -m32 -I..
pref_test_CFLAGS = -m32 -I..
tests_CFLAGS = -Wextra -fprofile-arcs -ftest-coverage -m32 -I..

#pose_test_LDFLAGS = -L/lib32 -L/usr/lib32 -L..
ltlib_test_LDFLAGS = -L/lib32 -L/usr/lib32 -L..
webcam_driver_test_LDFLAGS = -L/lib32 -L/usr/lib32  -L..
pref_test_LDFLAGS = -L/lib32 -L/usr/lib32  -L..
tests_LDFLAGS = -fprofile-arcs -ftest-coverage
