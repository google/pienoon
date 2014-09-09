Splat Version 0.1.0

# Splat

Splat is a simple multiplayer party game where you can throw pies at your
friends.

## Gameplay

Players control cardboard cutout characters on sticks arranged roughly in a
circle as part of a cardboard diorama.

To win a match, each player must try to cover their opponents with as much
pie filling as possible.  When a player is covered with too much filling
they're knocked out of the match.  The last player in the match is the winner.

To cover others with pie filling, players need to throw and successfully land
pies on their opponents.  It sounds simple but pies are not all made equal,
the longer a player waits to throw a pie the larger it will grow resulting in
more pie filling covering an opponent.  Players aren't entirely defenseless,
it's possible to block incoming pies and deflect them towards other players.
While blocking an incoming pie is deflected in the player's aim direction.

## Controls

### Linux/OSX/Windows

| Action    | Player 1 Key | Player 2 Key | Player 3 Key | Player 4 Key |
|-----------|--------------|--------------|--------------|--------------|
| Aim Left  | a            | j            | cursor left  | numpad 4     |
| Aim Right | d            | l            | cursor right | numpad 6     |
| Throw     | w            | i            | cursor up    | numpad 8     |
| Block     | s            | k            | cursor down  | numpad 5     |

### Android

TODO


## Installing and running the game

### Linux

Unpack the Splat archive, for example from a terminal assuming
splat-linux.tar.gz is in the current directory:

     mkdir splat
     cd splat
     tar -xvf ../splat-linux.tar.gz

Run the game by changing into the splat directory and running the executable.
For example, from a terminal:

     cd splat
     ./run.sh

### OSX

Unpack the Splat archive, for example from a terminal assuming splat-osx.tar.gz
is in the current directory:

    mkdir splat
    cd splat
    tar -xvf ../splat-osx.tar.gz

Run the game by right clicking the run.command file and select
"Open with Terminal".  It's also possible to run the game from a terminal
using:

    cd splat
    ./run.sh

### Windows

Unpack the Splat archive in a directory and run the Splat executable under
the Debug or Release directory.  For example:

    cd splat
    Release\splat.exe

### Android

On your Android device open the "Settings" application, select "Security"
and enable the "Unknown sources" option.  It's possible to install the
application using [adb] on the command line or sending the apk to a location
that is accessible from the device.

#### Using Google Drive

*   Upload the apk to Google Drive from your desktop.
*   Open the apk in Google Drive on your Android to install the game.
*   Open "splat" from the application launcher.

#### Using e-mail

*   e-mail the apk to yourself as an attachment from your desktop.
*   Open the apk attachment on your Android device.
*   Open "splat" from the application launcher.

#### Using ADB from the Command Line

*   If it isn't installed already, install the [Android SDK].
*   Open a terminal window.
*   Change into the splat directory.
*   Install the apk to the device using adb install.
*   Open "splat" from the application launcher.

For example, from the terminal:

    cd splat
    adb install bin/splat-release.apk


## Building from source

Developers can build the game from source for Android, Linux, OSX and Windows.

### Building for Linux

#### Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building Splat for Linux:

-   OpenGL: libglapi-mesa 8.0.4 (tested with libglapi-mesa 8.0.4-0ubuntu0)
-   GLU: libglu1-mesa-dev 8.0.4 (tested with libglu1-mesa-dev 8.0.4.0ubuntu0)
-   cmake: 2.8.12 or newer

#### Before Building

