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

package org.cyanogenmod.nemesis.widgets;

import android.content.Context;
import android.hardware.Camera;

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

public class HdrWidget extends SimpleToggleWidget {

    public HdrWidget(CameraManager cam, Context context) {
        super(cam, context, "ae-bracket-hdr", R.drawable.ic_widget_hdr);

        // Reminder: AOSP's HDR mode is scene-mode, so we did put that in scene-mode
        // Here, it's for qualcomm's ae-bracket-hdr param. We filter out scene-mode hdr
        // in priority though, for devices like Nexus 4 which reports ae-bracket-hdr, but
        // doesn't use it.
        Camera.Parameters params = cam.getParameters();

        if (!params.getSupportedSceneModes().contains("hdr")) {
            addValue("Off", R.drawable.ic_widget_hdr_off);
            addValue("HDR", R.drawable.ic_widget_hdr_on);
            addValue("AE-Bracket", R.drawable.ic_widget_hdr_aebracket);
        }
    }
}
