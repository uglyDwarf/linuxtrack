# Make sure pkg-conf macross missing doesn't break build (on MacOSX)
AC_DEFUN([PKG_PROG_PKG_CONFIG], [])
AC_DEFUN([PKG_CHECK_MODULES],[echo "PKG-CONFIG not used on MacOSX"])
