SUBDIRS = src src/mac src/tests doc 
if ! DARWIN
#SUBDIRS += src/mickey
if X64
if LTR32ON64
  SUBDIRS += src/helper
endif
endif
endif

if WINE_PLUGIN
  SUBDIRS +=src/wine_bridge/client src/wine_bridge/controller src/wine_bridge/tester \
  src/wine_bridge/ft_client src/wine_bridge/ft_tester src/wine_bridge 
endif


dist_pkgdata_DATA = README README.devel README.xplane
EXTRA_DIST = Readme.rtf deploy mac_build pkgconf_extract mac_m4/fake_pkg_conf.m4
AM_DISTCHECK_CONFIGURE_FLAGS = --with-xplane-sdk=/home/qbuilder/devel/SDK/CHeaders \
  --enable-wine-plugin --enable-mickey \
  --with-wine-libs="-L/usr/lib/i386-linux-gnu -L/usr/lib/i386-linux-gnu/wine" \
  --with-wine64-libs="-L/usr/lib/x86_64-linux-gnu -L/usr/lib/x86_64/wine"
