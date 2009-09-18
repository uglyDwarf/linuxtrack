#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.59])
AC_INIT([LinuxTrack], [0.0], [http://code.google.com/p/linux-track/issues/list])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_SRCDIR([src/tir4_driver.h])
AC_CONFIG_HEADERS([config.h])

AS_IF([test "$debug" != "no"],
      [CFLAGS+=" -g -O0 -Wall "],
      [CFLAGS+=" -O3 "])
CFLAGS+=" -m32 "

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_RANLIB
AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([usb], [usb_open], , AC_MSG_ERROR([libusb not found]))
AC_CHECK_LIB([cwiid], [cwiid_open], 
  [
    AC_DEFINE([CWIID], [], [Enable wiimote support])
    with_cwiid=yes
  ],)

# Checks for header files.
AC_CHECK_HEADERS([stdint.h assert.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

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
            [XPLANE_SDK="../SDK"]
)

AC_ARG_ENABLE([xplane_plugin],
            [AS_HELP_STRING([--enable-xplane-plugin],
              [build XPlane plugin @<:@default=yes@:>@])], 
            [],
            [
                AC_CHECK_FILE([${XPLANE_SDK}/CHeaders/XPLM/XPLMUtilities.h],
                    [
                      AC_SUBST([XPLANE_SDK])
                      with_xplane_plugin=yes
                    ],
                    [AC_MSG_FAILURE([Invalid path of XPlane SDK specified])]
                )
            ]
)

AS_IF([case $host_os in darwin*) true;; *) false;; esac],
  [    
    AC_DEFINE([DARWIN], [], [We are on Darwin])
    with_darwin=yes
  ]
)


AM_CONDITIONAL(XPLANE_PLUGIN, [test x$with_xplane_plugin = xyes])
AM_CONDITIONAL(CWIID, [test x$with_cwiid = xyes])
AM_CONDITIONAL(V4L2, [test x$with_v4l2 = xyes])
AM_CONDITIONAL(DARWIN, [test x$with_darwin = xyes])






# Checks for library functions.

AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
AC_OUTPUT
