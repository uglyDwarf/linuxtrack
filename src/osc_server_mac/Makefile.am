EXTRA_DIST = OscServer-Info.plist English.lproj/InfoPlist.strings \
English.lproj/MainMenu.xib PkgInfo linuxtrack.icns 

AM_CFLAGS = -arch x86_64
SLEOPARD_CFLAG = 
SLEOPARD_LDFLAG = -L..
DARWINFLAGS = -undefined dynamic_lookup 

if DARWIN
  bin_PROGRAMS = OscServer
  noinst_SCRIPTS = English.lproj/MainMenu.nib
endif

OscServer_SOURCES = ../linuxtrack.h OscServerAppDelegate.h OscServerAppDelegate.m \
  osc_server.c osc_server.h main.m ../linuxtrack.c
OscServer_LDADD = ../libltr.la -lobjc -lpthread -llo -ldl
OscServer_LDFLAGS = ${AM_CFLAGS} ${SLEOPARD_LDFLAG} -framework Cocoa -framework CoreData \
  -framework AppKit -framework Foundation
OscServer_CFLAGS = ${AM_CFLAGS} ${SLEOPARD_CFLAG} -I${srcdir} -I${srcdir}/.. -I..
OscServer_OBJCFLAGS = ${AM_CFLAGS} ${SLEOPARD_CFLAG} -I${srcdir} -I${srcdir}/.. -I..

English.lproj/MainMenu.nib : English.lproj/MainMenu.xib
	ibtool --errors --warnings --notices --output-format human-readable-text \
      English.lproj/MainMenu.xib --compile English.lproj/MainMenu.nib
