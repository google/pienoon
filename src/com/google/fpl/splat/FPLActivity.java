// Helper class that allows us to rename our app.
// Can't just modify SDLActivity, since the native code depends on that package.

package com.google.fpl.splat;

import org.libsdl.app.SDLActivity;
import android.content.Intent;
import android.app.Activity;
import android.view.View;

public class FPLActivity extends SDLActivity {
    // GPG's GUIs need activity lifecycle events to function properly, but
    // they don't have access to them. This code is here to route these events
    // back to GPG through our C++ code.
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
      super.onActivityResult(requestCode, resultCode, data);
      nativeOnActivityResult(this, requestCode, resultCode, data);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
      super.onWindowFocusChanged(hasFocus);
      if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.KITKAT &&
        // We use API 15 as our minimum, and these are the only features we
        // use in higher APIs, so we define cloned constants:
        final int SYSTEM_UI_FLAG_LAYOUT_STABLE = 256;  // API 16
        final int SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION = 512;  // API 16
        final int SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN = 1024;  // API 16
        final int SYSTEM_UI_FLAG_HIDE_NAVIGATION = 2;  // API 14
        final int SYSTEM_UI_FLAG_FULLSCREEN = 4;  // API 16
        final int SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 4096; // API 19
        mLayout.setSystemUiVisibility(
            SYSTEM_UI_FLAG_LAYOUT_STABLE
            | SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
            | SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | SYSTEM_UI_FLAG_FULLSCREEN
            | SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
      }
    }

    // Implemented in C++.
    private static native void nativeOnActivityResult(
        Activity activity,
        int requestCode,
        int resultCode,
        Intent data);
}

