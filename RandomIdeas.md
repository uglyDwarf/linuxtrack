# Introduction #

We could use this to brain-storm a bit on how to do it right...

# Details #

There are just few random ideas on linux track:
  * It would be a library (not sure if dynamic would be better than static, so for the start I'd go for static one) against which you'd link your program - XPlane in our case, but FlightGear or any other game/program could use it then.
  * XPlane SDK allows to create some GUI I guess, but for a start we would use that Python/GTK for rapid prototyping... If it wouldn't be sufficient for some reason (speed or anything else), then we'd go for something else...
  * For the TIR driver I'd use C, so it can be incorporated to the library itself... Also the speed is a factor there as you said. If you check in sources of your driver, I'll help you to convert them to C.
  * In the mean time, I'll be working on cleaning up/commenting of the sources needed for pose computation and the support stuff, so we can put together a prototype and try it out.
  * Given the fact, that I don't have TIR myself, I'm going to use my webcam instead. But the main part will be the same for both of us, so hopefully this setup will work well.

## Tulth's response ##

### Concept of Operation ###
I totally agree we need to brainstorm some.  I am trying to figure out what it should look like from the toplevel before we go crazy coding.

Here is what I would consider my ideal operation:

I'd install the app, then configure it to run at X login.  It would need to be run sudo/gksudo since usb access is restricted to root.

Then, the linux-track would run as a daemon, and show up as an icon in the gnome system tray.

The linux-track would provide some interface, such as a socket for plug-ins to connect to.

There would be a seperate x-plane plug in that would connect to this socket.  It would read from the socket translation vectors and rotation matrices.

If the user clicked on the system tray icon, it would pop up a configuration window.

The configuration window would allow the following user inputs:
  * A quit option.  I think this should also appear on the tray icon context menu.
  * A disable option.  I think this should also appear on the tray icon context menu.
  * select input device (webcam or tir4 or whatever)
  * Input device-webcam specific parameters: for example, a webcam would need resolution info, and threshold info.
  * reflector/led model points, or select a predefined model
  * scaling factors of some sort for each axis/rotation
  * selecting the reset coordinate system reference keypress

The configuration window would give the following diagnostic outputs:
  * A red/green light on whether its tracking.  I think this should also appear on the tray icon.  Question: what should it do if it disconnects?
  * A thresholded real-time video image that also identifies trackable blob centers.  This is so a user can tell things are working right, and identify problems (like a mirror or the sun in the background).
  * A set of six horizontal bar graphs showing the pitch, yaw, roll, x,y,z in realtime.  It should show both the raw input, and the scaled input.  For example, the user could turn his head left to right and see the yaw bar go from far left to yaw right.
  * A 3d output showing the raw head, and scaled head.

While in the configuration window or in the tray:
  * The linux-track would intercept the user defined keypress for reset coordinate system reference, and update.  Ideally this could be a keypress and/or joystick button.

### Implementation notes so far ###
I have uploaded my python code to trunk/python\_prototype.  The main driver file is tir4.py.  You can take a crack at c-ifying it if you like.  If not I will do so soon.

I am still getting a handle on the image to rotation/translation conversion.  I understand up to getting the M model points in the camera-centered coord system.  I'm still working though getting from the camera-centered coord system to the reference-point coordinate system.

My simulations do show weak perspective being an excellent approximation of full-perspective.  If you are interested, I can upload the maxima code I'm working with.

I think the existing algorithm is missing a 4th point on the reflector/LED models that is the center of the user head.  Rotation/translations should be with respect to the user's head, not some point reflector/LED.  The point reflector/led as a reference will be ok if its very close to the center of the user's head, but I think they may be too far away in reality.  I'm still mucking with that.

I'm not yet sure if the configuration app will actually be the driver app or if its a separate program that talks to the driver app.  If seperate, perhaps there could be two sockets, one for just tracking data, then a second for configuration of the background daemon.

I'm not sure how to it is to intercept X keypresses from an app without focus.  Mumble (linux-like ventrilo) does it though, so I think we could look at what we do.