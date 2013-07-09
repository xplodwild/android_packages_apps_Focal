/**
 * Copyright (C) 2013 The CyanogenMod Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

package org.cyanogenmod.nemesis;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;


/**
 * Manages the storage of all the settings
 */
public class SettingsStorage {
    private final static String PREFS_CAMERA = "nemesis-camera";
    public final static String TAG = "SettingsStorage";

    /**
     * Stores a setting of the camera parameters array
     * @param context
     * @param key
     * @param value
     */
    public static void storeCameraSetting(Context context, int facing, String key, String value) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_CAMERA, 0);
        SharedPreferences.Editor editor = prefs.edit();

        editor.putString(Integer.toString(facing) + ":" + key, value);
        editor.commit();
    }


    /**
     * Returns a setting of the camera
     * @param context
     * @param key
     * @param def Default if key doesn't exist
     * @return
     */
    public static String getCameraSetting(Context context, int facing, String key, String def) {
        SharedPreferences prefs = context.getSharedPreferences(PREFS_CAMERA, 0);
        String value = prefs.getString(Integer.toString(facing) + ":" + key, def);

        return value;
    }
}
