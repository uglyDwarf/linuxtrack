# Introduction #

Linuxtrack saves its preferences in file '.linuxtrack', that is located in your home directory.

Here is an example of this file:
```
#Global section should contain global settings (eg. Capture device type)
[Global]
Input = TrackIR4
#Input = Webcamera
#Input = Wii
Model = Cap
#Comment previous and uncomment this if you use Clip...
#Note that clip disables the TIR4 IR Illuminator LEDS
#Model = Clip


[Cap]
Model-type = Cap
Cap-X = 70
Cap-Y = 50
Cap-Z = 92.5
Head-Y = 100
Head-Z = 90

[Clip]
Model-type = Clip
Clip-Y1 = 40
Clip-Y2 = 110
Clip-Z1 = 30
Clip-Z2 = 50
#Change sign of Head-X, if your clip is on the right side
Head-X = -100
Head-Y = -100
Head-Z = 50

[TrackIR4]
Capture-device = Tir4
Capture-device-id = Track IR 4

[Webcamera]
Capture-device = Webcam
Capture-device-id = Live! Cam Optia
Resolution = 640 x 480
Pixel-format = YUYV
Fps = 30/1
Threshold = 150
Max-blob = 200
Min-blob = 10

[Wii]
Capture-device = Wiimote
Capture-device-id = Wiimote

[Default]
Filter-factor = 5
Pitch-multiplier = 3.0
Yaw-multiplier = 3.0
Roll-multiplier = 3.0
Xtranslation-multiplier = 3.0
Ytranslation-multiplier = 3.0
Ztranslation-multiplier = 3.0

[XPlane]
Freeze-button = 3
Recenter-button = 4

```
# Details #

# Global section #
This section should always be in the file and it should be the first one (the file is more clear if it is first)...
It should contain 2 entries:
  * **Input = ...** - name of section containing input device specification.
  * **Model = ...** - name of section containing used model type/dimmensions.

# Device info #
This section tracking device info.
For the moment three devices are supported: TrackIR4, Wiimote and Webcam (V4L2).
This section contains following items:
  * **Capture-device = ...** - type of device. Valid options are 'TIR4', 'Wiimote' and 'Webcam'.
  * **Capture-device-id = ...** - device id string (currently used only for Webcams to allow user to have several webcms attached at once - eg. laptop with built-in crappy one and better one for tracking).

The rest of options is to be used just by webcam interface:
  * **Resolution = ... x ...** - resolution to be used
  * **Pixel-format = YUYV** - right now the only supported format
  * **Fps = ../1** - frames per second - make sure webcam can do what you ask from it!
  * **Threshold = ...** value from 0 to 255 used to threshold image to get just three bright spots on dark background.
  * **Max-blob = ...** If number of pixels in the bright spot goes over this number, the spot is considered invalid.
  * **Min-blob = ...** Bright spot with less than this number of pixels is not valid (if the sensor if noisy, this should help).

# Model info #
This section describes model that tracker tracks...
Highest model point is the reference.
First item specifies type of model (all values are in millimeters):
  * **Model-type = ...** Type of model. Valid options are 'Cap' and 'Clip'.
If type is Cap, following items are present:
  * **Cap-X = ...** Distance between left and right LED/reflector.
  * **Cap-Y = ...** Vertical distance of line between left and right LED, and upper LED
  * **Cap-Z = ...** Horizontal distance of line between left and right LED, and upper LED
  * **Head-Y = ...** Vertical distance between upper LED and point around which your head pitches/rolls.
  * **Head-Z = ...** Horizontal distance between upper LED and point around which your head pitches/rolls.

If the model is Clip, following items are present:
  * **Clip-Y1 = ...** Vertical distance between upper and middle LED
  * **Clip-Y2 = ...** Vertical distance between upper and lower LED
  * **Clip-Z1 = ...** Horizontal distance between upper and middle LED
  * **Clip-Z2 = ...** Horizontal distance between upper and lower LED
  * **Head-X = ...** Horizontal distance (left/right) between center of your head and clip (positive value, if your clip is on the right side of your head, negative otherwise)
  * **Head-Y = ...** Vertical distance between upper LED and point around which your head pitches/rolls.
  * **Head-Z = ...** Horizontal distance (forward/backward) between upper LED and point around which your head pitches/rolls.

**There is one difference there - All cap values are positive, while Head-... values for Clip can be both positive and negative!**

# Default section #
This section contains info on tracking - sensitivities and filter factor.
Values specified here can be overriden by plugin/program specific section (more on that later).

# Plugin/program specific sections #
.linuxtrack file can contain any number of sections that are used to store data for plugins/programs using linuxtrack library.