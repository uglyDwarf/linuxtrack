SUBDIRS = src src/mac src/tests doc
if ! DARWIN
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

if TIR_FW
  dist_pkgdata_DATA += MD5SUM SHA1SUM  tir4.fw.gz tir5.fw.gz copyright_and_terms_of_use.txt
endif
