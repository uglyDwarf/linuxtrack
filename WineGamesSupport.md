# Introduction #

There is a number of TrackIR enabled games that run just fine on Linux/Mac using Wine, so Linuxtrack added support for those games too. This page will guide you through the installation, setup, usage and troubleshooting of the linuxtrack-wine.

# Linux installation #
To install the linuxtrack-wine, just open up the ltr\_gui, go to the "Misc." pane (fourth) and press the "Install Wine plugin..." button.

You'll be presented with a dialog, allowing you to choose a wine prefix, where to install the linuxtrack-wine. Please note, that it has to be installed into each prefix, that contain a game you intend to use with linuxtrack. And if you have no idea what I'm talking about, just select the .wine directory inside your home dir and press open. The installer will pop up - just press next few times and you are done.

# MacOSX installation #
To install the linuxtrack-wine, install the linuxtrack-wine.exe file into each of Wine bottles (or prefixes you have). The installer will pop up - just press next few times and you are done.

# Setup #
First of all, be sure to run ltr\_gui and setup the headtracking there - choose which device to use, which model and so on. When done, you can close ltr\_gui and run the Controller application, that was installed as a part of linuxtrack-wine. A simple window will pop up, showing which keys (if any) are bound to tracking pause and recentering. When you run it for the first time, you should choose two keys for those task, so you can effectively control the thing. This window should stay open or minimized to tray to allow the control.

# Usage #
When you run the game for the first time, a profile for this game is created in linuxtrack. So when you need to adjust the tracking, be sure you select the right profile! Then just run the game and enjoy the ride!

Also make sure, that you run Controler from the same prefix/bottle, otherwise it is not going to work.


# Supported games #
Technically, any TrackIR enabled game, that runs in Wine should "just work" (TM) - if not, feel free to file a bug...

**Il-2**

**Condor soaring simulator**

**Falcon4 Allied Force**

If some other game works, please leave a comment and I'll add it here.