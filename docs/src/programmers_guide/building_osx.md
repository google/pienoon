Building for OS X    {#pie_noon_guide_building_osx}
=================

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


<br>

  [cmake]: http://www.cmake.org
  [Pie Noon]: @ref pie_noon_guide_overview
  [OS X]: http://www.apple.com/osx/
  [Xcode]: http://developer.apple.com/xcode/
  [Xquartz]: http://xquartz.macosforge.org/
