# Introduction #

Currently we are supporting three types of input devices.
  * Track IR 2, 3, 4, 5 and SmartNav 4
  * Wiimote
  * Webcam

If you own other version of Track IR or other suitable device and you'd like to contribute to out project, you're welcome.

# Track IR 2, 3, 4, 5 and SmartNav 3, 4 #
TrackIR 4 is supported from the start of our project. Tulthix managed to understand the communication protocol and he wrote a driver for it using libusb (fully userspace).
I added support for the rest with help from Ola (TrackIR 3), Jaša (TrackIR 2), Bill (SmartNav 4) and Pierre (SmartNav 3).

Pros of this device are given by the fact that it is designed for motion tracking:
  * wide field of view (TrackIR 4, 5 and SmartNav 4)
  * 120Hz framerate
  * if you are using cap reflectors, you don't need any wires/batteries
  * uses infra red
  * ease of use (almost plug and play)
  * low cpu usage

Cons:
  * infra red diodes and the FPGA in the tracker can become quite hot (lowers lifetime)
  * price - it is just high

# Wiimote #
Initial support for this device under Linux was contributed by Mr. Big.

Pros:
  * wireless bluetooth connection
  * ease of use (plug and play)
  * price
  * low cpu usage

Cons:
  * uses batteries
  * reportedly bluetooth can interfere with your wifi
  * Harder troubleshooting (it tracks only up to 4 IR sources).

# Webcam #
Support for webcams in Linux got much better over the years; that said, be carefull with the cheapest webcams - your best bet is UVC compatible webcam. Preferably use a webcam, which allows you to turn off the automatic exposition, in order to get consistent high framerate.

If you intend to use IR LEDs model, be aware that some webcams have quite strong IR filter in a form of a lens coating; it might be quite challenging to get it off and you risk damaging the camera...

The biggest benefit you can get by using webcam is higher picture resolution.
So my recommendation is following:
  * if you have a webcam and it works for you, try it out...
  * if you don't have webcam and you want to give it a shot, buy some, but be sure at least to consult some hardware compatibility lists first...

Pros:
  * webcams can be cheapest
  * higher physical image resolutions

Cons:
  * not really guaranteed to work well, if at all
  * needs manual gain/brightness/gamma setting...
  * lower framerate
  * for some webcams you can't turn automatic exposure off resulting in low/instable framerate


_I'm speaking from my own experience - I had 6 different webcams of which four worked relatively well; If someone is interested, the ones working for me is Creative Live! Cam Optia (non AF model) and Logitech C270, and two notebook webcams (couldn't turn off autoexposure with all the consequences)._