Building for OS X    {#pie_noon_guide_building_osx}
=================

You can use [cmake][] to generate an [Xcode][] project for PieNoon on [OS X][].

# Version Requirements

These are the minimum required versions for building PieNoon on OS X:

-   [OS X][]: Mavericks 10.9.1.
-   [Xcode][]: 5.1.1 or newer
-   [Xquartz][]: 2.7.5 (xorg-server 1.14.4)
-   [cmake][]: 2.8.12 or newer
-   [Python][]: 2.7.*
-   [cwebp][]: 0.4.0 or newer (download from the
    [WebP Precompiled Utilities][])

# Before Building    {#building_osx_prerequisites}

-   Install [Xquartz][].
-   Reboot your machine.  Rebooting sets the `DISPLAY` environment variable for
    [Xquartz][], which enables sample applications to run correctly.
-   Download the libwebp archive for [OS X][] from
    [WebP Precompiled Utilities][].
-   Unpack [cwebp][] to a directory on your system.
-   Add directory containing [cwebp][] to the `PATH` variable.
    -   For example, if [cwebp][] is installed in `/home/dev/cwebp` the
        following line should be added to the user's bash resource file
        `~/.bashrc`.<br>
        `export PATH="$PATH:/home/dev/cwebp/bin"`
-   Install [Ragel][] using package manager(e.g. brew install ragel).

# Creating the Xcode project using cmake

The [Xcode][] project is generated using [cmake][].

For example, the following generates the [Xcode][] project in the `pie_noon`
directory.

~~~{.sh}
    cd pie_noon
    cmake -G "Xcode"
~~~

# Building with Xcode

-   Double-click on `pie_noon/pie_noon.xcodeproj` to open the project in
    [Xcode][].
-   Select "Product-->Build" from the menu.

You can also build the game from the command-line.

-   Run `xcodebuild` after generating the Xcode project to build all targets.
-   You may need to force the `generated_includes` target to be built first.

For example, in the pie_noon directory:

~~~{.sh}
    xcodebuild -target generated_includes
    xcodebuild
~~~

# Executing the Game

-   Select "pie_noon" in `Scheme`, for example "pie_noon-->My Mac 64-bit", from
    the combo box to the right of the "Run" button.
-   Click the "Run" button.


<br>

  [cmake]: http://www.cmake.org
  [Pie Noon]: @ref pie_noon_guide_overview
  [OS X]: http://www.apple.com/osx/
  [Xcode]: http://developer.apple.com/xcode/
  [Xquartz]: http://xquartz.macosforge.org/
  [Python]: http://www.python.org/download/releases/2.7/
  [cwebp]: https://developers.google.com/speed/webp/docs/cwebp
  [WebP Precompiled Utilities]: https://developers.google.com/speed/webp/docs/precompiled
  [Ragel]: http://www.colm.net/open-source/ragel/
