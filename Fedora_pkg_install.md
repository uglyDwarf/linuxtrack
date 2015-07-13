# Introduction #

This document describes installation process of Linuxtrack packages on Fedora 14 to 16.

# Details #

Start by downloading linuxtrack.repo from the project downloads page. I'am assuming the file is downloaded to the Downloads directory inside your home directory.

Now open terminal and run the following commands:
```
> cd ~/Downloads
> sudo cp linuxtrack.repo /etc/yum.repos.d
```
This will enable Linuxtrack repository; the last command can also ran as root instead of sudo, depending on your preference (and as always - when having root privileges, make sure you know what are you doing!).

Now the Linuxtrack is ready to be installed by the following command:

```
> sudo yum install linuxtrack linuxtrack-xplane-plugin
...
Retrieving key from http://www.linuxtrack.eu/repositories/fedora-pubkey.gpg
Importing GPG key 0x061A0257:
 Userid: "Michal Navratil <f.josef@email.cz>"
 From  : http://www.linuxtrack.eu/repositories/fedora-pubkey.gpg
Is this ok [y/N]: y
...
Total download size: 1.9 M
Installed size: 8.7 M
Is this ok [y/N]: y
...
Complete!
```

You'll be asked several times to confirm - at first to allow public key import, then to confirm Linuxtrack installation (with dependencies).

When done, you can start ltr\_gui either from terminal, or via menu Games.
If you find yourself lost, just click the Help button and it should provide you with enough info to set the tracking up.

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