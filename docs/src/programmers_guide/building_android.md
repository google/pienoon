Building for Android    {#pie_noon_guide_building_android}
====================


# Version Requirements

Following are the minimum tested versions for the tools and libraries you
need for building [Pie Noon][] for Android:

-   [Android SDK][]:  Android 5.0 (API Level 21)
-   [ADT][]: 20140702
-   [Android NDK][]: android-ndk-r10e
-   NDK plugn for Eclipse: Bundled with ADT
-   [Google Play Games C++ SDK][]: 1.4 or above
-   [cmake][]: 2.8.12 or newer
-   [Python][]: 2.7.*
-   [cwebp][]: 0.4.0 or newer (download from the
    [WebP Precompiled Utilities][])
-   [Ragel][]: 6.9 or newer

# Before Building

-   Install prerequisites for the developer machine's operating system.
    -   [Linux prerequisites](@ref building_linux_prerequisites)
    -   [OS X prerequisites](@ref building_osx_prerequisites)
    -   [Windows prerequisites](@ref building_windows_prerequisites)
-   Install [fplutil prerequisites][]
-   Install the [Android SDK][].
-   Install the [Android NDK][].
-   Download the [Google Play Games C++ SDK][] and unpack into Pie Noon's
    `dependencies/gpg-cpp-sdk` directory.
    -   For example, if you've fetched Pie Noon to `~/pie_noon/`, unpack
        the downloaded `gpg-cpp-sdk.v1.4.zip` to
        `~/pie_noon/dependencies/gpg-cpp-sdk`.

## Set up Google Play Games Services

Optionally, to use the [Google Play Games Services][] features in the game,
follow the steps below to set up [Google Play Games Services][] IDs:

-   Create an App ID with new package name in the
    [Google Play Developer Console][].
    -   Replace `app_id` with the newly created one in
        `res/values/strings.xml`.
    -   Update the package name in `AndroidManifest.xml` and java files.
        - For example, rename `com.google.fpl.pie_noon` to
          `com.mystudio.coolgame`.
-   Create 5 achievement IDs and 14 leaderboards in the
    [Google Play Developer Console][].
    -   Replace Google Play Games IDs in `pie_noon_game.cpp`
        (`achievements[]` and `gpg_ids[]`).

# Building

The [Pie Noon][] project has an `AndroidManifest.xml` file which contains
details about how to build an Android package (apk).

To build the project:

-   Open a command line window.
-   Go to the [Pie Noon][] directory.
-   Run [build_all_android][] to build the project.

For example:

    cd pie_noon
    ./dependencies/fplutil/bin/build_all_android -E dependencies jni/libs

# Installing and running the game.

Install the game using [build_all_android][].

For example, the following will install and run the game on a device attached
to the workstation with serial number `ADA123123`.

    cd pie_noon
    ./dependencies/fplutil/bin/build_all_android -E dependencies jni/libs -d ADA123123 -i -r -S

If only one device is attached to a workstation, the `-d` argument
(which selects a device) can be ommitted.

<br>

  [Pie Noon]: @ref pie_noon_guide_overview
  [adb]: http://developer.android.com/tools/help/adb.html
  [ADT]: http://developer.android.com/tools/sdk/eclipse-adt.html
  [Android Developer Tools]: http://developer.android.com/sdk/index.html
  [Android NDK]: http://developer.android.com/tools/sdk/ndk/index.html
  [Android SDK]: http://developer.android.com/sdk/index.html
  [NDK Eclipse plugin]: http://developer.android.com/sdk/index.html
  [apk]: http://en.wikipedia.org/wiki/Android_application_package
  [fplutil]: http://google.github.io/fplutil
  [fplutil prerequisites]: http://google.github.io/fplutil/fplutil_prerequisites.html
  [managing avds]: http://developer.android.com/tools/devices/managing-avds.html
  [Google Play Games C++ SDK]: http://developers.google.com/games/services/downloads/
  [Google Play Games Services]: http://developer.android.com/google/play-services/games.html
  [build_all_android]: http://google.github.io/fplutil/build_all_android.html
  [cmake]: http://www.cmake.org/
  [Python]: http://www.python.org/download/releases/2.7/
  [cwebp]: http://developers.google.com/speed/webp/docs/cwebp
  [WebP Precompiled Utilities]: http://developers.google.com/speed/webp/docs/precompiled
  [Google Play Developer Console]: http://play.google.com/apps/publish/
  [Ragel]: http://www.colm.net/open-source/ragel/
