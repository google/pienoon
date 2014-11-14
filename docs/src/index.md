Pie Noon
========

Pie Noon    {#pie_noon_index}
========

[Pie Noon][] is a simple multiplayer party game where you can throw pies at your
friends.

[Pie Noon][] is a cross-platform, open-source game that supports,

   * up to 4 players using Bluetooth controllers,
   * touch controls,
   * Google Play Games Services sign-in and leaderboards,
   * other Android devices
     * you can play on your phone or tablet in single-player mode,
     * or against human adversaries using Bluetooth controllers).

[Pie Noon][] can be downloaded from [GitHub](http://github.com/google/pienoon)
or the [releases page](http://github.com/google/pienoon/releases).

**Important**: Pie Noon uses submodules to reference other components it depends
upon, so download the source using:

~~~{.sh}
    git clone --recursive https://github.com/google/pienoon.git
~~~

## Functionality

[Pie Noon][] uses several simple open source technology components developed by
Google.

   * [FlatBuffers][], a fast serialization system. The game configuration data
     is stored in [JSON][] files which are converted to FlatBuffer binary files
     using the flatc compiler.
   * [MathFu][], a geometry math library optimized for [ARM][] and [x86][]
     processors. The game uses Pie Noon data types for two and three dimensional
     vectors, and for the 4x4 matrices used by the rendering system.
   * [fplutil][], a collection of small utilities to aid native Android
     development. The game uses scripts in fplutil to build, deploy, and run
     the game, build and archive the game, and profile the game's CPU
     performance.

## Supported Platforms

[Pie Noon][] has been tested on the following platforms:

   * [Nexus Player][], an [Android TV][] device
   * [Android][] phones and tablets
   * [Linux][] (x86_64)
   * [OS X][]
   * [Windows][]

The game is written entirely in C++ with the exception of one Java file used
only on Android builds. The game can be compiled using [Linux][], [OS X][] or
[Windows][].

## Feedback and Reporting Bugs

   * Discuss Pie Noon with other developers and users on the
     [Pie Noon Google Group][].
   * File issues on the [Pie Noon Issues Tracker][].
   * Post your questions to [stackoverflow.com][] with a mention of **pienoon**.

  [Android]: http://www.android.com
  [Android TV]: http://www.android.com/tv/
  [ARM]: http://en.wikipedia.org/wiki/ARM_architecture
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://android-developers.blogspot.ca/2014/11/utilities-for-cc-android-developers.html
  [JSON]: http://www.json.org/
  [Linux]: http://en.m.wikipedia.org/wiki/Linux
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [OS X]: http://www.apple.com/osx/
  [Pie Noon]: @ref pie_noon_index
  [Pie Noon Google Group]: http://group.google.com/group/pienoon
  [Pie Noon Issues Tracker]: http://github.com/google/pienoon/issues
  [stackoverflow.com]: http://www.stackoverflow.com
  [Windows]: http://windows.microsoft.com/
  [x86]: http://en.wikipedia.org/wiki/X86
