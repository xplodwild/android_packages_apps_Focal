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

/**
 * ISO widget, sets ISO sensitivity value
 */
public class IsoWidget extends SimpleToggleWidget {
    public IsoWidget(CameraManager cam, Context context) {
        super(cam, context, "iso", R.drawable.ic_widget_iso);

        addValue("auto",    R.drawable.ic_widget_iso_auto);
        addValue("ISO_HJR", R.drawable.ic_widget_iso_hjr);
        addValue("ISO100",  R.drawable.ic_widget_iso_100);
        addValue("ISO200",  R.drawable.ic_widget_iso_200);
        addValue("ISO400",  R.drawable.ic_widget_iso_400);
        addValue("ISO800",  R.drawable.ic_widget_iso_800);
        addValue("ISO1600", R.drawable.ic_widget_iso_1600);
    }
}
