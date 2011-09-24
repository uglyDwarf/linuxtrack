#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([LinuxTrack], [0.0], [http://code.google.com/p/linux-track/issues/list])
AM_INIT_AUTOMAKE([no-dependencies -Wall -Werror])
AC_CONFIG_SRCDIR([src/tir4_driver.h])
AC_CONFIG_HEADERS([config.h])
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

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt], [LIBM=true], [AC_MSG_FAILURE([I need libm!])])
AC_CHECK_LIB([z], [gzopen], [LIBZ=true])
AC_CHECK_LIB([pthread], [pthread_create], [LIBPTHREAD=true], [AC_MSG_FAILURE([I need libpthread!])])
AC_CHECK_LIB([usb], [usb_open], [LIBUSB=true])
#AC_CHECK_LIB([openusb], [openusb_init], [LIBOPENUSB=true])
AC_CHECK_LIB([usb-1.0], [libusb_init], [LIBUSB10=true])
AC_CHECK_LIB([cwiid], [cwiid_open], [LIBCWIID=true])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h assert.h])
AC_CHECK_HEADERS([stdlib.h unistd.h netdb.h sys/socket.h fcntl.h sys/ioctl.h])

AC_CHECK_FILE([tir4.fw.gz], [with_tir_fw=yes], [with_tir_fw=no])

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
  [[#include  <linux/videodev2.h>]]                                                                                                           
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


AS_IF([case $host_os in darwin*) true;; *) false;; esac],
  [    
    AC_DEFINE([DARWIN], [], [We are on Darwin])
    with_darwin=yes
  ]
)

case "$host_cpu" in
i[[3456]]86) with_64_bit=no ;;
x86_64) with_64_bit=yes ;;
*) AC_MSG_ERROR([unsupported CPU type]) ;;
esac


AS_IF([test x$(uname -o) = x"GNU/Linux"], [with_linux=yes])


AM_CONDITIONAL(XPLANE_PLUGIN, [test x$with_xplane_plugin = xyes])
AM_CONDITIONAL(CWIID, [test  "x$ac_cv_lib_cwiid_cwiid_open" = xyes])
AM_CONDITIONAL(V4L2, [test x$with_v4l2 = xyes])
AM_CONDITIONAL(DARWIN, [test x$with_darwin = xyes])
AM_CONDITIONAL(SNOW_LEOPARD, [test x$with_snow_leopard = xyes])
AM_CONDITIONAL(LIBUSB, [test "x$ac_cv_lib_usb_usb_open" = xyes])
AM_CONDITIONAL(LIBUSB10, [test "x$ac_cv_lib_usb_1_0_libusb_init" = xyes])
AM_CONDITIONAL(OPENUSB, [test  "x$ac_cv_lib_openusb_openusb_init" = xyes])
AM_CONDITIONAL(LIBZ, [test  "x$ac_cv_lib_z_gzopen" = xyes])
AM_CONDITIONAL(TIR_FW, [test  "x$with_tir_fw" = xyes])
AM_CONDITIONAL(X64, [test  "x$with_64_bit" = xyes])
AM_CONDITIONAL([LTR_PIPE], [test  x$with_ltr_pipe = xyes])
AM_CONDITIONAL([LINUX], [test x$with_linux = xyes])

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
 doc/Makefile
])
AC_OUTPUT
