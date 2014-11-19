Building for Linux    {#pie_noon_guide_building_linux}
==================

# Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building PieNoon for Linux:

-   [OpenGL][]: libglapi-mesa 8.0.4 (tested with libglapi-mesa 8.0.4-0ubuntu0)
-   [GLU][]: libglu1-mesa-dev 8.0.4 (tested with
    libglu1-mesa-dev 8.0.4.0ubuntu0)
-   [cmake][]: 2.8.12 or newer
-   [automake][]: 1.141 or newer
-   [autoconf][]: 2.69 or newer
-   [libtool][]: 2.4.2 or newer

# Before Building

Prior to building, install the following components using the [Linux][]
distribution's package manager:
-    [cmake][]. You can also manually install from
     [cmake.org](http://cmake.org).
-    [OpenGL][] (`libglapi-mesa`).
-    [GLU][] (`libglu1-mesa-dev`).
-    [autoconf][], [automake][], and [libtool][]

For example, on Ubuntu:

    sudo apt-get install cmake
    sudo apt-get install libglapi-mesa
    sudo apt-get install libglu1-mesa-dev
    sudo apt-get install autoconf automake libtool

# Building

-   Generate makefiles from the [cmake][] project in the `pie_noon` directory.
-   Execute `make` to build the game.

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

    ./bin/pie_noon


<br>

  [autoconf]: http://www.gnu.org/software/autoconf/
  [automake]: http://www.gnu.org/software/automake/
  [libtool]: http://www.gnu.org/software/libtool/
  [cmake]: http://www.cmake.org/
  [Linux]: http://en.wikipedia.org/wiki/Linux
  [Pie Noon]: @ref pie_noon_guide_overview
  [OpenGL]: http://www.mesa3d.org/
  [GLU]: http://www.mesa3d.org/
