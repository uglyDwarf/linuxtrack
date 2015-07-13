# Introduction #

Beginning with Linuxtrack 1.0 Beta 1 there is a new application called **mickey** - the virtual mouse. It allows to use your headtracker to
control a mouse cursor (and using dedicated hardware (namely SmartNav4) you can also emulate clicks).

For now, mickey is linux only, but MacOS version is coming soon.

# Details #

To enable mickey, you have to have access to the /dev/uinput file; there are several ways to achieve this goal and I'm still investigating which of them is the best.

For the moment, I'd recommend this one (issue the following commands in terminal):
```
cd /opt/linuxtrack_130217/usr/shared/linuxtrack #adjust according to your version
sudo cp 51-Mickey.rules /lib/udev/rules.d #add udev rule for uinput
sudo addgroup uinput #add new group uinput
sudo usermod -a -G uinput $USER #add yourself to uinput group
sudo chgrp uinput /dev/uinput #to avoid a need to reboot
sudo chmod 660 /dev/uinput #dtto
```

Now you can start using mickey.

# Tips #
Although you can use mickey with three point setup, or even pure headtracking, it is recommended to use single point model for its greater accuracy.
