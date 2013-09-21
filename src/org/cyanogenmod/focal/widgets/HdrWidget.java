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

package org.cyanogenmod.focal.widgets;

import android.content.Context;
import android.hardware.Camera;

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

import java.util.List;

public class HdrWidget extends SimpleToggleWidget {
    private final static String KEY_PARAMETER = "ae-bracket-hdr";

    public HdrWidget(CameraManager cam, Context context) {
        super(cam, context, "ae-bracket-hdr", R.drawable.ic_widget_hdr);
        getToggleButton().setHintText(R.string.widget_hdr);

        // Reminder: AOSP's HDR mode is scene-mode, so we did put that in scene-mode
        // Here, it's for qualcomm's ae-bracket-hdr param. We filter out scene-mode
        // HDR in priority though, for devices like the Nexus 4 which reports
        // ae-bracket-hdr, but doesn't use it.
        Camera.Parameters params = cam.getParameters();
        if (params == null) {
            return;
        }

        List<String> sceneModes = params.getSupportedSceneModes();

        if (sceneModes != null && !sceneModes.contains("hdr")) {
            addValue("Off", R.drawable.ic_widget_hdr_off, context.getString(R.string.disabled));
            addValue("HDR", R.drawable.ic_widget_hdr_on, context.getString(R.string.enabled));
            addValue("AE-Bracket", R.drawable.ic_widget_hdr_aebracket,
                    context.getString(R.string.widget_hdr_aebracket));

            restoreValueFromStorage(KEY_PARAMETER);
        }
    }
}
