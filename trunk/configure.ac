#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([LinuxTrack], [0.0], [http://code.google.com/p/linux-track/issues/list])
AM_INIT_AUTOMAKE([no-dependencies -Wall -Werror])
AC_CONFIG_SRCDIR([src/tir4_driver.h])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_PROG_OBJC
AS_IF([test "$debug" != "no"],
      [CFLAGS+=" -g -O0 -Wall -Wextra"],
      [CFLAGS+=" -O3 "])

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
LT_INIT
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt], [LIBM=true], [AC_MSG_FAILURE([I need libm!])])
AC_CHECK_LIB([z], [gzopen], [LIBZ=true])
AC_CHECK_LIB([pthread], [pthread_create], [LIBPTHREAD=true], [AC_MSG_FAILURE([I need libpthread!])])
AC_CHECK_LIB([usb], [usb_open], [LIBUSB=true])
#AC_CHECK_LIB([openusb], [openusb_init], [LIBOPENUSB=true])
AC_CHECK_LIB([usb-1.0], [libusb_init], [LIBUSB10=true])
AC_CHECK_LIB([cwiid], [cwiid_open], [LIBCWIID=true])
AC_CHECK_LIB([QxtGui], [_ZN17QxtGlobalShortcutD0Ev], [LIBQXT=true])


# Checks for header files.
AC_CHECK_HEADERS([stdint.h assert.h])
AC_CHECK_HEADERS([stdlib.h unistd.h netdb.h sys/socket.h fcntl.h sys/ioctl.h])

AC_PATH_PROGS(QMAKE_PATH, [qmake qmake-qt4])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_INT8_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_C_INLINE

AC_CANONICAL_HOST


AC_CHECK_TYPE([struct v4l2_frmsizeenum],                                                                                                  
  [
    AC_DEFINE([V4L2], [1],[Use videodev2.h (V4L2)])
    with_v4l2=yes
  ], [],                                                                             
  [[#include  <linux/videodev2.h>
  ]]                                                                                                           
)                                                                                                                                             

AC_CHECK_HEADER([libv4l2.h],                                                                                                  
  [
    AC_DEFINE([LIBV4L2], [1],[Use libv4l2.h (V4L2)])
    with_libv4l2=yes
  ], [], []
)                                                                                                                                             

XPLANE_SDK=
AC_ARG_WITH([xplane_sdk],
            [AS_HELP_STRING([--with-xplane-sdk],
              [specify path to XPlane SDK @<:@default=../SDK@:>@])], 
            [XPLANE_SDK=${with_xplane_sdk}],
            [XPLANE_SDK="/usr/include/xplane_sdk"]
)

AC_ARG_ENABLE([xplane_plugin],
            [AS_HELP_STRING([--enable-xplane-plugin],
              [build XPlane plugin (requires XPlane SDK > 2.0.1)@<:@default=yes@:>@])], 
            [],
            [
                AC_CHECK_FILE([${XPLANE_SDK}/XPLM/XPLMScenery.h],
                    [
                      AC_SUBST([XPLANE_SDK])
                      with_xplane_plugin=yes
                    ],
                    [
                      with_xplane_plugin=no
                    ]
                )
            ]
)

AC_ARG_ENABLE([ltr_pipe],
		[AS_HELP_STRING([--enable-ltr-pipe],
				[build ltr_pipe data sender @<:@default=yes@:>@])],
		[with_ltr_pipe=$enableval],
		[with_ltr_pipe=yes]
)

AC_ARG_ENABLE([snow_leopard],
            [AS_HELP_STRING([--enable-snow-leopard],
              [specify that you are building on MacOSX 10.6])], 
            [with_snow_leopard=yes],
            [with_snow_leopard=no]
)

AC_ARG_ENABLE([ltr_32lib_on_x64],
		[AS_HELP_STRING([--enable-ltr-32lib-on-x64],
				[build 32bit linuxtrack library on 64bit host @<:@default=yes@:>@])],
		[with_ltr_32lib_on_x64=$enableval],
		[with_ltr_32lib_on_x64=yes]
)

AC_ARG_ENABLE([wine_plugin],
            [AS_HELP_STRING([--enable-wine-plugin],
              [build wine plugin (requires winegcc, wineg++ and makensis) @<:@default=no@:>@])], 
            [with_wine_plugin=yes],
            [with_wine_plugin=no]
)

#AC_ARG_ENABLE([face_tracker],
#            [AS_HELP_STRING([--enable-face-tracker],
#              [enables building of face tracker (requires OpenCV 2.X)])], 
#            [with_face_tracker=yes],
#            [with_face_tracker=no]
#)


