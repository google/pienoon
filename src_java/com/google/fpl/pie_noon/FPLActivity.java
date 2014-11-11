// Copyright 2014 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Helper class that allows us to rename our app.
// Can't just modify SDLActivity, since the native code depends on that package.

package com.google.fpl.pie_noon;

import org.libsdl.app.SDLActivity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.app.Activity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.view.Display;
import android.widget.TextView;
import android.widget.ScrollView;
import android.app.AlertDialog;
import android.util.Log;
import android.util.TypedValue;
import android.graphics.Point;
import android.content.Context;
import android.content.SharedPreferences;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.text.util.Linkify;

public class FPLActivity extends SDLActivity {
  // GPG's GUIs need activity lifecycle events to function properly, but
  // they don't have access to them. This code is here to route these events
  // back to GPG through our C++ code.
  protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    super.onActivityResult(requestCode, resultCode, data);
    nativeOnActivityResult(this, requestCode, resultCode, data);
  }

  boolean textDialogOpen;

  @Override
  public void onWindowFocusChanged(boolean hasFocus) {
    super.onWindowFocusChanged(hasFocus);
    if (hasFocus) {
      textDialogOpen = false;
    }
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
    String title;
    String text;
    boolean html;
    public TextDialogRunnable(Activity activity, String title, String text,
                              boolean html) {
      this.activity = activity;
      this.title = title;
      this.text = text;
      this.html = html;
    }
    public void run() {
      try {
        textDialogOpen = true;
        TextView tv = new TextView(activity);

        tv.setText(html ? Html.fromHtml(text) : text);
        if (html) {
          tv.setMovementMethod(LinkMovementMethod.getInstance());
        }
        tv.setLayoutParams(new LayoutParams(
                               LayoutParams.WRAP_CONTENT,
                               LayoutParams.WRAP_CONTENT));
        ScrollView sv = new ScrollView(activity.getApplicationContext());
        sv.setLayoutParams(new LayoutParams(
                               LayoutParams.FILL_PARENT,
                               LayoutParams.FILL_PARENT));
        sv.setPadding(DpToPx(20), DpToPx(20), DpToPx(20), DpToPx(0));
        sv.addView(tv);
        AlertDialog alert = new AlertDialog.Builder(activity, AlertDialog.THEME_DEVICE_DEFAULT_DARK)
            .setTitle(title)
            .setView(sv)
            .setNeutralButton("OK", null)
            .create();
        alert.show();
      } catch (Exception e) {
        textDialogOpen = false;
        Log.e("SDL", "exception", e);
      }
    }
  }

  // Capture motionevents and keyevents to check for gamepad movement.  Any events we catch
  // (That look like they were from a gamepad or joystick) get sent to C++ via JNI, where
  // they are stored, so C++ can deal with them next time it updates the game state.
  @Override
  public boolean dispatchGenericMotionEvent(MotionEvent event) {
    if ((event.getAction() == MotionEvent.ACTION_MOVE) &&
       (event.getSource() & (InputDevice.SOURCE_JOYSTICK | InputDevice.SOURCE_GAMEPAD)) != 0) {
       float axisX = event.getAxisValue(MotionEvent.AXIS_X);
       float axisY = event.getAxisValue(MotionEvent.AXIS_Y);
       float hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X);
       float hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y);
       float finalX, finalY;
       // Decide which values to send, based on magnitude.  Hat values, or analog/axis values?
       if (Math.abs(axisX) + Math.abs(axisY) > Math.abs(hatX) + Math.abs(hatY)) {
         finalX = axisX;
         finalY = axisY;
       } else {
         finalX = hatX;
         finalY = hatY;
       }
       nativeOnGamepadInput(event.getDeviceId(), event.getAction(),
                          0,  // Control Code is not needed for motionEvents.
                          finalX, finalY);
    }
    return super.dispatchGenericMotionEvent(event);
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event)
  {
  if ((event.getSource() &
      (InputDevice.SOURCE_JOYSTICK | InputDevice.SOURCE_GAMEPAD)) != 0) {
          nativeOnGamepadInput(event.getDeviceId(), event.getAction(),
                               event.getKeyCode(), 0.0f, 0.0f);
    }
    return super.dispatchKeyEvent(event);
  }

  public void showTextDialog(String title, String text, boolean html) {
    runOnUiThread(new TextDialogRunnable(this, title, text, html));
  }

  public boolean isTextDialogOpen() {
    return textDialogOpen;
  }

  public boolean hasSystemFeature(String featureName) {
    return getPackageManager().hasSystemFeature(featureName);
  }

  public void WritePreference(String key, int value) {
    SharedPreferences.Editor ed = getPreferences(Context.MODE_PRIVATE).edit();
    ed.putInt(key, value);
    ed.commit();
  }

  public int ReadPreference(String key, int default_value) {
    return getPreferences(Context.MODE_PRIVATE).getInt(key, default_value);
  }

  public int DpToPx(int dp) {
    // Convert the dps to pixels, based on density scale
    return (int)TypedValue.applyDimension(
      TypedValue.COMPLEX_UNIT_DIP, dp, getResources().getDisplayMetrics());
  }

  // Implemented in C++. (gpg_manager.cpp)
  private static native void nativeOnActivityResult(
      Activity activity,
      int requestCode,
      int resultCode,
      Intent data);

  // Implemented in C++. (input.cpp)
  private static native void nativeOnGamepadInput(
      int controllerId,
      int eventCode,
      int controlCode,
      float x,
      float y);

}

