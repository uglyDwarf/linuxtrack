# Introduction #

This page provides sommary of the steps needed to setup linuxtrack.


# Details #

## Setup tracking device ##
Select **Device setup** tab (if not already selected) and in **Tracking device** combo select the tracking device you intend to use.

### Webcam setup ###
If the webcam you want to use was not attached to the system when linuxtrack GUI started, attach it and press **Refresh** button. The webcam should appear in the **Tracking device** combo.

The next thing to do is to select the pixel format(only formats supported by the device are shown!). Please note, that for the moment only YUYV format is supported by the tracker!

Then select desired resolution and framerate - the higher resolution and framerate the better, but higher CPU usage too...

The threshold should be setup according to the **Camera view** in the **Tracking window**.
Start the tracking by pressing the **Start** button in the **Tracking window** and make sure that camera sees only lights from your model...
If there are more lights, try to remove them by setting higher threshold (or remove them physically). On the other hand, if the lights disappear when you turn your head, try lowering the threshold.

**Min. blob** and **Max. blob** spinboxes are specifying lower and upper limits for number of pixels in the blob (a light that camera sees). This way you can ignore lights that are too small or too big, although it is always better to get rid of all unwanted light sources that can confise the tracking.

### Wiimote setup ###
If you select wiimote as your tracking device, you can specify which LEDs should be turned on in which mode - this can be useful when you are unsure if tracker is running or paused. On the other hand, to maximize life of batteries in the wiimote, it is better to have those LEDs turned of all the time.

### TrackIR setup ###
To be able to use TrackIR device, you have to download its firmware first - please read instructions concerning linuxtrack installation if you do not have it installed already.
If the firmware is in the correct place, you'll see "Firmware found!" message there.

The next thing is setup of **threshold**, **min. blob** and **max. blob** - please refer to the webcam setup above.

Then if you use TrackIR 5, you can set Status LED brightness and IR LEDs brightness.

Checkbox **Signalize status** allows you to turn on/off signalization of the tracker state (running/paused/stopped).

## Model setup ##
**Model Setup** tab allows you to set which model you intend to use. You can either choose one of predefined models or you can create your own model. You can also tweak dimensions of your model.

When creating new model, press button **Create New** and start by filling in you model's name. Then select what type of model you want to create - three point cap, three point clip or single point.
Finally measure dimensions of your model and select whether it is active (uses LEDs) or passive (reflective). Hx, Hy and Hz values can be approximate, you can tweak them later.

## Tracking setup ##
**Tracking setup** tab manages profiles and tracking parameters.
You can set which profile you want to edit or you can create new one.

In the sensitivities part of this tab you can disable each axis and also set sensitivities for them. If this is not enough, press **Edit sensitivity curves** button...

In the newly opened dialogue user can set response for each axis in a more detailed way.
**Input Limits** sets maximum angle/movement of your real head to get maximal response.
You can select how curved the sensitivity curve should be by moving the **Curvature** slider; **Mult. Factor** sets how much your simulated can turn/move.
**Dead Zone** slider can make the axis insensitive around the center.
Checkbox **Symetrical** does exactly what its name says...

Last thing to set in the **Tracking setup** tab is the **Filter Factor** slider. By moving this slider right, tracking will be smoother, but a little bit laggier. Moving it to the left makes tracking less laggy, but possibly more jumpy.

## Tweaking H values ##
The Hx, Hy and Hz values you saw in the **Model Setup** tab are needed to minimize simulated head movement while turning your real head.

### Three point Cap setup tweaking ###
When using three point Cap, you have to set only Hy and Hz.
Lets start by Setting Hz. Start tracking, go to the **Tracking setup** and disable all axes except Move Left/Right... Look to the center of the screen, press **Recenter** button and now try to look left and right. If the picture moves left or right, Hz is not correct.
If the picture moves left, when you look right, Hz is too high and vice versa.
Now tweak the Hz value until the movement is almost unnoticeable.

Now to the Hy...
Disable all axes but Move back/forth and try to look up and down again...
If the simulated head moves forth while you look up, the Hy is too high and vice versa.
Again tweak the value to get the smallest movement possible.

### Three pont Clip tweaking ###
With clip model you have to select all three offsets.
Hy and Hz are set up the same way

To setup Hx correctly do the following:
disable all axes except Move back/forth and turn your head left/right. If your simulated head moves forth, when you look to the side where your clip is, then Hx is too high.

**Now you can turn all axes on and enjoy the linuxtrack!**