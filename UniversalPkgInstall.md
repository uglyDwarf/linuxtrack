# How to install the universal Linuxtrack package #

Linuxtrack universal package can be used on distros, that aren't directly supported by means of native package.

Start by downloading the newest package from the Linuxtrack project home download page - choose the one that matches your distro's architecture. Also download the public key (new\_pubkey.gpg).

I assume you downloaded it to directory "Downloads" inside your home directory.

Open up the terminal, go to the Downloads directory and unpack the package (adjust to the package version you actually downloaded):

```
> cd ~/Downloads
> unzip linuxtrack_0.0_130127_64.zip
```

Before going any further, check the public key authenticity:

```
>gpg --with-fingerprint new_pubkey.gpg 
pub  4096R/9F2760DC 2012-10-17 Michal Navratil <f.josef@email.cz>
      Key fingerprint = 7546 4AFF 0819 6C02 8537  EAF3 EF44 396A 9F27 60DC
sub  4096R/8166BE4A 2012-10-17
sub  4096R/22343B2D 2012-10-17
```

Check that the fingerprint matches - **if not, don't import the key and please contact me immediately.**

If the fingerprint matches, you can import the public key and verify the package authenticity:

```
> gpg --import new_pubkey.gpg
> gpg --verify linuxtrack_0.0_130127_64.sig linuxtrack_0.0_130127_64.tar.gz 
gpg: Signature made Sun 27 Jan 2013 03:52:09 PM CET using RSA key ID 22343B2D
gpg: Good signature from "Michal Navratil <f.josef@email.cz>"
gpg: WARNING: This key is not certified with a trusted signature!
gpg:          There is no indication that the signature belongs to the owner.
Primary key fingerprint: 7546 4AFF 0819 6C02 8537  EAF3 EF44 396A 9F27 60DC
     Subkey fingerprint: 36B5 9CB9 17B1 8F22 5DAD  C147 873E 7850 2234 3B2D

```

The warning is OK as long as the fingerprint matches- it just means the key is not signed by someone you trust.

**If the package doesn't pass this check, don't install it and please contact me**.

Now the package can be safely installed:

```
> cd /opt
> sudo tar xfj ${HOME}/Downloads/linuxtrack-0.99.10-64.tar.bz2
```

Instead of using sudo commmand you can login as root if it is a prefered way of obtaining root privileges on your distro. Just be sure you know what are you doing, since as root you can severely damage the system!

**Please note, that linuxtrack has to be installed in /opt, otherwise it is not going to work (at least not without changing LD\_LIBRARY\_PATH to let it find its libraries).**

When the package is installed, to make it a little bit convenient, let's make a link to the actual package:

```
> sudo ln -s linuxtrack-0.99.10 linuxtrack
```

This way several Linuxtrack versions can coexist happily - just change the link (remove the old one first using command 'sudo rm -i linuxtrack') to point to the version of your choice and rerun ltr\_gui (just open it and close again to let it realize the version changed).

In order to get Linuxtrack working, you have to install its dependencies; this step will be different in each distro. First of all, enter the bin directory:

```
> cd linuxtrack/bin
```

At this point, you run this command, looking for missing libraries:

```
> ldd * | grep 'not found' | sort -u
```

If you get any output from this command, it lists libraries you have to install in order to have linuxtrack
ready. Look them up according to your distro - e.g. for RPM based distros look at rpm.pbone.net .
The same needs to be done for linuxtrack libraries:

```
> cd ../lib/linuxtrack
> ldd *.so  | grep 'not found' | sort -u
```

To give you an idea what to look for, here is a list from Ubuntu (the package names might slightly differ in your distro):
```
libqtgui4
locales
libusb-1.0
libcwiid
zlib1g
libv4l
```
Last thing to be done is to add linuxtrack to the path - add the following line towards the end of your _.bashrc_ file(it is in your home directory):

```
export PATH=${PATH}:/opt/linuxtrack/bin
```

Optionaly you can create a launchers for ltr\_gui and wii\_server using icons found at _/opt/linuxtrack/share/pixmaps_ .

When done, close the terminal and start another one (this will load the new PATH setting) and start Linuxtrack GUI by this command:
```
ltr_gui
```
If you find yourself lost, just click the Help button and it should provide you with enough info to set the tracking up.


## Note for TrackIR users ##
To access TrackIR, you have to have permission to do that; if you go to the /opt/linuxtrack-0.99.10/share/linuxtrack (assuming you installed version 0.99.10; for other versions please change the path accordingly), you'll find there file 51-TIR.rules containing udev rules granting you this permission. This file has to be installed to the /lib/udev/rules.d directory.
The following command does that:
```
sudo cp /opt/linuxtrack-0.99.10/share/linuxtrack/51-TIR.rules /lib/udev/rules.d
```
Now either reboot the machine, or you can do it the linux-way:
```
sudo service udev restart
```
and then replug/plug-in the TrackIR and you are good to go.

---


Any questions, comments and ideas concerning either this how-to or Linuxtrack are welcome!