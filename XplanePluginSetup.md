# Introduction #

This page explains howto setup the new XPlane plugin.

First of all, you have to install the XPlane plugin. To do that, start the Linuxtrack GUI, go to the fourth tab (Misc.) and press the "Install XPlane plugin...". A dialog will popup, use it to find the XPlane executable and press "Open". This will install the plugin.

Please note, that if you used linuxtrack XPlane plugin before, you will need to reinstall the plugin using the linuxtrack GUI to be able to use it in XPlane 10.

Setup of tracking parameters is done in the linuxtrack GUI, this page deals with setup of hotkeys and/or joystick buttons to control the tracking.

Last but not least - unlike other headtracking programs like freetrack, the GUI should not be running when using headtracking - e.g. when running XPlane with linuxtrack plugin installed.

# Joystick buttons setup #

Joystick buttons are setup using _Buttons : Adv_ pane in the _Joystick & Equipment_ window, that is accesible through _Settings / Joystick, Keys & Equipment_ menu.

![http://linux-track.googlecode.com/files/JoyButtonsSetup1.png](http://linux-track.googlecode.com/files/JoyButtonsSetup1.png)

Press the desired joystick button, and now check the checkbox next to the tabs (under the mouse pointer in the screenshot above). An Open dialog will appear.

![http://linux-track.googlecode.com/files/JoyButtonsSetup2.png](http://linux-track.googlecode.com/files/JoyButtonsSetup2.png)

Click the combobox in the upper part of the dialog and select **X System folder**.

![http://linux-track.googlecode.com/files/JoyButtonsSetup3.png](http://linux-track.googlecode.com/files/JoyButtonsSetup3.png)

Select the linuxtrack line in the lower part of the dialog.

![http://linux-track.googlecode.com/files/JoyButtonsSetup4.png](http://linux-track.googlecode.com/files/JoyButtonsSetup4.png)

Finally select the desired command (ltr\_run, ltr\_pause or ltr\_recenter).

![http://linux-track.googlecode.com/files/JoyButtonsSetup5.png](http://linux-track.googlecode.com/files/JoyButtonsSetup5.png)

Repeat those steps and when done, just close the window and the new bindings should be active.

# Keyboard bindings setup #

Keyboard bindings are setup pretty much in the same way - use _Keys_ pane in the _Joystick & Equipment_ window, that is accessible through _Settings / Joystick, Keys & Equipment_ menu.

![http://linux-track.googlecode.com/files/KeysSetup1.png](http://linux-track.googlecode.com/files/KeysSetup1.png)

When you create or select key binding, use the checkbox next to the tabs and continue the same way as above.

You can also press _Add new key assignment_ button in the middle bottom, that will create new key position - just press the newly created button and then press the key (or combination) that you want to assign to it. Then again select the desired binding.

There is also a possibility to assign keys using plugin Hot Key manager, but this method is deprecated since it doesn't seem to make this change persistent (after Xplane restart you have to do the assignment again).

# PilotView interface #

Since Linuxtrack 1.0 beta3 it is possible to use Linuxtrack's XPlane plugin in conjunction with PilotView (version 1.7).

To enable the PilotView interface, open the PilotView.ini file (located in Resources/plugins/PilotView directory in your XPlane installation folder) in your favorite text editor and add a line "EnableExternalData = 1" to the **CONFIG** section, so the result reads like this:
```
[CONFIG]
EnableExternalData = 1
AutoStart = 0
EngineVibration = 1
...
```

When done, start XPlane and start the tracking - now, when PilotView plugin is enabled, the tracking should work.

## Troubleshooting PilotView interface ##
Should you encounter problems, please follow these steps:
  1. Verify you have PilotView 1.7 (see the titlebar of its Config window)
  1. If you installed PilotView plugin before May 19th, please download the fresh version and try again (don't forget the required modification of the .ini file!)
  1. If you didn't reinstalled the Linuxtrack plugin, please do so in the ltr\_gui (Misc. pane)
  1. In the plugins menu inside XPlane, open Lnuxtrack and Linuxtrack Setup - there should be a line saying: "PilotView plugin found, chanelling headtracking data through it!".

Should you encounter any problems getting this to work, please contact me first, to see if the problem is not on my side.