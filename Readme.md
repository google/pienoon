PieNoon Version 0.1.0

# PieNoon

PieNoon is a simple multiplayer party game where you can throw pies at your
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

### Linux/OS X/Windows

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

Unpack the PieNoon archive, for example from a terminal assuming
pie_noon-linux.tar.gz is in the current directory:

     mkdir pie_noon
     cd pie_noon
     tar -xvf ../pie_noon-linux.tar.gz

Run the game by changing into the pie_noon directory and running the executable.
For example, from a terminal:

     cd pie_noon
     ./run.sh

### OS X

Unpack the PieNoon archive, for example from a terminal
assuming pie_noon-osx.tar.gz is in the current directory:

    mkdir pie_noon
    cd pie_noon
    tar -xvf ../pie_noon-osx.tar.gz

Run the game by right clicking the run.command file and select
"Open with Terminal".  It's also possible to run the game from a terminal
using:

    cd pie_noon
    ./run.sh

### Windows

Unpack the PieNoon archive in a directory and run the PieNoon executable under
the Debug or Release directory.  For example:

    cd pie_noon
    Release\pie_noon.exe

### Android

On your Android device open the "Settings" application, select "Security"
and enable the "Unknown sources" option.  It's possible to install the
application using [adb] on the command line or sending the apk to a location
that is accessible from the device.

#### Using Google Drive

*   Upload the apk to Google Drive from your desktop.
*   Open the apk in Google Drive on your Android to install the game.
*   Open "pie_noon" from the application launcher.

#### Using e-mail

*   e-mail the apk to yourself as an attachment from your desktop.
*   Open the apk attachment on your Android device.
*   Open "pie_noon" from the application launcher.

#### Using ADB from the Command Line

*   If it isn't installed already, install the [Android SDK].
*   Open a terminal window.
*   Change into the pie_noon directory.
*   Install the apk to the device using adb install.
*   Open "pie_noon" from the application launcher.

For example, from the terminal:

    cd pie_noon
    adb install bin/pie_noon-release.apk


## Building from source

Developers can build the game from source for Android, Linux, OS X and Windows.

### Building for Linux

#### Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building PieNoon for Linux:

-   OpenGL: libglapi-mesa 8.0.4 (tested with libglapi-mesa 8.0.4-0ubuntu0)
-   GLU: libglu1-mesa-dev 8.0.4 (tested with libglu1-mesa-dev 8.0.4.0ubuntu0)
-   cmake: 2.8.12 or newer
-   automake: 1.141 or newer
-   autoconf: 2.69 or newer
-   libtool: 2.4.2 or newer

#### Before Building

