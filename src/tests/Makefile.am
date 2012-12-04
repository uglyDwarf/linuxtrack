if DARWIN
  AM_CFLAGS = -arch i386 -arch ppc
  DARWINFLAGS = -undefined dynamic_lookup
else
  LINUXFLAGS = -fprofile-arcs -ftest-coverage 
endif

noinst_PROGRAMS = ltlib_test #tests

#if V4L2
#if LIBV4L2
#  noinst_PROGRAMS += webcam_driver_test 
#endif
#endif

#pose_test_SOURCES = pose_test.c
ltlib_test_SOURCES = ltlib_test.c ../utils.c ../utils.h
#webcam_driver_test_SOURCES = webcam_driver_test.c ../webcam_driver.c \
#                ../utils.h ../utils.c ../list.c ../list.h ../pref.c ../pref.h \
#                ../pref_bison.c ../pref_bison.h ../pref_flex.c ../pref_int.h \
#                ../pref_global.c ../pref_global.h ../image_process.c \
#                ../image_process.h ../axis.c ../axis.h ../spline.c ../spline.h 
#pref_test_SOURCES = pref_test.c ../utils.h ../utils.c ../list.c ../list.h ../pref.c ../pref.h \
#                ../pref_bison.c ../pref_bison.h ../pref_flex.c ../pref_int.h \
#                ../pref_global.c ../pref_global.h ../axis.c ../axis.h \
#                ../spline.c ../spline.h 
#tests_SOURCES = test.c ../utils.h ../utils.c ../list.c ../list.h ../pref.c ../pref.h \
#                ../pref_bison.c ../pref_bison.h ../pref_flex.c ../pref_int.h 
                

#pose_test_LDADD = -lm -lpthread -ldl -llinuxtrack
ltlib_test_LDADD = -lm -lpthread -ldl -llinuxtrack
#webcam_driver_test_LDADD = -lm -lpthread -ldl -lltr -lv4l2
#pref_test_LDADD = -lm -lpthread -ldl -lltr
#test_LDALL = -lm

#pose_test_CFLAGS = -I.. '-DLIB_PATH="@libdir@/"'
ltlib_test_CFLAGS = -I${srcdir} -I${srcdir}/.. -I.. '-DLIB_PATH="$(pkglibdir)/"'
#webcam_driver_test_CFLAGS = -I${srcdir} -I${srcdir}/.. -I.. '-DLIB_PATH="$(pkglibdir)/"'
#pref_test_CFLAGS = -I.. '-DLIB_PATH="@libdir@/"'
#tests_CFLAGS = -Wextra $(LINUXFLAGS) -I${srcdir} -I${srcdir}/.. -I.. '-DLIB_PATH="$(pkglibdir)/"'

#pose_test_LDFLAGS = -L..
ltlib_test_LDFLAGS = -L..
#webcam_driver_test_LDFLAGS = -L..
#pref_test_LDFLAGS = -L..
#tests_LDFLAGS = $(LINUXFLAGS)

MOSTLYCLEANFILES = *.gcno

