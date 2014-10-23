// Helper class that allows us to rename our app.
// Can't just modify SDLActivity, since the native code depends on that package.

package com.google.fpl.splat;

import org.libsdl.app.SDLActivity;
import android.content.Intent;
import android.app.Activity;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.Display;
import android.widget.TextView;
import android.widget.ScrollView;
import android.app.AlertDialog;
import android.util.Log;
import android.graphics.Point;

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
    final int BUILD_VERSION_KITCAT = 18;
    if (android.os.Build.VERSION.SDK_INT >= BUILD_VERSION_KITCAT &&
        hasFocus) {
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

  private class TextDialogRunnable implements Runnable {
    Activity activity;
    String text;
    public TextDialogRunnable(Activity activity, String text) {
      this.activity = activity;
      this.text = text;
    }
    public void run() {
      try {
        TextView tv = new TextView(activity);
        tv.setText(text);
        tv.setLayoutParams(new LayoutParams(
                               LayoutParams.WRAP_CONTENT,
                               LayoutParams.WRAP_CONTENT));
        ScrollView sv = new ScrollView(activity.getApplicationContext());
        tv.setLayoutParams(new LayoutParams(
                               LayoutParams.FILL_PARENT,
                               LayoutParams.FILL_PARENT));
        sv.addView(tv);
        AlertDialog alert = new AlertDialog.Builder(activity)
            .setTitle("Open Source Licenses")
            .setView(sv)
            .setNeutralButton("OK", null)
            .create();
        alert.show();
        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        alert.getWindow().setLayout(size.x / 2, size.y - 50);
      } catch (Exception e) {
        Log.e("SDL", "exception", e);
      }
    }
  }

  public void showTextDialog(String text) {
    runOnUiThread(new TextDialogRunnable(this, text));
  }

  // Implemented in C++.
  private static native void nativeOnActivityResult(
      Activity activity,
      int requestCode,
      int resultCode,
      Intent data);
}