Prior to building, install the following components using the [Linux][]
distribution's package manager:
-    [cmake][]. You can also manually install from [cmake.org]
     (http://cmake.org).
-    OpenGL (`libglapi-mesa`).
-    GLU (`libglu1-mesa-dev`).

For example, on Ubuntu:

    sudo apt-get install cmake
    sudo apt-get install libglapi-mesa
    sudo apt-get install libglu1-mesa-dev

The sample applications require OpenGL and GLU.

#### Building

-   Generate makefiles from the [cmake][] project in the `splat` directory.
-   Execute `make` to build the library and sample applications.

For example:

    cd splat
    cmake -G'Unix Makefiles'
    make

To perform a debug build:

    cd splat
    cmake -G'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug
    make

Build targets can be configured using options exposed in
`splat/CMakeLists.txt` by using cmake's `-D` option.
Build configuration set using the `-D` option is sticky across subsequent
builds.

For example, if a build is performed using:

    cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
    make

to switch to a release build CMAKE_BUILD_TYPE must be explicitly specified:

    cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
    make

#### Executing the game.

After building the project, you can execute the game from the command line.
For example:

    ./splat

### Building for OS X

You can use [cmake][] to generate an [Xcode][] project for Splat on [OS X][].

#### Version Requirements

These are the minimum required versions for building Splat on OS X:

-   OS X: Mavericks 10.9.1.
-   Xcode: 5.0.1
-   Xquartz: 2.7.5 (xorg-server 1.14.4)
-   cmake: 2.8.12 or newer

#### Before Building

-   Install [Xquartz][].
-   Reboot your machine.  Rebooting sets the `DISPLAY` environment variable for
    [Xquartz][], which enables sample applications to run correctly.

#### Creating the Xcode project using [cmake][]

When working directly with the source, you can generate the [Xcode][]
project using [cmake][].  For example, the following generates the Xcode
project in the splat directory.

    cd splat
    cmake -G "Xcode"

#### Building with [Xcode][]

-   Double-click on `splat/splat.xcodeproj` to open the project in [Xcode][].
-   Select "Product-->Build" from the menu.

#### Executing the game

-   Select "splat" in `Scheme`, for example "splat-->My Mac 64-bit", from the
    combo box to the right of the "Run" button.
-   Click the "Run" button.

### Building for Windows

You can use [cmake][] to generate a [Visual Studio][] project for Splat on
[Windows][].

#### Version Requirements

These are the minimum required versions for building Splat for Windows:

-   [Windows][]: 7
-   [Visual Studio][]: 2010 or 2012
-   [DirectX SDK][]: 9.29.1962 or newer.
-   [cmake][]: 2.8.12 or newer.
-   [GNU Make Windows][]: 3.81 or newer.

#### Creating the Visual Studio solution using [cmake][]

When working directly with the source, use [cmake][] to generate the
[Visual Studio][] solution and project files.  The DXSDK variable needs to
be set to point to the install location of the [DirectX SDK], for example
`c:\Program Files (x86)\Microsoft DirectX SDK (June 2010)`.

The following example generates the [Visual Studio][] 2012 solution in the
`splat` directory:

    cd splat
    cmake -G "Visual Studio 11"

To generate a [Visual Studio][] 2010 solution, use this command:

    cd splat
    cmake -G "Visual Studio 10"

Running [cmake][] under [cygwin][] requires empty TMP, TEMP, tmp and temp
variables.  To generate a [Visual Studio][] solution from a [cygwin][]
bash shell use:

    $ cd splat
    $ ( unset {temp,tmp,TEMP,TMP} ; cmake -G "Visual Studio 11" )

#### Building with [Visual Studio][]

-   Double-click on `splat/splat.sln` to open the solution.
-   Select "Build-->Build Solution" from the menu.

It's also possible to build from the command line using msbuild after using
vsvars32.bat to setup the [Visual Studio][] build environment.  For example,
assuming [Visual Studio][] is installed in
`c:\Program Files (x86)\Microsoft Visual Studio 11.0`.

    cd splat
    "c:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\vsvars32.bat"
    cmake -G "Visual Studio 11"
    msbuild splat.sln

#### Executing the game

-   Right-click on the Splat project in the Solution Explorer
    pane, and select "Set as StartUp Project".
-   Select "Debug-->Start Debugging" from the menu.

### Building for Android

#### Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building Splat for Android:

-   Android SDK:  Android 4.4 (API Level 19)
-   ADT: 20140702
-   NDK: android-ndk-r10
-   NDK plugn for Eclipse: Bundled with ADT

#### Before Building

-   Install the [Android SDK].
-   Install the [Android NDK].

#### Building

The Splat project has an `AndroidManifest.xml` file which contains details
about how to build an Android package (apk).

To build the project:

-   Open a command line window.
-   Go to the working directory containing the project to build.
-   Execute `build_apk_sdl.sh` to build.

For example:

    cd splat
    ./build_apk_sdl.sh DEPLOY=0 LAUNCH=0

#### Installing and running the game.

`build_apk_sdl.sh` will also install and run the game after a build is
complete.

For example, the following will install and run the game on a device attached
to the workstation with serial number `ADA123123`.

    cd splat
    ./build_apk_sdl.sh ADB_DEVICE=ADA123123

If only one device is attached to a workstation, the `ADB_DEVICE1 argument
can be ommitted.

#### Code Generation

By default, code is generated for devices that support the `armeabi-v7a` ABI.
Alternatively, you can generate a fat `.apk` that includes code for all ABIs.
To do so, override APP\_ABI on ndk-build's command line via `build_apk_sdl.sh`:

    ./build_apk_sdl.sh APP_ABI=all


## Source Layout

The following bullets describe the directory structure of the game.

*   `splat`: Project build files and run script.
*   `splat/assets`: Assets loaded by the game.
*   `splat/external/`: OpenGL extension header required by the project on
    Windows.
*   `splat/jni/`: Top-level Android NDK makefile.
*   `splat/jni/SDL`: Android NDK makefile for SDL.
*   `splat/jni/SDL-mixer`: Android NDK makefile for SDL-mixer.
*   `splat/jni/gpg`: Android NDK makefile for Google Play Games Services.
*   `splat/jni/src`: Android NDK makefile for the game.
*   `res`: Android specific resource files.
*   `scripts`: Asset build makefiles.
*   `src`: Game's source.
*   `src/com/google/fpl/splat`: SDL derived Android activity (entry point of
    the application on Android).
*   `src/flatbufferschemas`: Schemas for [FlatBuffers] used by the game data.
*   `src/org/libsdl/app`: Real entry point of the application on Android.
*   `src/rawassets`: JSON [FlatBuffers] used as game data.
*   `tests`: Unit tests for some components of the game.

### Overview

TODO

### Components

TODO

  [adb]: http://developer.android.com/tools/help/adb.html
  [ADT]: http://developer.android.com/tools/sdk/eclipse-adt.html
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [cmake]: http://www.cmake.org
  [Cygwin installation]: http://www.cygwin.com/
  [cygwin]: http://www.cygwin.com/
  [DirectX SDK]: http://www.microsoft.com/en-us/download/details.aspx?id=6812
  [GNU Make Windows]: http://gnuwin32.sourceforge.net/packages/make.htm
  [Linux]: http://en.wikipedia.org/wiki/Linux
  [managing avds]: http://developer.android.com/tools/devices/managing-avds.html
  [NDK Eclipse plugin]: http://developer.android.com/sdk/index.html
  [OS X]: http://www.apple.com/osx/
  [Ubuntu]: http://www.ubuntu.com
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
  [Xcode]: http://developer.apple.com/xcode/
  [Xquartz]: http://xquartz.macosforge.org/
  [FlatBuffers]: http://google.github.io/flatbuffers/
