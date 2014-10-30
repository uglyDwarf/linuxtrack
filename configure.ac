#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([LinuxTrack], [0.99.12], [http://code.google.com/p/linux-track/issues/list])
AM_INIT_AUTOMAKE([no-dependencies -Wall -Werror])
AC_CONFIG_SRCDIR([src/tir4_driver.h])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CXX
AC_PROG_OBJC
AS_IF([test "$debug" != "no"],
      [CFLAGS+=" -g -O2 -Wall -Wextra -Wformat -Wformat-security \
         --param ssp-buffer-size=4 -fstack-protector -D_FORTIFY_SOURCE=2 "
       CXXFLAGS+=" -g -O2 -Wall -Wextra -Wformat -Wformat-security \
         --param ssp-buffer-size=4 -fstack-protector -D_FORTIFY_SOURCE=2 "
       CPPFLAGS+=" -D_FORTIFY_SOURCE=2 -O2 "],
      [CFLAGS+=" -O3 " CXXFLAGS+=" -O3 "])

# Checks for programs.
AC_CHECK_PROG(BISON, [bison], [yes], )
AS_IF([test x$BISON = x],
 [AC_MSG_FAILURE([bison not found!])]
)
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt], [LIBM=true], [AC_MSG_FAILURE([I need libm!])])
AC_CHECK_LIB([z], [gzopen], [LIBZ=true])
AC_CHECK_LIB([pthread], [pthread_create], [LIBPTHREAD=true], [AC_MSG_FAILURE([libpthread not found!])])
#AC_CHECK_LIB([openusb], [openusb_init], [LIBOPENUSB=true])
AC_CHECK_LIB([usb-1.0], [libusb_init], [LIBUSB10=true])
AC_CHECK_LIB([cwiid], [cwiid_open], [LIBCWIID=true])
AC_CHECK_LIB([mxml], [mxmlNewXML], [LIBMXML=true], [AC_MSG_FAILURE([libmxml not found!])], [-pthread])
AC_CHECK_LIB([crypto], [SHA1], [LIBCRYPTO=true])


# Checks for header files.
AC_CHECK_HEADERS([stdint.h assert.h])
AC_CHECK_HEADERS([stdlib.h unistd.h netdb.h sys/socket.h fcntl.h sys/ioctl.h])

AC_PATH_PROGS(QMAKE_PATH, [qmake-qt4 qmake])

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
    AC_DEFINE([V4L2], [],[Use videodev2.h (V4L2)])
    with_v4l2=yes
  ], [],
  [[#include  <linux/videodev2.h>
  ]]
)

AC_CHECK_HEADER([libv4l2.h],
  [
    AC_DEFINE([LIBV4L2], [],[Use libv4l2.h (V4L2)])
    with_libv4l2=yes
  ], [], []
)

AC_CHECK_HEADER([mxml.h], [], [AC_MSG_FAILURE([mxml.h is needed for ltr_gui compilation!])])
AC_CHECK_HEADER([linux/uinput.h], [build_mickey=yes], [build_mickey=no])

AC_CHECK_PROG([with_wine64], [wine64], [yes])
AC_CHECK_PROG([with_makensis], [makensis], [yes])

XPL_CPPFLAGS=
AC_ARG_WITH([xplane_sdk],
            [AS_HELP_STRING([--with-xplane-sdk],
              [specify path to XPlane SDK @<:@default=/usr/include/xplane_sdk@:>@])],
            [XPL_CPPFLAGS=" -I${with_xplane_sdk}/XPLM -I${with_xplane_sdk}/Widgets "],
            [XPL_CPPFLAGS=" -I/usr/include/xplane_sdk/XPLM -I/usr/include/xplane_sdk/Widgets "]
)

BCK_CPPFLAGS="$CPPFLAGS"
CPPFLAGS+="$XPL_CPPFLAGS -DLIN=1"
AC_CHECK_HEADER([XPLMPlugin.h], [build_xplane_plugin=yes], [build_xplane_plugin=no])
CPPFLAGS="$BCK_CPPFLAGS"
AC_SUBST([XPL_CPPFLAGS])

