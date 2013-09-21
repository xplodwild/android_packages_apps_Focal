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

import org.cyanogenmod.focal.CameraManager;
import fr.xplod.focal.R;

public class VideoFrWidget extends SimpleToggleWidget {

    public VideoFrWidget(CameraManager cam, Context context) {
        super(cam, context, "video-hfr", R.drawable.ic_widget_videofr);

        // For video only
        setVideoOnly(true);

        // TODO: Needs filtering depending on resolution (see video-hfr-size)
        addValue("off", R.drawable.ic_widget_videofr_30, "30 FPS");
        addValue("60", R.drawable.ic_widget_videofr_60, "60 FPS");
        addValue("90", R.drawable.ic_widget_videofr_90, "90 FPS");
        addValue("120", R.drawable.ic_widget_videofr_120, "120 FPS");

        getToggleButton().setHintText(R.string.widget_videofr);
    }
}
