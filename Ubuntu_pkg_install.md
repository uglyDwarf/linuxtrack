# Introduction #

To install linuxtrack packages for Ubuntu versions 10.04 to 11.10 the following steps needs to be taken:
  * Download and import public key
  * Setup repository to use
  * Install linuxtrack package

# Details #

Start by downloading the public key, used to verify authenticity of linuxtrack packages.
This key is available in the download section of linuxtrack google code home, or at
http://www.linuxtrack.eu/repositories/new_pubkey.gpg

When you have the key file, verify its fingerprint by issuing the following commands in the terminal (assuming the key was downloaded to the Downloads directory in your home directory):
```
cd ~/Downloads
gpg --with-fingerprint new_pubkey.gpg
```

The fingerprint should be exactly:
```
7546 4AFF 0819 6C02 8537  EAF3 EF44 396A 9F27 60DC
```

**If the fingerprint doesn't match, please contact me immediately and don't import the key!**

When you have verified the authenticity of the key, open the "Software sources" application and go to the "Authentication" tab.

![http://linux-track.googlecode.com/svn/wiki/img/inst_auth_pre.png](http://linux-track.googlecode.com/svn/wiki/img/inst_auth_pre.png)

Press the "Import key file..." and select the new\_pubkey.gpg file you just downloaded. You'll be prompted for your password and when finished, you should see something like this:

![http://linux-track.googlecode.com/svn/wiki/img/inst_auth_post.png](http://linux-track.googlecode.com/svn/wiki/img/inst_auth_post.png)

Now the key is imported, it is time to specify the repository location.
Go to the "Other software" tab and click "Add..." button.

![http://linux-track.googlecode.com/svn/wiki/img/inst_addr_pre.png](http://linux-track.googlecode.com/svn/wiki/img/inst_addr_pre.png)

In the dialog, enter one of the following:

  * on Ubuntu 12.04 Precise Pangolin
`deb http://www.linuxtrack.eu/repositories/ubuntu precise main`

  * on Ubuntu 11.10 Oneiric Ocelot
`deb http://www.linuxtrack.eu/repositories/ubuntu oneiric main`

  * on Ubuntu 11.04 Natty Narwhal
`deb http://www.linuxtrack.eu/repositories/ubuntu natty main`

  * on Ubuntu 10.10 Maverick Meerkat
`deb http://www.linuxtrack.eu/repositories/ubuntu maverick main`

  * on Ubuntu 10.04 Lucid Lynx
`deb http://www.linuxtrack.eu/repositories/ubuntu lucid main`

When "Add source" button is pressed, you'll be prompted for your password again and then you should see something like this:

![http://linux-track.googlecode.com/svn/wiki/img/inst_addr_post.png](http://linux-track.googlecode.com/svn/wiki/img/inst_addr_post.png)

Now you can close the "Software sources" application and you have to refresh informations
about available packages.

This can be done in several ways:
  * using "Synaptic Package Manager" by pressing the Refresh button
  * using the "Update Manager" application by pressing the "Check" button
  * using commandline
```
sudo apt-get update
```

Now you are ready to install the linuxtrack:
  * using "Synaptic Package Manager", search for linuxtrack and linuxtrack-dev
  * using commandline
```
sudo apt-get install linuxtrack
```

To uninstall the package:
  * using "Synaptic Package Manager", search for linuxtrack and either remove or purge it
  * using commandline
```
sudo apt-get purge linuxtrack
sudo apt-get autoremove
```

## Note for TrackIR users ##
To access TrackIR, you have to have permission to do that; if you go to the /usr/share/linuxtrack, you'll find there file 51-TIR.rules containing udev rules granting you this permission. This file has to be installed to the /lib/udev/rules.d directory.
The following command does that:
```
sudo cp /usr/share/linuxtrack/51-TIR.rules /lib/udev/rules.d
```
Now either reboot the machine, or you can do it the linux-way:
```
sudo service udev restart
```
and then replug/plug-in the TrackIR and you are good to go.