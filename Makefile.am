SUBDIRS = src src/mac src/tests doc 
if ! DARWIN
SUBDIRS += src/mickey
if X64
if LTR32ON64
  SUBDIRS += src/helper
endif
endif
endif

if WINE_PLUGIN
  SUBDIRS +=src/wine_bridge
endif


dist_pkgdata_DATA = README README.devel README.xplane
EXTRA_DIST = Readme.rtf deploy mac_build pkgconf_extract mac_m4/fake_pkg_conf.m4