Prior to building, install the following components using the [Linux][]
distribution's package manager:
-    [cmake][]. You can also manually install from [cmake.org]
     (http://cmake.org).
-    OpenGL (`libglapi-mesa`).
-    GLU (`libglu1-mesa-dev`).
-    autoconf, automake, and libtool

For example, on Ubuntu:

    sudo apt-get install cmake
    sudo apt-get install libglapi-mesa
    sudo apt-get install libglu1-mesa-dev
    sudo apt-get install autoconf automake libtool

The sample applications require OpenGL and GLU.

#### Building

-   Generate makefiles from the [cmake][] project in the `pie_noon` directory.
-   Execute `make` to build the library and sample applications.

For example:

    cd pie_noon
    cmake -G'Unix Makefiles'
    make

To perform a debug build:

    cd pie_noon
    cmake -G'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug
    make

Build targets can be configured using options exposed in
`pie_noon/CMakeLists.txt` by using cmake's `-D` option.
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

    ./pie_noon

### Building for OS X

You can use [cmake][] to generate an [Xcode][] project for PieNoon on [OS X][].

#### Version Requirements

These are the minimum required versions for building PieNoon on OS X:

-   OS X: Mavericks 10.9.1.
-   Xcode: 5.1.1 or newer
-   Xquartz: 2.7.5 (xorg-server 1.14.4)
-   cmake: 2.8.12 or newer

#### Before Building

-   Install [Xquartz][].
-   Reboot your machine.  Rebooting sets the `DISPLAY` environment variable for
    [Xquartz][], which enables sample applications to run correctly.

#### Creating the Xcode project using [cmake][]

When working directly with the source, you can generate the [Xcode][]
project using [cmake][].  For example, the following generates the Xcode
project in the pie_noon directory.

    cd pie_noon
    cmake -G "Xcode"

#### Building with [Xcode][]

-   Double-click on `pie_noon/pie_noon.xcodeproj` to open the project in
    [Xcode][].
-   Select "Product-->Build" from the menu.

You can also build the game from the command-line.

-   Run `xcodebuild` after generating the Xcode project to build all targets.
-   You may need to force the `generated_includes` target to be built first.

For example, in the pie_noon directory:

    xcodebuild -target generated_includes
    xcodebuild

#### Executing the game

-   Select "pie_noon" in `Scheme`, for example "pie_noon-->My Mac 64-bit", from
    the combo box to the right of the "Run" button.
-   Click the "Run" button.

### Building for Windows

You can use [cmake][] to generate a [Visual Studio][] project for PieNoon on
[Windows][].

#### Version Requirements

These are the minimum required versions for building PieNoon for Windows:

-   [Windows][]: 7
-   [Visual Studio][]: 2010 or 2012
-   [DirectX SDK][]: 9.29.1962 or newer.
-   [cmake][]: 2.8.12 or newer.
-   [Python][]: 2.7.*

#### Creating the Visual Studio solution using [cmake][]

When working directly with the source, use [cmake][] to generate the
[Visual Studio][] solution and project files.  The DXSDK_DIR variable needs to
be set to point to the install location of the [DirectX SDK], for example
`c:\Program Files (x86)\Microsoft DirectX SDK (June 2010)`.

The following example generates the [Visual Studio][] 2012 solution in the
`pie_noon` directory:

    cd pie_noon
    cmake -G "Visual Studio 11"

To generate a [Visual Studio][] 2010 solution, use this command:

    cd pie_noon
    cmake -G "Visual Studio 10"

Running [cmake][] under [cygwin][] requires empty TMP, TEMP, tmp and temp
variables.  To generate a [Visual Studio][] solution from a [cygwin][]
bash shell use:

    $ cd pie_noon
    $ ( unset {temp,tmp,TEMP,TMP} ; cmake -G "Visual Studio 11" )

#### Building with [Visual Studio][]

-   Double-click on `pie_noon/pie_noon.sln` to open the solution.
-   Select "Build-->Build Solution" from the menu.

It's also possible to build from the command line using msbuild after using
vsvars32.bat to setup the [Visual Studio][] build environment.  For example,
assuming [Visual Studio][] is installed in
`c:\Program Files (x86)\Microsoft Visual Studio 11.0`.

    cd pie_noon
    "c:\Program Files (x86)\Microsoft Visual Studio 11.0\Common7\Tools\vsvars32.bat"
    cmake -G "Visual Studio 11"
    msbuild pie_noon.sln

#### Executing the game

-   Right-click on the PieNoon project in the Solution Explorer
    pane, and select "Set as StartUp Project".
-   Select "Debug-->Start Debugging" from the menu.

### Building for Android

#### Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building PieNoon for Android:

-   Android SDK:  Android 4.4 (API Level 19)
-   ADT: 20140702
-   NDK: android-ndk-r10
-   NDK plugn for Eclipse: Bundled with ADT

#### Before Building

-   Install the [Android SDK].
-   Install the [Android NDK].

#### Building

The PieNoon project has an `AndroidManifest.xml` file which contains details
about how to build an Android package (apk).

To build the project:

-   Open a command line window.
-   Go to the working directory containing the project to build.
-   Execute `build_apk_sdl.sh` to build.

For example:

    cd pie_noon
    ./build_apk_sdl.sh DEPLOY=0 LAUNCH=0

#### Installing and running the game.

`build_apk_sdl.sh` will also install and run the game after a build is
complete.

For example, the following will install and run the game on a device attached
to the workstation with serial number `ADA123123`.

    cd pie_noon
    ./build_apk_sdl.sh ADB_DEVICE=ADA123123

If only one device is attached to a workstation, the `ADB_DEVICE1 argument
can be ommitted.

#### Code Generation

By default, code is generated for devices that support the `armeabi-v7a` ABI.
Alternatively, you can generate a fat `.apk` that includes code for all ABIs.
To do so, override APP\_ABI on ndk-build's command line via `build_apk_sdl.sh`:

    ./build_apk_sdl.sh APP_ABI=all

## Modifying Assets

### Overview

The game has many data driven components allowing designers the tweak the
game mechanics and content creators to add assets.

Game data is specified by [JSON][] files in `pie_noon/src/rawassets` which are
converted to binary and read in-game using [Flatbuffers][].  Each [JSON][]
file in `pie_noon/src/rawassets` is described by a [Flatbuffers schema][] in
`pie_noon/src/flatbufferschemas`.  Each [Flatbuffers schema][] file contains
descriptions of each field (prefixed by `//`) in associated [JSON][] files.

The following table maps each set of [JSON][] files in `src/rawassets` to
[Flatbuffers][] schemas in `src/flatbufferschemas`:

| JSON File(s)                       | Flatbuffers Schema                |
|------------------------------------|-----------------------------------|
| `buses.json`                       | `buses.fbs`                       |
| `character_state_machine_def.json` | `character_state_machine_def.fbs` |
| `config.json`                      | `config.fbs`                      |
| `materials/*.json`                 | `materials.fbs`                   |
| `rendering_assets.json`            | `rendering_assets.fbs`            |
| `sound_assets.json`                | `sound_assets.fbs`                |
| `sounds/*.json`                    | `sound.fbs`                       |

### Building

The game loads assets from the `assets` directory.  In order to convert data
into a format suitable for the game runtime, assets are built from a python
script which requires:

*   [Python][] to run the asset build script. [Windows][] users may need to
    install [Python][] and add the location of the python executable to the PATH
    variable.  For example in Windows 7:
    *   Right click on `My Computer`, select `Properties`.
    *   Select `Advanced system settings`.
    *   Click `Environment Variables`.
    *   Find and select `PATH`, click `Edit...`.
    *   Add `;%PATH_TO_PYTHONEXE%` where `%PATH_TO_PYTHONEXE%` is the location
        of the python executable.
*   `flatc` ([Flatbuffers compiler][]) to convert [JSON][] text files to .bin
    files in the `pie_noon/assets` directory.
*   [cwebp][] is required to convert `png` images to the [webp][] format.
    Install [cwebp][] by downloading the libwebp archive for your operating
    system (see [WebP Precompiled Utilities][]) unpack somewhere on your
    system and add the directory containing the [cwebp][] binary to the `PATH`
    variable.

After modifying the data in the `pie_noon/src/rawassets` directory, the assets
need to be rebuilt by running the following command:

    cd pie_noon
    python scripts/build_assets.py

Each file under `src/rawassets` will produce a corresponding output file in
`assets`.  For example, after running the asset build
`assets/config.bin` will be generated from `src/rawassets/config.json`.

### Game Configuration

Global configuration options for the game are specified by data in
`src/rawassets/config.json` which uses the `Config` table of the
[Flatbuffers][] schema `src/flatbufferschema/config.fbs`.  The following
sections describe the most used subset of the configuration options.

#### Characters

The number of characters in the game is currently controlled by the
`character_count` field.  Characters are arranged in the scene using the
`character_arrangements` table which specifies the location of characters
based upon the number of characters referenced in each entry of the table.

#### Game Mechanics

`character_health` controls the initial health of each player when the
game starts.

Pies speed is controlled by `pie_flight_time`.  The animation of pie flight
is controlled using `pie_arc_height`, `pie_src_height_variance`,
`pie_initial_height`, `pie_rotations`, `pie_rotation_variance`,
`pie_initial_angle` and `pie_target_angle`.

#### Camera

The initial position of the camera is set via the `camera_position` and will
be facing the location specified by `camera_target`.  The camera's field of
view is controlled using `viewport_angle`, `viewport_aspect_ratio` and
`viewport_near_plane`, `viewport_far_plane`.

#### Lighting

`light_positions` allows declaration of multiple light-sources, but only the
first entry is used by the game.  The single light is omni-directional.

#### Visuals

The scene is composed of a range of objects called renderables.  The game
has a discrete number of renderable objects described by the `RenderableId`
enumeration in `src/flatbufferschemas/pie_noon_common.fbs`.  `RenderableId`
values are used to associate game objects with renderable objects.

All renderables in the game are billboards (quads) which can be rendered with
different materials.  Each renderable (`RenderableDef`) associates properties
of a billboard with a `RenderableId`.  The `RenderableDef` is used to specify
the materials (textures + shader) used to render a billboard.
Where `cardboard_front` the the billboard rendered in the foreground and
`cardboard_back` is rendered behind the foreground material to provide the
illusion of depth.  The materials used for popsicle sticks (rendered if
`stick` is true) are configured using `stick_front` and `stick_back`.

##### Characters

The renderables used for characters are configured using the
*Character State Machine* (see section below).

##### Character Accessories

Character accessories (`pie_noonter_accessories` and `health_accessories`) are
rendered next to each character.  For example, the health display is rendered
alongside the character to illustrate the characters remaining health.

##### Props

Props (`Prop`) are static billboards that are positioned in the scene with
positions relative to world coordinates.  Props are configured using the
`props` field and are unconditionally rendered.

##### Materials

Each renderable must be associated with a material which specifies how the
renderable is rendered.  Material [JSON][] files are located in
`src/rawassets/materials` and use the `src/flatbufferschemas/materials.fbs`
[Flatbuffers][] schema.

Fields that reference materials (`cardboard_front`, `cardboard_back`,
`stick_front` and `stick_back`) specify the path of the material .bin file
*generated* from the [JSON][] in `src/rawassets/materials`.  For example,
to use `src/rawassets/materials/pie_small.json` the designer should specify
`materials/pie_small.bin` string.

The `shader_basename` references a vertex, fragment shader pair from
`assets/shaders` to use to render the material.  For example, the string
`shaders/textured` will cause the game to load the
`assets/shaders/textured.glslf` fragment shader and the
`assets/shaders/textured.glslv` vertex shader.

Textures are associated with a material using `texture_filenames` where the
filenames - in a similar fashion to shader paths - are relative to the
`assets` directory.  Each material can be associated with multiple textures,
where the use of each texture depends upon the shader associated wih the
material.

##### Shaders

###### shaders/color

`shaders/color` does not take texture input and simply renders polygons using
vertex colors specified by the game.

###### shaders/lit_textured_normal

`shaders/lit_textured_normal` is a generic shader for rendering polygons with
a normal map.  This uses the first texture specified in the `texture_filenames`
list as the diffuse texture and the second texture as a normal map.  Ambient
and specular lighting of the material is controlled by the normal map and the
light position in the world (see `light_positions` in
`src/flatbufferschemas/config.fbs`).

###### shaders/simple_shadow

`shaders/simple_shadow` projects a shadow onto the ground plane (X-Z plane)
away from the light position (see `light_positions` in
`src/flatbufferschemas/config.fbs`).

##### shaders/textured

`shaders/textured` simply renders the first texture in `texture_filenames`
with a color tint - supplied by the game runtime - with no lighting.

### Audio

`SoundId` values in `src/flatbufferschemas/pie_noon_common.fbs` are used to
associate game events (states) in the *Character State Machine* with sounds.

`src/rawassets/sound_assets.json` associates `SoundId` values with sounds
(`SoundDef`, see `src/flatbufferschemas/sound.fbs`).  Each entry in the
`sounds` list references a .bin file in the `assets` directory generated from
a [JSON][] file in `src/rawassets/sounds`.  For example, to load
`src/rawassets/sounds/throw_pie.json` the `sounds` list should reference
`sounds/throw_pie.bin`.

Each sound (`SoundDef`) can reference a set of audio samples, where each
filename references a sample in the `assets` directory.  For example, to load
`assets/sounds/smack01.wav` the string `sounds/smack01.wav` should be added
to the `filename` entry of an `audio_sample` in the `audio_sample_set` list.

### Character State Machine

The character state machine is `Finite-state machine` described by the
`CharacterStateMachineDef` from the
`src/flatbufferschemas/character_state_machine_def.fbs` schema and configured
using `src/rawassets/character_state_machine_def.json`.

#### Transitions

All characters in the game start in the state specified by `initial_state`
in the `CharacterStateMachineDef`.  Transitions from each state
(`CharacterState`) are primarily dependent upon `transitions` (`Transition`).
When the `condition` of a `Transition` is satisified.

For example, the following declares two states `Loading_1` and `Throwing`:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "ThrowPie"
              }
            },
          ]
        },
        {
          "id": "Throwing",
          "transitions": [
            {
              "target_state": "Loading_1",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ]
        }
      ]
    }

