Pie Noon   {#pie_noon_readme}
========

Pie Noon Version 1.1.0

[Pie Noon][] is a simple multiplayer party game where you can throw pies at
your friends.

## Motivation

[Pie Noon][] is a demonstration for several technologies, including the
[FlatBuffers][] serialization system, the [Motive][] animation system, the
[MathFu][] geometry math library, the [Pindrop][] audio engine, and the
[fplutil][] utility library for Android C++ development. It also uses simple
rendering and GUI subsystems (see [Engine][]).

Pie Noon is also an example of a quick and fun party game for the living room
via support for the [Nexus Player][], an [Android TV][] device.

## Downloading

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

## Documentation

See our online documentation for how to [Build and Run Pie Noon][], and for a
[Programmer's Guide][] that details the overall structure of the game and all
of it's subsystems.

To contribute the this project see [CONTRIBUTING][].

For applications on Google Play that are derived from this application, usage
is tracked.
This tracking is done automatically using the embedded version string
(kVersion), and helps us continue to optimize it. Aside from
consuming a few extra bytes in your application binary, it shouldn't affect
your application at all. We use this information to let us know if Pie Noon
is useful and if we should continue to invest in it. Since this is open
source, you are free to remove the version string but we would appreciate if
you would leave it in.


  [Android TV]: http://www.android.com/tv/
  [Pindrop]: http://google.github.io/pindrop/
  [Build and Run Pie Noon]: http://google.github.io/pienoon/pie_noon_guide_building.html
  [Engine]: http://google.github.io/pienoon/pie_noon_guide_engine.html
  [FlatBuffers]: http://google.github.io/flatbuffers/
  [fplutil]: http://android-developers.blogspot.ca/2014/11/utilities-for-cc-android-developers.html
  [Motive]: http://google.github.io/motive/
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [Pie Noon]: http://google.github.io/pienoon/
  [Programmer's Guide]: http://google.github.io/pienoon/pie_noon_guide_overview.html
  [CONTRIBUTING]: http://github.com/google/pienoon/blob/master/CONTRIBUTING
  [GitHub]: http://github.com/google/pienoon
