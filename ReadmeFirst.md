# Introduction #

Here is short walk-through made by Bill that should help you to get linuxtrack up and going.

# Details #

# Using SVN to get linuxtrack #
  * You are going to need svn installed, if you don't have it, you need to install it (on Ubuntu use Synaptic Package Manager, other distros have different packaging systems)
  * make a directory in your home to hold two subdirectories (I called mine XLinuxTracker but could be anything)
  * download XPlane SDK (http://www.xsquawkbox.net/xpsdk/mediawiki/Main_Page) and extract it into that directory
  * start up your terminal and cd to the directory you made
  * use the following command to download all the files you need (this will get the latest version)
```
svn co http://linux-track.googlecode.com/svn/trunk/ linux-track-read-only
```

> If you wanted to use older revision, e.g. [revision 84](https://code.google.com/p/linux-track/source/detail?r=84), use
```
svn co http://linux-track.googlecode.com/svn/trunk/ linux-track-read-only_84 -r84
```

  * You will now have two subdirectories - SDK and linux-track-read-only
  * Now get to the business:
```
cd linux-track-read-only
./configure
make
```
> Be sure to check after configure is run if there aren't any errors. It is possible that you'll have to install cwiid and libusb libraries (also development packages for those libraries!)
> If all goes well you will find the following files in the linux-track-read-only/src
> > subdirectory: 'xlinuxtrack.xpl' and '.linuxtrack'

  * xlinuxtrack.xpl goes in X-Plane/Resources/plugins directory
  * .linuxtrack (which is hidden normaly) goes in your home directory.

> Edit its contents (for the moment there is no GUI that would do it for you, but it is comming soon...) to fill in type of model you use (clip or cap), dimmensions of your model, device you are going to use (tir, wiimote, webcam) and possibly few other parameters... **TODO - elaborate**
  * There are some dependences that have to be met to be able to compile some drivers (these packages are valid for ubuntu9.04):
    * 'trackir4' driver needs 'libusb-dev' and its dependences
> > > Another file that you will need is 'bulk\_config\_data.bin' this can be found from Natural Point at
```
         http://media.naturalpoint.com/software/external/bulk_config_data.tgz
```
> > > Unpack it to your X-Plane directory.
    * 'trackir' driver needs either 'libusb-1.0-0-dev' or 'libopenusb-dev' and its dependences
> > > Another file that you will need is 'bulk\_config\_data.bin' this can be found from Natural Point at
```
         http://media.naturalpoint.com/software/external/bulk_config_data.tar.gz
```
> > > Unpack it to some directory and fill-in the name of this directory into the .linuxtrack file under 'Store-directory' key (of course delete hash mark on the begining of the line...).
    * 'wiimote' driver needs 'libcwiid1-dev' and its dependences

  * I am using a TrackIr 4 Pro and needed a rule to make my Ubuntu 9.04 work.

> (otherwise you need to run X-Plane as root, which is very bad thing).
> I named the rule 51-TIR4.rules and put it in my /lib/udev/rules.d folder.

> This was because in the 50-udev-default.rules file there is a rule setting all usb devices to a lower permision then what we need to get the tracker to work.

> In it is the following line.
```
SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device",SYSFS{idVendor}=="131d", MODE="0666"
```
> This is going to work for TrackIR5 too...

  * Now when you run X-Plane, by pressing F8 you should be able to move you simulated head by moving your read head. In Plugins menu, there is linuxtrack item containing setup window. Use it to adjust sensitivities of different axes and possibly map functions to joystick buttons. When done, save prefs and enjoy!

  * When things go wrong, try running X-Plane from terminal and check messages you see there. Also check lt\_log.txt (all lowercase!) that resides in your X-Plane directory.
> In case of crash, try running X-Plane from gdb (debugger) and make it crash again. When it does, make backtrace log and send it to us - it will help us to find what went wrong and hopefully to correct it. **TODO - elaborate on backtrace**

# Building on 64 bit system #
This note applies directly to Ubuntu 8 (Hardy Heron), but I guess it should give people an overview of what to do on other distros too...
When trying to build linuxtrack on 64 bit system, you can get following message when running configure:
```
...
checking for usb_open in -lusb... no
configure: error: libusb not found
```
although you are sure you have that library installed.
First of all take a look to file configure.log - chances are you'll see something similar to this snippet:
```
configure:4088: checking for usb_open in -lusb
configure:4123: gcc -o conftest -g -O0 -Wall -m32 conftest.c -lusb -lpthread -lm >&5
/usr/bin/ld: skipping incompatible /usr/lib/gcc/x86_64-linux-gnu/4.3.2/../../../libusb.so when searching for -lusb
/usr/bin/ld: skipping incompatible /usr/lib/gcc/x86_64-linux-gnu/4.3.2/../../../libusb.a when searching for -lusb
/usr/bin/ld: skipping incompatible /usr/lib/libusb.so when searching for -lusb
/usr/bin/ld: skipping incompatible /usr/lib/libusb.a when searching for -lusb
/usr/bin/ld: cannot find -lusb
collect2: ld returned 1 exit status
```
This means, that gcc found library but it wasn't what he was looking for. Why? Well, linuxtrack is 32 bit application and all he found is a 64 bit library.

The solution to this problem is not that hard - you have to get 32bit version of the library and tell the configure where to search for it.
On Ubuntu, all (or most at least) 32 bit libraries are present in the package "ia32-libs",
and looking at its contents you can see that libusb-0.1 is there (for other distros consult their packaging systems on which package you need). Now install the package and go to /lib32 (or /usr/lib32, if the library you need is there) and check that the library is there and it ends in '.so' (libusb-0.1.so is OK, libusb-0.1.so.4 is not). If you get only something like libusb-0.1.so.4, make a link that has a name you need
```
cd /lib32
sudo ln -s libusb-0.1.so.4 libusb-0.1.so
```
The lase step is to inform configure where to find the library (assuming it is in /lib32, if not, modify the line accordingly; also assuming you use bash)
```
LDFLAGS="-L/lib32 -Wl,-rpath,/lib32"
export LDFLAGS
./configure
```

Now all should work just fine... Or at least you should get a bit farther than before.