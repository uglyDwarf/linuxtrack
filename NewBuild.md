# Introduction #

This page provides instructions how to build Linuxtrack on linux and Mac.

## Building and installing linuxtrack on Linux ##

First of all, some prerequisities have to be installed - find them in your favorite package manager.

They include (names from Ubuntu, in other distros might differ a bit):

```
build-essential
autotools-dev
libtool
libc6-dev-i386 (on amd64 arch)
gcc-4.4-multilib (amd64 arch)
gobjc
libqt4-dev
libqt4-opengl-dev
libusb-1.0-0-dev
libcwiid-dev
zlib1g-dev
libv4l-dev
bison
flex
```

Now lets assume you checked out sources from the repository into
'/home/simmer/linux-track-read-only' using this command

```
cd /home/simmer/
svn checkout http://linux-track.googlecode.com/svn/trunk linux-track-read-only
```

You should also install XPlane SDK version 2.0.1 and higher (default path is '/usr/include/xplane\_sdk' - e.g there is file '/usr/include/xplane\_sdk/XPLM/XPLMCamera.h'), or specify option
'--with-xplane-sdk' (e.g --with-xplane-sdk=/home/simmer/SDK/CHeaders).

Also decide where the linuxtrack should be installed using --prefix option;
in this example we are installing it into /home/simmer/ltr directory.

```
cd /home/simmer/linux-track-read-only
./configure --prefix=/home/simmer/ltr --with-xplane-sdk=/home/simmer/SDK/CHeaders
make
make install
```

It is possible that you'll see several warnings about relinking libraries (they happen when you have linuxtrack already installed) and one saying that libltr.la wasn't installed. You can safely ignore them.

Now linuxtrack is installed along with the gui and you need to make it available...
Add following to the .bashrc file (I assume your shell is bash)

```
export PATH=${PATH}:/home/simmer/ltr/bin
```

This line makes linuxtrack binaries available without specifying full path to them.

Last but not least - if you intend to use TrackIR, you'll have to install
file 51-TIR.rules to your udev rules directory (might be /lib/udev/rules.d, but
refer to your distro documentation). You can find this file in 'prefix/share/linuxtrack' directory. Then either restart udev service, or reboot the computer and replug the TrackIR.


## Building and installing on Mac ##
Please note, that compiling Linuxtrack for Mac is usually not necessary - you can grab the .dmg archive in the Downloads section - it should work on both PowerPC and Intel Macs in MacOS X 10.4 to 10.7...

Also note that due to supporting Tiger and PowerPC, development platform of choice is MacOS X 10.5. However, the compilation should work on 10.6 too(although I'm not using that, so I can't guarantee that)...

First of all, there are some prerequisities to download...
  * **libusb-1.0.8.tar.bz2** - http://sourceforge.net/projects/libusb/files/libusb-1.0/
  * **XPlane SDK 2.0.1** - http://www.xsquawkbox.net/xpsdk/XPSDK201.zip
  * **Qt SDK for Mac (Qt Libraries should do too)** - http://qt.nokia.com/downloads

Install the Qt, now create a build directory somewhere and copy libusb-1.0.8.tar.bz2, bulk\_config\_data.tar.gz and unpacked SDK there too.

Finally open terminal, go to the build directory and check out the linuxtrack itself using command:

```
svn checkout http://linux-track.googlecode.com/svn/trunk
```

Now the structure should look like this:
```
...
   build
      michal                  - checked out from svn
      libusb-1.0.8.tar.bz2    - you can download it from sourceforge
      bulk_config_data.tar.gz - trackIR firmware
      SDK                     - unpacked XPlane SDK
```

Go to the _michal_ directory and run:
```
./mac_build
```

This should compile linuxtrack and create .dmg containing all that is needed.