Installing and Running {#pie_noon_guide_installing_and_running}
======================

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

