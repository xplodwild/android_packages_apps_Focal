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
        super(cam, context, R.drawable.ic_widget_hdr);

        // Detect if device uses ae-bracket-hdr (qualcomm) or AOSP's scene-mode=hdr
        Camera.Parameters params = cam.getParameters();

        if (params.get("ae-bracket-hdr") != null && !params.get("ae-bracket-hdr").isEmpty()) {
            // Use ae-bracket-hdr
            setKey("ae-bracket-hdr");
            addValue("Off", R.drawable.ic_widget_hdr_off);
            addValue("HDR", R.drawable.ic_widget_hdr_on);
            addValue("AE-Bracket", R.drawable.ic_widget_hdr_aebracket);
        } else if (params.getSupportedSceneModes().contains("hdr")) {
            // Use HDR scene-mode
            setKey("scene-mode");
            addValue("auto", R.drawable.ic_widget_hdr_off);
            addValue("hdr", R.drawable.ic_widget_hdr_on);
        }
    }
}
