/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.focal;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.hardware.Camera.CameraInfo;
import android.util.Log;

// We want to disable camera-related activities if there is no camera. This
// receiver runs when BOOT_COMPLETED intent is received. After running once
// this receiver will be disabled, so it will not run again.
public class DisableCameraReceiver extends BroadcastReceiver {
    private static final String TAG = "FocalDisableCameraReceiver";
    private static final String ACTIVITIES[] = {
        "org.cyanogenmod.focal.CameraActivity",
    };

    @Override
    public void onReceive(Context context, Intent intent) {
        // Disable camera-related activities if there is no camera.
        boolean needCameraActivity = (android.hardware.Camera.getNumberOfCameras() > 0);

        if (!needCameraActivity) {
            Log.i(TAG, "no camera detected: disable all focal activities");
            for (int i = 0; i < ACTIVITIES.length; i++) {
                disableComponent(context, ACTIVITIES[i]);
            }
        }

        // Disable this receiver so it won't run again.
        disableComponent(context, "org.cyanogenmod.focal.DisableCameraReceiver");
    }

    private void disableComponent(Context context, String klass) {
        ComponentName name = new ComponentName(context, klass);
        PackageManager pm = context.getPackageManager();

        // We need the DONT_KILL_APP flag, otherwise we will be killed
        // immediately because we are in the same app.
        pm.setComponentEnabledSetting(name,
            PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP);
    }
}