The state machine will leave the `Loading_1` state and enter the `Throwing`
state when the input event `ThrowPie` is received by the character.
The `ThrowPie` event is usually mapped to a button on a game controller
(see the *Controls* section).  When the animation associated with the
`Throwing` state is complete `AnimationEnd` is fired and the state machine
transitions to the `Loading_1` state.

Conditions are not limited to listening for a single input event. To make a
condition depend on two or more inputs, just list all input events in a list
separated by spaces.  For example, `"Throwing JustHit"` would be true only if
the player pressed the `Throwing` button as the character is hit.

Additionally, conditions can be configured to only be valid for a certain time
period by using the `time` and `end_time` fields.  Times are specified in
milliseconds.  For example, if you only want to allow the character to throw
after half a second, you could add `"time": 500` to the list of conditions.

Conditions are evaluated in the order they are listed.  If more the one
condition is true, the first transition in the list will be followed.

#### Events

The pallet of events is described by `LogicalInputs` in
`src/flatbufferschemas/character_state_machine_def.fbs`.  Events map to
controller input, gameplay events and the state machine's animation
(timeline) state.

| Event        | Type           | Cause                                     |
|--------------|----------------|-------------------------------------------|
| ThrowPie     | Input          | Throw button was pressed.                 |
| Deflect      | Input          | Shield / block button was pressed.        |
| Left         | Input          | Left turn button was pressed.             |
| Right        | Input          | Right turn button was pressed.            |
| JustHit      | Game Event     | Character hit by a pie.                   |
| NoHealth     | Game Event     | Character health reduced to zero.         |
| AnimationEnd | Animation      | Timeline has reached the `end_time`.      |
| Winning      | Game Event     | Character has won the match.              |

