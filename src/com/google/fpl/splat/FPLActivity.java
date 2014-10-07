// Helper class that allows us to rename our app.
// Can't just modify SDLActivity, since the native code depends on that package.

package com.google.fpl.splat;

import org.libsdl.app.SDLActivity;
import android.content.Intent;
import android.app.Activity;

public class FPLActivity extends SDLActivity {
    // This is needed for GPG's UIs to function correctly.
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);
      nativeOnActivityResult(this, requestCode, resultCode, data);
    }
    // Implemented in C++.
    private static native void nativeOnActivityResult(
        Activity activity,
        int requestCode,
        int resultCode,
        Intent data);
}