AC_ARG_ENABLE([ltr_32lib_on_x64],
		[AS_HELP_STRING([--enable-ltr-32lib-on-x64],
				[build 32bit linuxtrack library on 64bit host @<:@default=yes@:>@])],
		[with_ltr_32lib_on_x64=$enableval],
		[with_ltr_32lib_on_x64=yes]
)

WINE_LIBS=
AC_ARG_WITH([wine-libs],
            [AS_HELP_STRING([--with-wine-libs],
              [specify Wine libraries to link aginst])],
            [WINE_LIBS=${with_wine_libs}]
)

WINE64_LIBS=
AC_ARG_WITH([wine64-libs],
            [AS_HELP_STRING([--with-wine64-libs],
              [specify Wine64 libraries to link aginst])],
            [WINE64_LIBS=${with_wine64_libs}]
)

LIB32DIR="lib32"
AC_ARG_WITH([lib32-dir],
            [AS_HELP_STRING([--with-lib32-dir],
              [specify 32bit library directory (usually needed only on amd64 arch.)])],
            [LIB32DIR=${with_lib32_dir}]
)



BCK_CC="$CC"
BCK_CFLAGS="$CFLAGS"
BCK_CPPFLAGS="$CPPFLAGS"
BCK_LDFLAGS="$LDFLAGS"
BCK_LIBS="$LIBS"
BCK_EXEEXT="$ac_exeext"
AC_LANG_PUSH([C])
CC="winegcc"
ac_exeext=".exe"

CFLAGS=""

AS_IF([test x$with_wine64 = xyes],
 [
   LDFLAGS=" $WINE64_LIBS "
   AC_MSG_CHECKING([64bit winegcc])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [WINEGCC_COMPILES=yes], [WINEGCC_COMPILES=no])
   AS_IF([test  "x$WINEGCC_COMPILES" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
   AC_MSG_CHECKING([64bit wineg++])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [WINEGPP_COMPILES=yes], [WINEGPP_COMPILES=no])
   AS_IF([test  "x$WINEGPP_COMPILES" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
 ]
)

CFLAGS=" -m32 "
LDFLAGS=" $WINE_LIBS "
AC_MSG_CHECKING([32bit winegcc])
AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [WINEGCC32_COMPILES=yes], [WINEGCC32_COMPILES=no])
AS_IF([test  "x$WINEGCC32_COMPILES" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
AC_MSG_CHECKING([32bit wineg++])
AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [WINEGPP32_COMPILES=yes], [WINEGPP32_COMPILES=no])
AS_IF([test  "x$WINEGPP32_COMPILES" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])
AC_LANG_POP([C])
ac_exeext="$BCK_EXEEXT"
LIBS="$BCK_LIBS"
LDFLAGS="$BCK_LDFLAGS"
CPPFLAGS="$BCK_CPPFLAGS"
CFLAGS="$BCK_CFLAGS"
CC="$BCK_CC"

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

m4_ifdef([PKG_CHECK_MODULES], [PKG_CHECK_MODULES([OPENCV], [opencv >= 2.0])])


AC_SUBST([OPENCV_CFLAGS])
AC_SUBST([OPENCV_LIBS])
AC_SUBST([WINE_LIBS])
AC_SUBST([WINE64_LIBS])
AC_SUBST([LIB32DIR])


case "$host_cpu" in
i[[3456]]86) with_64_bit=no ;;
x86_64) with_64_bit=yes ;;
*) AC_MSG_ERROR([unsupported CPU type]) ;;
esac

AS_IF([((test "x$pkg_cv_OPENCV_CFLAGS" != x) && (test "x$pkg_cv_OPENCV_LIBS" != x)) ||
       ((test "x$OPENCV_CFLAGS" != x) && (test "x$OPENCV_LIBS" != x))],
                                [build_face_tracker=yes], [build_face_tracker=no])

