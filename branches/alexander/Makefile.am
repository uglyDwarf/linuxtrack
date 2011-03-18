SUBDIRS = src src/mac src/tests src/helper
dist_pkgdata_DATA = README

if TIR_FW
  dist_pkgdata_DATA += MD5SUM SHA1SUM  tir4.fw.gz tir5.fw.gz copyright_and_terms_of_use.txt
endif

if LTR_FGFS
  nobase_dist_doc_DATA = fgfs/linux-track.nas fgfs/linux-track.xml fgfs/README
endif

