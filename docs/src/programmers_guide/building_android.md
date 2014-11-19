Building for Android    {#pie_noon_guide_building_android}
====================


#### Version Requirements

Following are the minimum required versions for the tools and libraries you
need for building PieNoon for Android:

-   Android SDK:  Android 5.0 (API Level 21)
-   ADT: 20140702
-   NDK: android-ndk-r10c
-   NDK plugn for Eclipse: Bundled with ADT

#### Before Building

-   Install the [Android SDK].
-   Install the [Android NDK].
-   Download the [Google Play Games C++ SDK] and unpack into Pie Noon's
    `dependencies/gpg-cpp-sdk` directory.
    -   For example, if you've fetched Pie Noon to `~/pie_noon/`, unpack
        the downloaded `gpg-cpp-sdk.v1.3.zip` to
        `~/pie_noon/dependencies/gpg-cpp-sdk`.

#### (Optional) Set up [Google Play Games Services] IDs
To use [Google Play Games Services] features, follow steps below to set up [Google Play Games Services] IDs.
-   Create app ID with new package name in Google Play Developer Console.
    -   Replace app_id to new one in res/values/strings.xml.
    -   Update package names in AndroidManifest.xml and java files.
-   Create 5 achievement IDs and 14 leaderboards in Google Play Developer Console.
    -   Replace IDs in pie_noon_game.cpp (achievements[] and gpg_ids[]).

#### Building

The PieNoon project has an `AndroidManifest.xml` file which contains details
about how to build an Android package (apk).

To build the project:

-   Open a command line window.
-   Go to the working directory containing the project to build.
-   Execute `build_install_run.sh` to build.

For example:

    cd pie_noon
    ./build_install_run.sh DEPLOY=0 LAUNCH=0

#### Installing and running the game.

`build_install_run.sh` will also install and run the game after a build is
complete.

For example, the following will install and run the game on a device attached
to the workstation with serial number `ADA123123`.

    cd pie_noon
    ./build_install_run.sh ADB_DEVICE=ADA123123

If only one device is attached to a workstation, the `ADB_DEVICE1 argument
can be ommitted.

#### Code Generation

By default, code is generated for devices that support the `armeabi-v7a` ABI.
Alternatively, you can generate a fat `.apk` that includes code for all ABIs.
To do so, override APP\_ABI on ndk-build's command line via `build_install_run.sh`:

    ./build_install_run.sh APP_ABI=all


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
  [Google Play Games C++ SDK]: https://developers.google.com/games/services/downloads/
  [Google Play Games Services]: https://developer.android.com/google/play-services/games.html