#### Animation / Timeline

Each state in the state machine can be associated with a timeline which can
display character or character accessory renderables (see the
*Visuals* section), play sounds (see *Audio* section) or generate events for
the game to react to (see `EventId` in
`src/flatbufferschemas/pie_noon_common.fbs`).  Each item on the timeline is
associated with a timestamp (in milliseconds) which indicates when the
item happen relative to the start of the state.  When the state machine enters
a state it starts playing through items in the timeline.  The state machine
will generate the `AnimationEnd` event when the timeline's `end_time` has
been reached.

For example, the following state will generate the `AnimationEnd` event one
second after entering the state and will attempt to transition to the
`Throwing` state:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ],
          "timeline": {
            "end_time": 1000,
          }
        }
      ]
    }

The following state will display `CharacterLoad1` when entering the state
and `CharacterLoad2` 0.5 seconds after the timeline started playback:

    {
      "states": [
        {
          "id": "Loading_1",
          "transitions": [
            {
              "target_state": "Throwing",
              "condition": {
                "logical_inputs": "AnimationEnd"
              }
            },
          ],
          "timeline": {
            "end_time": 1000,
            "renderables": [
              {
                "time": 0,
                "renderable": "RenderableId.CharacterLoad1"
              },
              {
                "time": 500,
                "renderable": "RenderableId.CharacterLoad2"
              }
            ]
          }
        }
      ]
    }

