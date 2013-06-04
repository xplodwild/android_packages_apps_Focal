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

import org.cyanogenmod.nemesis.CameraManager;
import org.cyanogenmod.nemesis.R;

public class SceneModeWidget extends SimpleToggleWidget {

    public SceneModeWidget(CameraManager cam, Context context) {
        super(cam, context, "scene-mode", R.drawable.ic_widget_scenemode);

        addValue("auto", R.drawable.ic_widget_scenemode_auto);
        addValue("portrait", R.drawable.ic_widget_scenemode_portrait);
        addValue("landscape", R.drawable.ic_widget_scenemode_landscape);
        addValue("night", R.drawable.ic_widget_scenemode_night);
        addValue("nightportrait", R.drawable.ic_widget_scenemode_nightportrait);
        addValue("beach", R.drawable.ic_widget_scenemode_beach);
        addValue("snow", R.drawable.ic_widget_scenemode_snow);
        addValue("sports", R.drawable.ic_widget_scenemode_sports);
        addValue("party", R.drawable.ic_widget_scenemode_party);
        addValue("barcode", R.drawable.ic_widget_scenemode_barcode);
    }
}
