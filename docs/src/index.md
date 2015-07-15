Pie Noon
========

Pie Noon    {#pie_noon_index}
========

[Pie Noon][] is a simple multiplayer party game where you can throw pies at
your friends. It is written in cross-platform C++.

\htmlonly
<iframe width="560" height="315"
  src="https://www.youtube.com/embed/p1F6HRFnLoo?rel=0"
  frameborder="0" allowfullscreen>
</iframe>
\endhtmlonly

## Motivation

[Pie Noon][] demonstrates a quick and fun party game for the living room
via support for the [Nexus Player][], an [Android TV][] device.  The game
shows how to use several open source technology components developed by Google.
<table style="border: 0; padding: 3em">
<tr>
  <td>
    <img src="info_panel_google_play_games.png"
     style="height: 20em" />
  [Google Play Games Services][] is used to share scores and reward players
  with achievements.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_webp_textures.png"
    style="height: 20em" />
   [WebP][], an image compression technology, is used to compress textures
   which reduces the size of the final game package and ultimately reduces
   download time.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_data_driven_flatbuffers.png"
     style="height: 20em" />
   [FlatBuffers][], a fast serialization system is used to store the
   game's data. The game configuration data is stored in [JSON][] files
   which are converted to FlatBuffer binary files using the flatc compiler.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_procedural_animation_mathfu.png"
     style="height: 20em" />
   [MathFu][], a geometry math library optimized for [ARM][] and [x86][]
   processors. The game uses [MathFu][] data types for two and three
   dimensional vectors, and for the 4x4 matrices used by the rendering
   system, and also by the [Motive][] animation system.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_multiple_control_schemes.png"
     style="height: 20em" />
   The game uses an [input device abstraction](@ref pie_noon_controllers)
   which enables it to work effectively with multiple control schemes
   including touch for [Android][], joypads on [Android TV][] and
   keyboards on desktop platforms.
  </td>
</tr>
<tr>
  <td>
    <img src="info_panel_normal_mapping.png"
     style="height: 20em" />
   The [Renderer](@ref pie_noon_guide_engine_renderer) is used
   conjunction with a normal mapping shader to provide cardboard rendering.
  </td>
</tr>
</table>
In addition, [fplutil][] is used to build, deploy, and run the game,
build and archive the game, and profile the game's CPU performance.

## Functionality

[Pie Noon][] is a cross-platform, open-source game that supports,

   * up to 4 players using Bluetooth controllers,
   * touch controls,
   * Google Play Games Services sign-in and leaderboards,
   * other Android devices
     * you can play on your phone or tablet in single-player mode,
     * or against human adversaries using Bluetooth controllers.
     * or in multi-screen mode with an Android TV and 1-4 Android phones/tablets
       (*new in v1.1!*)
     * or in a virtual reality mode with a Google Cardboard device (*new in
       v1.2!*)

## Supported Platforms

[Pie Noon][] has been tested on the following platforms:

   * [Nexus Player][], an [Android TV][] device
   * [Android][] phones and tablets
   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

We use [SDL] as our cross platform layer.
The game is written entirely in C++, with the exception of one Java file used
only on Android builds. The game can be compiled using [Linux][], [OS X][] or
[Windows][].

## Download

[Pie Noon][] can be downloaded from:
   * [GitHub][] (source)
   * [GitHub Releases Page](http://github.com/google/pienoon/releases) (source)
   * [Google Play](http://play.google.com/store/apps/details?id=com.google.fpl.pie_noon)
     (binary for Android)

**Important**: Pie Noon uses submodules to reference other components it
depends upon, so download the source from [GitHub][] using:

~~~{.sh}
    git clone --recursive https://github.com/google/pienoon.git
~~~

## Feedback and Reporting Bugs

   * Discuss Pie Noon with other developers and users on the
     [Pie Noon Google Group][].
   * File issues on the [Pie Noon Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of
     **pienoon**.

<br>

  [Android]: http://www.android.com
  [Android TV]: http://www.android.com/tv/
  [ARM]: http://en.wikipedia.org/wiki/ARM_architecture
  [Engine]: @ref pie_noon_guide_engine
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://android-developers.blogspot.ca/2014/11/utilities-for-cc-android-developers.html
  [JSON]: http://www.json.org/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Motive]: http://google.github.io/motive/
  [Nexus Player]: http://www.google.com/nexus/player/
  [OS X]: http://www.apple.com/osx/
  [Pie Noon]: @ref pie_noon_index
  [Pie Noon Google Group]: http://group.google.com/group/pienoon
  [Pie Noon Issues Tracker]: http://github.com/google/pienoon/issues
  [SDL]: https://www.libsdl.org/
  [stackoverflow.com]: http://www.stackoverflow.com
  [Windows]: http://windows.microsoft.com/
  [x86]: http://en.wikipedia.org/wiki/X86
  [Google Play Games Services]: https://developer.android.com/google/play-services/games.html
  [WebP]: https://developers.google.com/speed/webp
  [GitHub]: http://github.com/google/pienoon
