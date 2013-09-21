/*
 * Copyright (C) 2013 Guillaume Lesniak
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

package org.cyanogenmod.focal;

import android.content.Context;
import android.content.SharedPreferences;

/**
 * Manages the storage of all the settings
 */
public class SettingsStorage {
    private final static String PREFS_CAMERA = "nemesis-camera";
    private final static String PREFS_APP = "nemesis-app";
    private final static String PREFS_VISIBILITY = "nemesis-visibility";
    private final static String PREFS_SHORTCUTS = "nemesis-shortcuts";
    public final static String TAG = "SettingsStorage";

    private static void store(Context context, String prefsName, String key, String value) {
        SharedPreferences prefs = context.getSharedPreferences(prefsName, 0);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putString(key, value);
        editor.commit();
    }

    private static String retrieve(Context context, String prefsName, String key, String def) {
        SharedPreferences prefs = context.getSharedPreferences(prefsName, 0);
        return prefs.getString(key, def);
    }

    /**
     * Stores a setting of the camera parameters array
     * @param context
     * @param key
     * @param value
     */
    public static void storeCameraSetting(Context context, int facing, String key, String value) {
        store(context, PREFS_CAMERA, Integer.toString(facing) + ":" + key, value);
    }

    /**
     * Returns a setting of the camera
     * @param context
     * @param key
     * @param def Default if key doesn't exist
     * @return
     */
    public static String getCameraSetting(Context context, int facing, String key, String def) {
        return retrieve(context, PREFS_CAMERA, Integer.toString(facing) + ":" + key, def);
    }

    /**
     * Stores a setting of the camera parameters array
     * @param context
     * @param key
     * @param value
     */
    public static void storeAppSetting(Context context, String key, String value) {
        store(context, PREFS_APP, key, value);
    }

    /**
     * Returns a setting of the app
     * @param context
     * @param key
     * @param def Default if key doesn't exist
     * @return
     */
    public static String getAppSetting(Context context, String key, String def) {
        return retrieve(context, PREFS_APP, key, def);
    }

    /**
     * Stores a setting of the widgets visibility
     * @param context
     * @param key
     * @param visible
     */
    public static void storeVisibilitySetting(Context context, String key, boolean visible) {
        store(context, PREFS_VISIBILITY, key, visible ? "true" : "false");
    }

    /**
     * Returns a setting of the app (always defaults to visible)
     * @param context
     * @param key
     * @return
     */
    public static boolean getVisibilitySetting(Context context, String key) {
        return retrieve(context, PREFS_VISIBILITY, key, "true").equals("true");
    }

    /**
     * Stores a setting of the widgets shortcut
     * @param context
     * @param key
     * @param pinned
     */
    public static void storeShortcutSetting(Context context, String key, boolean pinned) {
        store(context, PREFS_SHORTCUTS, key, pinned ? "true" : "false");
    }

    /**
     * Returns a setting of the widgets shortcut
     * @param context
     * @param key
     * @return
     */
    public static boolean getShortcutSetting(Context context, String key) {
        return retrieve(context, PREFS_SHORTCUTS, key, "false").equals("true");
    }
}
