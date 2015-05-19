Cardboard {#pie_noon_guide_cardboard}
===================

#  Introduction {#cardboard_introduction}

This document outlines how the [Cardboard SDK for Android][] is used in Pie
Noon, with specific regard to working with SDL.  For additional tips on
designing for Cardboard, be sure to check out
[Designing for Google Cardboard][].

# Overview {#cardboard_overview}

The Cardboard SDK provides various interfaces and classes to handle the
information that is needed to work with a Cardboard device, like head tracking
and detecting the magnet switch input.  As the Cardboard SDK is a Java library,
you can see these being used in FPLActivity.java.  Java Native Interface (JNI)
is used in order to interact with those objects from the C++ code base.

# Input {#cardboard_input}

Similar to other input devices, a [Controller][] is used to abstract input
from the Cardboard (cardboard_controller.h/.cpp).  In addition, the InputSystem
class (input.h/.cpp) has a **CardboardInput** object, which is captures events
sent from the Java SDK, and provides functions to interact with it from the
C++ code base.  You can reach this with the function **cardboard_input()**, on
the InputSystem class.

Head tracking is managed through the CardboardInput.  Every frame it updates
the two matrices it has for both eyes by calling, through JNI,
**getCurrentEyeParams()** on [CardboardView][].  These matrices can then be
reached with **left/right_eye_transform()**, on the CardboardInput object, and
are used when rendering for Cardboard, as they reflect the direction the device
is oriented towards.

Receiving input from the magnet switch is handled first in FPLActivity.java,
by the **MagnetSensor**, and the implemention of the
**OnCardboardTriggerListener** interface. When the device detects the magnet
switch, a call is passed from that to **CardboardInput**.  Then, to determine
if it was triggered in that frame, you can check the **triggered()** function.
This is used by the cardboard controller, which when a trigger is detected,
converts it to the correct logical inputs (Select Menu Option and Throw Pies).
Also in InputSystem, it checks for presses on the touchpad, which are also
translates as Cardboard triggers.  This is important, as not all Cardboard
devices use the magnet switch to handle input, and instead rely on touch.

# Rendering {#cardboard_rendering}

Rendering for Cardboard has to be handled, as the view needs to be duplicated
for the left and right eye.  Along with that, the Cardboard SDK provides the
function **undistortTexture()** in [CardboardView][], which undistorts the
given texture, adjusting the image to make it look correct for cardboard.

Because of how Cardboard works, the game needs to be rendered from two,
slightly different viewpoints, for the left and right eyes.  This is handled
in pie_noon_game.h/.cpp's **RenderForCardboard()**.  In that function, we
split the viewport in half, adjust the camera with the corresponding transform
from the **CardboardInput**, and render the scene for each view.

To use the undistort effects, some additional set up is required, which wraps
the split rendering.  The renderer class (renderer.h/.cpp) provides two
functions which are used, **BeginUndistortFramebuffer()** and
**FinishUndistortFramebuffer()**.  The first sets up a framebuffer that will be
rendered to, while that second takes that framebuffer modifies and renders it
to the device, with the Cardboard SDK.  These calls should wrap any rendering
that is meant to be seen with both eyes.

Additional rendering can be done outside of these calls to render on the
device as normal.  This can be seen with the **RenderCardboardCenteringBar()**
function, which displays a bar in the center of the screen, letting players
align the game correctly.

<br>

  [Cardboard SDK for Android]: https://developers.google.com/cardboard/android/
  [Designing for Google Cardboard]: http://www.google.com/design/spec-vr/designing-for-google-cardboard/a-new-dimension.html
  [CardboardView]: https://developers.google.com/cardboard/android/latest/reference/com/google/vrtoolkit/cardboard/CardboardView
  [Controller]: @ref pie_noon_guide_controllers