AS_IF([case $host_os in darwin*) true;; *) false;; esac],
  [    
    AC_DEFINE([DARWIN], [], [We are on Darwin])
    with_darwin=yes
    OPENCV_LIBS=
    OPENCV_CFLAGS=
    AC_ARG_WITH([opencv-libs],
            [AS_HELP_STRING([--with-opencv-libs],
              [specify OpenCV libraries to link aginst])], 
            [OPENCV_LIBS=${with_opencv_libs}]
    )
    AC_ARG_WITH([opencv-cflags],
            [AS_HELP_STRING([--with-opencv-cflags],
              [specify OpenCV includes])], 
            [OPENCV_CFLAGS=${with_opencv_cflags}]
    )
  ]
)

PKG_CHECK_MODULES([OPENCV], [opencv >= 2.0],
            [with_face_tracker=yes],
            [with_face_tracker=no]
)
AC_SUBST([OPENCV_CFLAGS])
AC_SUBST([OPENCV_LIBS])


case "$host_cpu" in
i[[3456]]86) with_64_bit=no ;;
x86_64) with_64_bit=yes ;;
*) AC_MSG_ERROR([unsupported CPU type]) ;;
esac


AS_IF([test x$(uname -o) = x"GNU/Linux"], [with_linux=yes])


AM_CONDITIONAL(XPLANE_PLUGIN, [test x$with_xplane_plugin = xyes])
AM_CONDITIONAL(CWIID, [test  "x$ac_cv_lib_cwiid_cwiid_open" = xyes])
AM_CONDITIONAL(V4L2, [test x$with_v4l2 = xyes])
AM_CONDITIONAL(LIBV4L2, [test x$with_libv4l2 = xyes])
AM_CONDITIONAL(DARWIN, [test x$with_darwin = xyes])
AM_CONDITIONAL(SNOW_LEOPARD, [test x$with_snow_leopard = xyes])
AM_CONDITIONAL(LIBUSB, [test "x$ac_cv_lib_usb_usb_open" = xyes])
AM_CONDITIONAL(LIBUSB10, [test "x$ac_cv_lib_usb_1_0_libusb_init" = xyes])
AM_CONDITIONAL(OPENUSB, [test  "x$ac_cv_lib_openusb_openusb_init" = xyes])
AM_CONDITIONAL(LIBZ, [test  "x$ac_cv_lib_z_gzopen" = xyes])
AM_CONDITIONAL(X64, [test  "x$with_64_bit" = xyes])
AM_CONDITIONAL([LTR32ON64], [test  "x$with_ltr_32lib_on_x64" = xyes])
AM_CONDITIONAL([LTR_PIPE], [test  x$with_ltr_pipe = xyes])
AM_CONDITIONAL([LINUX], [test x$with_linux = xyes])
AM_CONDITIONAL(FACE_TRACKER, [test x$with_face_tracker = xyes])
AM_CONDITIONAL(WINE_PLUGIN, [test x$with_wine_plugin = xyes])
AM_CONDITIONAL(LIBQXT, [test  "x$ac_cv_lib_QxtGui__ZN17QxtGlobalShortcutD0Ev" = xyes])

# Checks for library functions.
AC_CHECK_FUNCS([atexit])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([gettimeofday])


AC_CONFIG_FILES([
 Makefile
 src/Makefile
 src/pathconfig.h
 src/tests/Makefile
 src/mac/Makefile
 src/local_config.h
 src/qt_gui/ltr_gui.pro
 src/wii_server/wii_server.pro
 src/helper/Makefile
 src/wine_bridge/Makefile
 doc/Makefile
 src/linuxtrack.pc
 src/mickey/mickey.pro
])
AC_OUTPUT

echo "=============================================================="
echo -n "Building Webcam support       "
AS_IF([test  "x$with_libv4l2" = xyes], [echo "yes"], [echo "no"])

echo -n "Building Wiimote support      "
AS_IF([test  "x$ac_cv_lib_cwiid_cwiid_open" = xyes], [echo "yes"], [echo "no"])

echo -n "Building TrackIR support      "
AS_IF([(test  "x$ac_cv_lib_usb_1_0_libusb_init" = xyes) \
             && (test "x$ac_cv_lib_z_gzopen" = xyes)], [echo "yes"], [echo "no"])

echo -n "Building Facetracker support  "
AS_IF([test  "x$with_face_tracker" = xyes], [echo "yes"], [echo "no"])

echo -n "Building Virtual Mouse        "
AS_IF([test  "x$ac_cv_lib_QxtGui__ZN17QxtGlobalShortcutD0Ev" = xyes], [echo "yes"], [echo "no"])

echo -n "Building XPlane plugin        "
AS_IF([test  "x$with_xplane_plugin" = xyes], [echo "yes"], [echo "no"])

echo -n "Building Wine plugin          "
AS_IF([test  "x$with_wine_plugin" = xyes], [echo "yes"], [echo "no"])

echo -n "Building ltr_pipe             "
AS_IF([test  "x$with_ltr_pipe" = xyes], [echo "yes"], [echo "no"])