AS_IF([(test "x$WINEGCC32_COMPILES" = xyes) && (test "x$WINEGPP32_COMPILES" = xyes) &&
  (test "x$with_makensis" = xyes) ], [build_wine_plugin=yes], [build_wine_plugin=no])

AS_IF([(test "x$WINEGCC_COMPILES" = xyes) && (test "x$WINEGPP_COMPILES" = xyes) &&
  (test "x$with_makensis" = xyes) ], [build_wine64_plugin=yes], [build_wine64_plugin=no])


AM_CONDITIONAL(XPLANE_PLUGIN, [test x$build_xplane_plugin = xyes])
AM_CONDITIONAL(CWIID, [test  "x$ac_cv_lib_cwiid_cwiid_open" = xyes])
AM_CONDITIONAL(V4L2, [test x$with_v4l2 = xyes])
AM_CONDITIONAL(LIBV4L2, [test x$with_libv4l2 = xyes])
AM_CONDITIONAL(DARWIN, [test x$with_darwin = xyes])
AM_CONDITIONAL(LIBUSB10, [test "x$ac_cv_lib_usb_1_0_libusb_init" = xyes])
AM_CONDITIONAL(LIBZ, [test  "x$ac_cv_lib_z_gzopen" = xyes])
AM_CONDITIONAL(X64, [test  "x$with_64_bit" = xyes])
AM_CONDITIONAL([LTR32ON64], [test  "x$with_ltr_32lib_on_x64" = xyes])
AM_CONDITIONAL(FACE_TRACKER, [test x$build_face_tracker = xyes])
AM_CONDITIONAL(MICKEY, [test x$build_mickey = xyes])
AM_CONDITIONAL(WINE_PLUGIN, [test x$build_wine_plugin = xyes])
AM_CONDITIONAL(WINE64, [test x$build_wine64_plugin = xyes])
AM_CONDITIONAL(CRYPTO, [test  "x$ac_cv_lib_crypto_SHA1" = xyes])
AM_CONDITIONAL(MXML, [test  "x$ac_cv_lib_mxml_mxmlNewXML" = xyes])

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
 src/mac/Makefile
 src/mac/Info.plist
 src/local_config.h
 src/qt_gui/ltr_gui.pro
 src/wii_server/wii_server.pro
 src/wine_bridge/Makefile
 src/wine_bridge/client/Makefile
 src/wine_bridge/ft_client/Makefile
 src/wine_bridge/controller/Makefile
 src/wine_bridge/tester/Makefile
 src/wine_bridge/ft_tester/Makefile
 src/wine_bridge/tester/npview.rc
 src/wine_bridge/ft_tester/fttester.rc
 src/wine_bridge/views/Makefile
 src/wine_bridge/ltr_wine64.nsi
 src/wine_bridge/ltr_wine.nsi
 doc/Makefile
 src/linuxtrack.pc
 src/mickey/mickey.pro
])
AC_OUTPUT

AC_MSG_NOTICE([==============================================================]);
AC_MSG_CHECKING([Webcam support])
AS_IF([(test  "x$with_libv4l2" = xyes) || (test  "x$with_darwin" = xyes)],
								[AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([Wiimote support])
AS_IF([(test  "x$ac_cv_lib_cwiid_cwiid_open" = xyes) || (test  "x$with_darwin" = xyes)],
                                [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([TrackIR support])
AS_IF([(test  "x$ac_cv_lib_usb_1_0_libusb_init" = xyes) \
             && (test "x$ac_cv_lib_z_gzopen" = xyes)], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([Facetracker support])
AS_IF([(test  "x$build_face_tracker" = xyes)], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([XPlane plugin])
AS_IF([test  "x$build_xplane_plugin" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([Mickey])
AS_IF([test  "x$build_mickey" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])

AC_MSG_CHECKING([Wine plugin])
AS_IF([test  "x$build_wine_plugin" = xyes], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no])])




