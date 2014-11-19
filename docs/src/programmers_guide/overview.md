Overview    {#pie_noon_guide_overview}
========

## Motivation

Pie Noon is a cross-platform multi-player game written in C++.

It is a demonstration for several technologies, including the [FlatBuffers][]
serialization system, the [MathFu][] geometry library, and the [fplutil][]
utility library for Android C++ development. It also uses simple rendering,
audio, GUI, and animation subsystems.

Pie Noon demonstrates a quick and fun party game for the living room via support
for the [Nexus Player][], an [Android TV][] device.

## Subsystems

Pie Noon code is divided into the following subsystems
- [PieNoonGame][] -- holds game subsystems, state, and state machine
- [Audio][] -- a simple, game agnostic audio engine supporing buses and ducking
- [Engine][] -- a simple, game agnostic renderer, file loader, and input system
- [GUI][] -- a simple, game agnostic GUI
- [Impel][] -- simple animation of in game objects and GUI elements

Pie Noon is built on top of [SDL][], a low-level cross platform library.
[SDL][] abstracts input, file loading, threading, system events, logging, and
other systems from the underlying operating system.

## Source Layout

The following bullets describe the directory structure of the game.


| Path                          | Description                                  |
|-------------------------------|----------------------------------------------|
| `pie_noon` base directory     | Project build files and run script.          |
| `assets`                      | Assets loaded by the game.                   |
| `external`                    | OpenGL header required on Windows.           |
| `jni`                         | Top-level Android NDK makefile.              |
| `jni/SDL`                     | Android NDK makefile for [SDL][].            |
| `jni/SDL-mixer`               | Android NDK makefile for [SDL-mixer][].      |
| `jni/gpg`                     | Android NDK makefile for Play Games Services.|
| `jni/src`                     | Android NDK makefile for the game.           |
| `res`                         | Android specific resource files.             |
| `scripts`                     | Asset build makefiles.                       |
| `src`                         | Game's source.                               |
| `src/com/google/fpl/pie_noon` | SDL derived Android activity (entry point).  |
| `src/flatbufferschemas`       | Game schemas for [FlatBuffers][] data.       |
| `src/org/libsdl/app`          | Real entry point of the app on Android.      |
| `src/rawassets`               | JSON [FlatBuffers] used as game data.        |
| `tests`                       | Unit tests for some components of the game.  |


<br>

  [Android TV]: http://www.android.com/tv/
  [Audio]: @ref pie_noon_guide_audio
  [Engine]: @ref pie_noon_guide_engine
  [FlatBuffers]: http://google-opensource.blogspot.ca/2014/06/flatbuffers-memory-efficient.html
  [fplutil]: http://google.github.io/fplutil
  [GUI]: @ref pie_noon_guide_gui
  [Impel]: @ref pie_noon_guide_impel
  [MathFu]: http://googledevelopers.blogspot.ca/2014/11/geometry-math-library-for-c-game.html
  [Nexus Player]: http://www.google.com/nexus/player/
  [PieNoonGame]: @ref pie_noon_guide_pie_noon_game
  [SDL]: https://www.libsdl.org/
  [SDL-mixer]: http://www.libsdl.org/projects/SDL_mixer/
