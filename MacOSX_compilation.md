# These instructions are intended for the version under tags/no\_gui. If you intend to compile the newest version, follow the NewBuild instructions! #

# Current status #
Should be usable...
# Introduction #

Here are brief instructions that should help you to compile Linuxtrack on Mac OSX.

**Track IR 4 and 5 should be supported!**
(as I only have TIR4, TIR5 working is yet to be confirmed)

  1. Download and compile libusb-1.0
  1. Download XPlane SDK
  1. Download and compile linuxtrack

The rest of the process is the same as on linux - you have to copy the plugin to your XPlane installation, download firmware (look at ReadmeFirst page), copy .linuxtrack  file to your home directory and finally customize it...

# Details #
## XCode ##
Make sure you have XCode installed - if not, it is on your system DVD, so install it.

## Download and compile libusb-1.0 ##
Go to http://sourceforge.net/projects/libusb/ and download libusb-1.0...

Open the terminal, unpack the archive and compile the library:

```
mkdir linuxtrack
cd linuxtrack
cp ~/Downloads/libusb-1.0.4.tar.bz2 .
tar xfj libusb-1.0.4.tar.bz2
export CFLAGS="-arch i386"
cd libusb-1.0.4
./configure --prefix=/Users/michal/linuxtrack/libusb
make
make install
```

Remarks:
  * We assume you are going to use /Users/michal/linuxtarck directory for plugin staff, change paths accordingly...
  * If the system didn't unpack the archive for you, change the 'xf' options in tar command to 'xfz' for **.gz, or 'xfj' for**.bz2 archive.
  * The CFLAGS setting should take care of problems compiling on Snow Leopard (which is 64 bit system, while XPlane needs 32 bit plugins). Thanks to Frederik B. for suggesting this.

## Download XPlane SDK ##
Go to http://www.xsquawkbox.net/xpsdk/mediawiki/Download and download XPlane SDK (I use older 1.0.2, but 2.0 version should work too).

Just unpack it to the '~/linuxtrack' directory.

Now ~/linuxtrack contains two subdirs - 'libusb' and 'SDK'.

## Download and compile linuxtrack ##
Checkout linuxtrack source and compile it:

```
cd ~/linuxtrack
svn checkout http://linux-track.googlecode.com/svn/trunk/ linux-track-read-only
cd linux-track-read-only
export CFLAGS=-I/Users/michal/linuxtrack/libusb/include
export LDFLAGS=-L/Users/michal/linuxtrack/libusb/lib
./configure --disable-dependency-tracking
make
```

Remarks:
  * Be sure to adjust CFLAGS and LDFLAGS accordingly and use absolute paths!

## Download firmware ##
Download firmware files from http://media.naturalpoint.com/software/external/bulk_config_data.tar.gz and unpack them to a directory of your choice. Later on, when editing .linuxtrack, uncomment Store-directory key and specify your path there.

## Final touches ##
Go to linux-track-read-only/src and copy .linuxtrack file to your home directory. Then edit the file (there is a wikipage on the subject) to suit you.

First of all comment line 'Input = TrackIR4' and uncomment 'Input = TrackIR' - this enables the new driver.

Then uncomment 'Store-directory' key and specify the directory holding firmware files there (I didn't tested paths with spaces - it might cause problems).

If you use reflective TrackClip, change 'IR-LEDs = off' to 'IR-LEDs = on'.

Finally copy xlinuxtrack.xpl file to your XPlane - exactly to Resources/plugins directory.

## Problem? ##
If you encounter a problem, look to the directory where your XPlane executable resides and check log.txt file (lowercase!). Also running XPlane from terminal can give you some interesting info... If the log file doesn't make sense to you, please include it with the error description, also .linuxtrack file from your home directory and short description of your setup - TrackIR4/TrackIR5 with TrackClip/Vector clip...