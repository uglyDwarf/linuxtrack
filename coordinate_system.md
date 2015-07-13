# Introduction #

This page describes the internal coordinate system used in linux-track.
This is provided so that all developers are following a consistent coordinate model.
The coordinate system used is intended to be consistent with the Alter92 coordinate system.

The linux-track pose estimator has the following inputs and outputs:
  * **Reflector model** (input)
  * **Approximate distance bwtween model and tracker** (input)
  * **Reference Frame** (input)
  * **Nth Frame** (input)
  * **Transformation** (output)

The sections below describe
  1. the X-Y-Z coordinate system common to the reflector model and transformation output,
  1. each input/output a bit in detail.
  1. A word on "Euler" angles.

# Common X-Y-Z Definition #

When facing the camera, the +X vector is "right."

When facing the camera, the +Y vector is "up."

The +Z vector fires out of the camera.

The all X,Y,Z measurements use units of millimeters.

![http://linux-track.googlecode.com/svn/wiki/XYZDiagram.png](http://linux-track.googlecode.com/svn/wiki/XYZDiagram.png)

# Input Reflector Model #

The pose estimation algorithm requires the "common" X-Y-Z coordinates of the three points that make up the reflector (or active LEDs).
The algorithm also requires a reasonable estimate of the user's head-center with respect to the reflector model origin.
All sizes/distances are in millimeters.

Points must be defined in the common X-Y-Z coordiates with respect to the camera, in a neutral facing.

Point Definitions:
  * Point 0: The topmost (greatest +Y) point shall be defined as the origin of the reflector model, and point 0
  * Point 1: The leftmost (least +X) point shall be defined as point 1
  * Point 2: The remaining reflector point shall be point 2
  * headpoint: the reasonable estiimate of the user's head center with respect to point 0

![http://linux-track.googlecode.com/svn/wiki/XYZReflectorDiagram.png](http://linux-track.googlecode.com/svn/wiki/XYZReflectorDiagram.png)

Because point 0 is by definition 0,0,0, it is not an explicit  part of the to the `struct reflector_model_type`.

# Camera - model distance #
The Alter92 algorithm uses weak-perspective, so no Z-translations are directly estimatable.
However, by providing estimated distance between model and the tracker, Alter92 scaling factor may be used with this distance to estimate Z-translations.

# Input Frames #

Raw images taken from the camera are distilled to reflector/LED "blobs."
A frame input to the pose estimator is defined as three blobs.
Blobs are represented as X,Y vectors in image-space.
The blob vector points to the center of the blob in image-space.
Image-space units are in camera pixels.

In image-space:
  * the origin is in the center of the image.
  * the bottom left corner of the image is (-pixel\_width/2, -pixel\_height/2).
  * the top right corner of the image is (+pixel\_width/2, +pixel\_height/2).

Image space X-Y must track camera space X-Y.
In other words, moving a reflector to the right in space (increasing X), should see a corresponding increase in positive X of the blob center.
This means that for most cameras, the blob centers must be mirrored horizontally before being passed to the pose estimation algorithm.

Note that the blob input to the pose estimator are pixel-area sorted (biggest area is index0).  The pose estimator is responsible for resorting these as needed.

## Input Reference Frame ##
We use reference frame as a center point and we judge rotations and translations computed from Nth frame.

## Input Nth Frame ##
We get new frame from the image sensor, we compute new model postion. By referencing reference trame we compute angles and translations we need.

# Output Transformation #
The linux-track end-output is a transformation.
This transformation is made up of a translation vector and rotation matrix.

## Translation Vector ##
The translation vector describes the user's nthframe head movement relative to the reference frame.
This is represented as a 3x1 vector.

## Rotation Matrix ##
The rotation matrix describes the user's nthframe head rotations relative to the reference frame.
Internally, linux-track represents rotations as a 3x3 matrix.

# A word on Euler Angles (Yaw, Pitch, Roll) #

To be defined.

_picture to be included_