## Source Layout

The following bullets describe the directory structure of the game.


| Path                          | Description                                  |
|-------------------------------|----------------------------------------------|
| `pie_noon`                    | Project build files and run script.          |
| `pie_noon/assets`             | Assets loaded by the game.                   |
| `pie_noon/external`           | OpenGL header required on Windows.           |
| `pie_noon/jni`                | Top-level Android NDK makefile.              |
| `pie_noon/jni/SDL`            | Android NDK makefile for SDL.                |
| `pie_noon/jni/SDL-mixer`      | Android NDK makefile for SDL-mixer.          |
| `pie_noon/jni/gpg`            | Android NDK makefile for Play Games Services.|
| `pie_noon/jni/src`            | Android NDK makefile for the game.           |
| `res`                         | Android specific resource files.             |
| `scripts`                     | Asset build makefiles.                       |
| `src`                         | Game's source.                               |
| `src/com/google/fpl/pie_noon` | SDL derived Android activity (entry point).  |
| `src/flatbufferschemas`       | Game schemas for [FlatBuffers][] data.       |
| `src/org/libsdl/app`          | Real entry point of the app on Android.      |
| `src/rawassets`               | JSON [FlatBuffers] used as game data.        |
| `tests`                       | Unit tests for some components of the game.  |

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
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [DirectX SDK]: http://www.microsoft.com/en-us/download/details.aspx?id=6812
  [Finite-state machine]: http://en.wikipedia.org/wiki/Finite-state_machine
  [Flatbuffers]: http://google.github.io/flatbuffers/
  [Flatbuffers compiler]: http://google.github.io/flatbuffers/md__compiler.html
  [Flatbuffers schema]: http://google.github.io/flatbuffers/md__schemas.html
  [Linux]: http://en.wikipedia.org/wiki/Linux
  [JSON]: http://json.org/
  [managing avds]: http://developer.android.com/tools/devices/managing-avds.html
  [NDK Eclipse plugin]: http://developer.android.com/sdk/index.html
  [OS X]: http://www.apple.com/osx/
  [Python]: http://python.org/
  [Ubuntu]: http://www.ubuntu.com
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
  [webp]: https://developers.google.com/speed/webp/
  [Xcode]: http://developer.apple.com/xcode/
  [Xquartz]: http://xquartz.macosforge.org/

