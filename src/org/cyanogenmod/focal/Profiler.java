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

import android.os.SystemClock;
import android.util.Log;

import java.util.HashMap;

/**
 * Simple profiler to help debug app speed
 */
public class Profiler {
    private static final String TAG = "Profiler";

    private static Profiler sDefault = null;
    private HashMap<String, Long> mTimestamps;

    public static Profiler getDefault() {
        if (sDefault == null) sDefault = new Profiler();
        return sDefault;
    }

    private Profiler() {
        mTimestamps = new HashMap<String, Long>();
    }

    public void start(String name) {
        mTimestamps.put(name, SystemClock.uptimeMillis());
    }

    public void logProfile(String name) {
        long time = mTimestamps.get(name);
        long delta = SystemClock.uptimeMillis() - time;

        Log.d(TAG, "Time for '" + name + "': " + delta + "ms");
    }
}
