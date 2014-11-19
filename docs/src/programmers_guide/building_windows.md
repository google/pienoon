Building for Windows    {#pie_noon_guide_building_windows}
====================

You can use [cmake][] to generate a [Visual Studio][] project for
[Pie Noon][] on
[Windows][].

# Version Requirements

These are the minimum required versions for building [Pie Noon][] for Windows:

-   [Windows][]: 7
-   [Visual Studio][]: 2010 or 2012
-   [DirectX SDK][]: 9.29.1962 or newer.
-   [cmake][]: 2.8.12 or newer.
-   [Python][]: 2.7.*
-   [cwebp][]: 0.4.0 or newer (download from the
    [WebP Precompiled Utilities][] page)

# Before Building    {#building_windows_prerequisites}

-   Install [DirectX SDK][]
-   Configure the `DXSDK_DIR` variable needs to point to the install location
    of the [DirectX SDK][], for example
    `c:\Program Files (x86)\Microsoft DirectX SDK (June 2010)`
    (see [Setting Windows Environment Variables][]).
-   Install [cmake][]
-   Install [Python][]
-   Download the libwebp archive for [Windows][] from
    [WebP Precompiled Utilities][].
-   Unpack [cwebp][] to a directory on your system.
-   Add directory containing [cwebp][] to the `PATH` variable.
    -   For example, if [cwebp][] is installed in `c:\cwebp` the
        path `c:\cwebp\bin` should be added to the `PATH` variable
        (see [Setting Windows Environment Variables][]).

# Creating the Visual Studio Solution using cmake

Use [cmake][] to generate the [Visual Studio][] solution and project files.

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

# Building with Visual Studio

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

# Executing the Game

-   Right-click on the `pie_noon` project in the Solution Explorer
    pane, and select "Set as StartUp Project".
-   Select "Debug-->Start Debugging" from the menu.

<br>

  [CMake]: http://www.cmake.org
  [Pie Noon]: @ref pie_noon_guide_overview
  [Visual Studio]: http://www.visualstudio.com/
  [Windows]: http://windows.microsoft.com/
  [DirectX SDK]: http://www.microsoft.com/en-us/download/details.aspx?id=6812
  [Python]: http://www.python.org/download/releases/2.7/
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Setting Windows Environment Variables]: http://www.computerhope.com/issues/ch000549.htm
